/**
 * @file gs_haystack.hpp
 * @author Mit Bailey (mitbailey99@gmail.com)
 * @brief 
 * @version See Git tags for version information.
 * @date 2021.08.04
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef GS_HAYSTACK_HPP
#define GS_HAYSTACK_HPP

#include <stdint.h>
#include "rxmodem.h"
#include "txmodem.h"
#include "network.hpp"

#define SERVER_POLL_RATE 5 // Once per this many seconds
#define SEC *1000000
#define RECV_TIMEOUT 15

enum XBAND_STATUS
{
    XSTAT_NOT_READY = 0,
    XSTAT_INITD,
    XSTAT_ARMED
};

typedef struct
{
    int thread_status;
    rxmodem rx_modem[1];
    network_data_t network_data[1];
    XBAND_STATUS rx_status; // 0 = not initd nor armed, 1 = initd but not armed, 2 = ready
    uint8_t netstat;
} global_data_t;

/**
 * @brief Listens for X-Band packets from SPACE-HAUC.
 * 
 * @param args 
 * @return void* 
 */
void *gs_xband_rx_thread(void *args);

/**
 * @brief Listens for NetworkFrames from the Ground Station Network.
 * 
 * @param args 
 * @return void* 
 */
void *gs_network_rx_thread(void *args);

/**
 * @brief Sends X-band-received data to the Ground Station Network Server.
 * 
 * @param thread_data 
 * @param buffer 
 * @param buffer_size 
 */
void gs_network_tx(global_data_t *global_data, uint8_t *buffer, ssize_t buffer_size);

/**
 * @brief Periodically polls the Ground Station Network Server for its status.
 * 
 * Doubles as the GS Network connection watch-dog, tries to restablish connection to the server if it sees that we are no longer connected.
 * 
 * @param args 
 * @return void* 
 */
void *gs_polling_thread(void *args);

/**
 * @brief Packs data into a NetworkFrame and sends it.
 * 
 * @param network_data 
 * @param type 
 * @param endpoint 
 * @param data 
 * @param data_size 
 * @return int 
 */
int gs_network_transmit(network_data_t *network_data, NETWORK_FRAME_TYPE type, NETWORK_FRAME_ENDPOINT endpoint, void *data, int data_size);

/**
 * @brief 
 * 
 * @param global_data 
 * @return int 
 */
int gs_connect_to_server(global_data_t *global_data);

/**
 * @brief 
 * 
 * From:
 * https://github.com/sunipkmukherjee/comic-mon/blob/master/guimain.cpp
 * with minor modifications.
 * 
 * @param socket 
 * @param address 
 * @param socket_size 
 * @param tout_s 
 * @return int 
 */
int gs_connect(int socket, const struct sockaddr *address, socklen_t socket_size, int tout_s);

#endif // GS_HAYSTACK_HPP