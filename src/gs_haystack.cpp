/**
 * @file gs_haystack.cpp
 * @author Mit Bailey (mitbailey99@gmail.com)
 * @brief 
 * @version See Git tags for version information.
 * @date 2021.08.04
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "gs_haystack.hpp"
#include "meb_debug.hpp"

int gs_xband_init(global_data_t *global_data)
{
    if (global_data->rx_modem_ready && global_data->radio_ready)
    {
        dbprintlf(YELLOW_FG "RX modem and radio marked as ready, but gs_xband_init(...) was called anyway. Canceling redundant initialization.");
        return -1;
    }

    if (!global_data->rx_modem_ready)
    {
        // Initialize.
        if (rxmodem_init(global_data->rx_modem, uio_get_id("rx_ipcore"), uio_get_id("rx_dma")) < 0)
        {
            dbprintlf(RED_FG "RX modem initialization failure.");
            return -1;
        }
        dbprintlf(GREEN_FG "RX modem initialized.");
        global_data->rx_modem_ready = true;
    }

    if (!global_data->radio_ready)
    {
        if (adradio_init(global_data->radio) < 0)
        {
            dbprintlf(RED_FG "Radio initialization failure.");
            return -3;
        }
        dbprintlf(GREEN_FG "Radio initialized.");
        global_data->radio_ready = true;
    }

    dbprintlf(GREEN_FG "Automatic initialization complete.");
    return 1;
}

void *gs_xband_rx_thread(void *args)
{
    global_data_t *global_data = (global_data_t *)args;

    while (!global_data->rx_modem_ready || !global_data->radio_ready)
    {
        if (gs_xband_init(global_data) < 0)
        {
            dbprintlf(RED_FG "Receive thread aborting, radio cannot initialize.");
            usleep(5 SEC);
            continue;
        }
    }

    while (global_data->network_data->thread_status > 0 && global_data->rx_modem_ready && global_data->radio_ready && global_data->PLL_ready)
    {
        ssize_t buffer_size = rxmodem_receive(global_data->rx_modem);

        uint8_t *buffer = (uint8_t *)malloc(buffer_size * sizeof(char));
        memset(buffer, 0x0, buffer_size);

        ssize_t read_size = 0;
        read_size = rxmodem_read(global_data->rx_modem, buffer, buffer_size);
        if (read_size != buffer_size)
        {
            dbprintlf(RED_FG "Read %d of %d bytes.", read_size, buffer_size);
            continue;
        }

        gs_network_transmit(global_data->network_data, CS_TYPE_DATA, CS_ENDPOINT_CLIENT, buffer, buffer_size);

        free(buffer);
    }

    if (global_data->network_data->thread_status > 0)
    {
        global_data->network_data->thread_status = 0;
    }
    return NULL;
}

void *gs_network_rx_thread(void *args)
{
    global_data_t *global_data = (global_data_t *)args;
    network_data_t *network_data = global_data->network_data;
    global_data->PLL->spi_bus = 0;
    global_data->PLL->spi_cs = 1;
    global_data->PLL->spi_cs_internal = 1;
    global_data->PLL->cs_gpio = -1;
    global_data->PLL->single = 1;
    global_data->PLL->muxval = 6;

    // Haystack is a network client to the GS Server, and so should be very similar in socketry to ground_station.

    while (network_data->rx_active)
    {
        if (!network_data->connection_ready)
        {
            usleep(5 SEC);
            continue;
        }

        int read_size = 0;

        while (read_size >= 0 && network_data->rx_active)
        {
            char buffer[sizeof(NetworkFrame) * 2];
            memset(buffer, 0x0, sizeof(buffer));

            dbprintlf(BLUE_BG "Waiting to receive...");
            read_size = recv(network_data->socket, buffer, sizeof(buffer), 0);
            dbprintlf("Read %d bytes.", read_size);

            if (read_size > 0)
            {
                dbprintf("RECEIVED (hex): ");
                for (int i = 0; i < read_size; i++)
                {
                    printf("%02x", buffer[i]);
                }
                printf("(END)\n");

                // Parse the data by mapping it to a NetworkFrame.
                NetworkFrame *network_frame = (NetworkFrame *)buffer;

                // Check if we've received data in the form of a NetworkFrame.
                if (network_frame->checkIntegrity() < 0)
                {
                    dbprintlf("Integrity check failed (%d).", network_frame->checkIntegrity());
                    continue;
                }
                dbprintlf("Integrity check successful.");

                global_data->netstat = network_frame->getNetstat();

                // For now, just print the Netstat.
                uint8_t netstat = network_frame->getNetstat();
                dbprintlf(BLUE_FG "NETWORK STATUS");
                dbprintf("GUI Client ----- ");
                ((netstat & 0x80) == 0x80) ? printf(GREEN_FG "ONLINE" RESET_ALL "\n") : printf(RED_FG "OFFLINE" RESET_ALL "\n");
                dbprintf("Roof UHF ------- ");
                ((netstat & 0x40) == 0x40) ? printf(GREEN_FG "ONLINE" RESET_ALL "\n") : printf(RED_FG "OFFLINE" RESET_ALL "\n");
                dbprintf("Roof X-Band ---- ");
                ((netstat & 0x20) == 0x20) ? printf(GREEN_FG "ONLINE" RESET_ALL "\n") : printf(RED_FG "OFFLINE" RESET_ALL "\n");
                dbprintf("Haystack ------- ");
                ((netstat & 0x10) == 0x10) ? printf(GREEN_FG "ONLINE" RESET_ALL "\n") : printf(RED_FG "OFFLINE" RESET_ALL "\n");

                // Extract the payload into a buffer.
                int payload_size = network_frame->getPayloadSize();
                unsigned char *payload = (unsigned char *)malloc(payload_size);
                if (network_frame->retrievePayload(payload, payload_size) < 0)
                {
                    dbprintlf(RED_FG "Error retrieving data.");
                    continue;
                }

                NETWORK_FRAME_TYPE type = network_frame->getType();
                switch (type)
                {
                case CS_TYPE_ACK:
                {
                    dbprintlf(BLUE_FG "Received an ACK frame!");
                    break;
                }
                case CS_TYPE_NACK:
                {
                    dbprintlf(BLUE_FG "Received a NACK frame!");
                    break;
                }
                case CS_TYPE_CONFIG_XBAND:
                {
                    dbprintlf(BLUE_FG "Received an X-Band CONFIG frame!");
                    if (!global_data->radio_ready)
                    {
                        // TODO: Send a packet indicating this.
                        dbprintlf(RED_FG "Cannot configure radio: radio not ready, does not exist, or failed to initialize.");
                        break;
                    }
                    if (network_frame->getEndpoint() == CS_ENDPOINT_HAYSTACK)
                    {
                        // xband_set_data_t *config = (xband_set_data_t *)payload;
                        // adradio_set_tx_lo(global_data->tx_modem, config->LO);
                        phy_config_t *config = (phy_config_t *)payload;
                        // TODO: Figure out how to configure the X-Band radio.

                        // RECONFIGURE XBAND
                        adradio_set_ensm_mode(global_data->radio, (ensm_mode)config->mode);
                        // TODO: set freq???
                        adradio_set_rx_lo(global_data->radio, config->LO);
                        adradio_set_samp(global_data->radio, config->samp);
                        // TODO: Set bw (bandwidth)?
                        // TODO: Set filter?
                        // TODO: Set temp?
                        // TODO: Set rssi?
                        // TODO: Set hardware gain?
                        // TODO: Set curr_gainmode?
                        adradio_set_rx_hardwaregainmode(global_data->radio, strcmp("fast_attack", config->curr_gainmode) ? SLOW_ATTACK : FAST_ATTACK);
                        // TODO: Set pll_lock?
                    }
                    else
                    {
                        dbprintlf(YELLOW_FG "Incorrectly received a configuration for Roof X-Band.");
                    }
                    break;
                }
                case CS_TYPE_POLL_XBAND_CONFIG:
                {
                    dbprintlf(BLUE_FG "Received a request for configuration information!");

                    if (!global_data->radio_ready)
                    {
                        // TODO: Send a packet indicating this.
                        dbprintlf(RED_FG "Cannot get radio config: radio not ready, does not exist, or failed to initialize.");
                        break;
                    }
                    
                    phy_status_t status[1];
                    memset(status, 0x0, sizeof(phy_config_t));
                    adradio_get_rx_bw(global_data->radio, (long long *)&status->bw);
                    adradio_get_rx_hardwaregain(global_data->radio, &status->gain);
                    adradio_get_rx_hardwaregainmode(global_data->radio, status->curr_gainmode, sizeof(status->curr_gainmode));
                    adradio_get_rx_lo(global_data->radio, (long long *)&status->LO);
                    adradio_get_rssi(global_data->radio, &status->rssi);
                    adradio_get_samp(global_data->radio, (long long *)&status->samp);
                    adradio_get_temp(global_data->radio, (long long *)&status->temp);
                    char buf[32];
                    memset(buf, 0x0, 32);
                    adradio_get_ensm_mode(global_data->radio, buf, sizeof(buf));
                    if (strcmp(buf, "SLEEP") == 0)
                    {
                        status->mode = 0;
                    }
                    else if (strcmp(buf, "FDD") == 0)
                    {
                        status->mode = 1;
                    }
                    else if (strcmp(buf, "TDD") == 0)
                    {
                        status->mode = 2;
                    }
                    else
                    {
                        status->mode = -1;
                    }
                    // NOTE: If this field is not set to true, it will be assumed this status update is for Roof X-Band! Very important!
                    status->is_haystack = true;
                    status->modem_ready = global_data->rx_modem_ready;
                    status->PLL_ready = global_data->PLL_ready;
                    status->radio_ready = global_data->radio_ready;
                    status->rx_armed = global_data->rx_armed;

                    NetworkFrame *network_frame = new NetworkFrame(CS_TYPE_POLL_XBAND_CONFIG, sizeof(phy_config_t));
                    network_frame->storePayload(CS_ENDPOINT_CLIENT, status, sizeof(phy_config_t));
                    network_frame->sendFrame(network_data);
                    delete network_frame;

                    break;
                }
                case CS_TYPE_XBAND_COMMAND:
                {
                    dbprintlf(BLUE_FG "Received XBAND command.");
                    XBAND_COMMAND *command = (XBAND_COMMAND *)payload;

                    switch(*command)
                    {
                    case XBC_INIT_PLL:
                    {
                        dbprintlf("Received PLL initialize command.");
                        if (global_data->PLL_ready)
                        {
                            dbprintlf(YELLOW_FG "PLL already initialized, canceling.");
                            break;
                        }

                        if (adf4355_init(global_data->PLL) < 0)
                        {
                            dbprintlf(RED_FG "PLL initialization failure.");
                        }
                        else if (adf4355_set_rx(global_data->PLL) < 0)
                        {
                            dbprintlf(RED_FG "PLL set RX failure.");
                        }
                        else
                        {
                            dbprintlf(GREEN_FG "PLL initialization success.");
                            global_data->PLL_ready = true;
                        }
                        break;
                    }
                    case XBC_DISABLE_PLL:
                    {
                        dbprintlf("Received Disable PLL command.");
                        if (!global_data->PLL_ready)
                        {
                            dbprintlf(YELLOW_FG "PLL already disabled, canceling.");
                            break;
                        }

                        if (adf4355_pw_down(global_data->PLL) < 0)
                        {
                            dbprintlf(RED_FG "PLL shutdown failure.");
                        }
                        else
                        {
                            dbprintlf(GREEN_FG "PLL shutdown success.");
                        }
                    }
                    case XBC_ARM_RX:
                    {
                        dbprintlf("Received Arm RX command.");
                        if (global_data->rx_armed)
                        {
                            dbprintlf(YELLOW_FG "RX already armed, canceling.");
                            break;
                        }

                        if (rxmodem_start(global_data->rx_modem) < 0)
                        {
                            dbprintlf(RED_FG "Failed to arm RX.");
                        }
                        else
                        {
                            dbprintlf("Armed RX.");
                            global_data->rx_armed = true;
                        }
                    }
                    case XBC_DISARM_RX:
                    {
                        dbprintlf("Received Disarm RX command.");
                        if (!global_data->rx_armed)
                        {
                            dbprintlf(YELLOW_FG "RX already disarmed, canceling.");
                            break;
                        }

                        if (rxmodem_stop(global_data->rx_modem) < 0)
                        {
                            dbprintlf(RED_FG "Failed to disarm RX.");
                        }
                        else
                        {
                            dbprintlf("Disarmed RX.");
                            global_data->rx_armed = false;
                        }
                    }
                    }

                    break;
                }
                case CS_TYPE_DATA:
                case CS_TYPE_CONFIG_UHF:
                case CS_TYPE_NULL:
                case CS_TYPE_ERROR:
                default:
                {
                    break;
                }
                }
                free(payload);
            }
            else
            {
                break;
            }
        }
        if (read_size == 0)
        {
            dbprintlf(RED_BG "Connection forcibly closed by the server.");
            strcpy(network_data->discon_reason, "SERVER-FORCED");
            network_data->connection_ready = false;
            continue;
        }
        else if (errno == EAGAIN)
        {
            dbprintlf(YELLOW_BG "Active connection timed-out (%d).", read_size);
            strcpy(network_data->discon_reason, "TIMED-OUT");
            network_data->connection_ready = false;
            continue;
        }
        erprintlf(errno);
    }

    network_data->rx_active = false;
    dbprintlf(FATAL "DANGER! NETWORK RECEIVE THREAD IS RETURNING!");

    if (global_data->network_data->thread_status > 0)
    {
        global_data->network_data->thread_status = 0;
    }
    return NULL;
}