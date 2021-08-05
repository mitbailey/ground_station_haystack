/**
 * @file network.cpp
 * @author Mit Bailey (mitbailey99@gmail.com)
 * @brief 
 * @version See Git tags for version information.
 * @date 2021.07.30
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <stdio.h>
#include <string.h>
#include "network.hpp"
#include "meb_debug.hpp"
#include "gs_haystack.hpp"

NetworkData::NetworkData()
{
    connection_ready = false;
    socket = -1;
    serv_ip->sin_family = AF_INET;
    serv_ip->sin_port = htons(SERVER_PORT);
    strcpy(discon_reason, "N/A");
}

NetworkFrame::NetworkFrame(NETWORK_FRAME_TYPE type, int payload_size)
{
    if (type < 0)
    {
        printf("NetworkFrame initialized with error type (%d).\n", (int)type);
        return;
    }

    if (payload_size > NETWORK_FRAME_MAX_PAYLOAD_SIZE)
    {
        printf("Cannot allocate payload larger than %d bytes.\n", NETWORK_FRAME_MAX_PAYLOAD_SIZE);
        return;
    }

    this->payload_size = payload_size;
    this->type = type;

    // TODO: Set the mode properly.
    mode = CS_MODE_ERROR;
    crc1 = -1;
    crc2 = -1;
    guid = NETWORK_FRAME_GUID;
    netstat = 0; // Will be set by the server.
    termination = 0xAAAA;

    memset(payload, 0x0, this->payload_size);
}

int NetworkFrame::storePayload(NETWORK_FRAME_ENDPOINT endpoint, void *data, int size)
{
    if (size > payload_size)
    {
        printf("Cannot store data of size larger than allocated payload size (%d > %d).\n", size, payload_size);
        return -1;
    }

    if (data == NULL)
    {
        dbprintlf("Prepping null packet.");
    }
    else
    {
        memcpy(payload, data, size);
    }

    crc1 = crc16(payload, NETWORK_FRAME_MAX_PAYLOAD_SIZE);
    crc2 = crc16(payload, NETWORK_FRAME_MAX_PAYLOAD_SIZE);

    this->endpoint = endpoint;

    // TODO: Placeholder until I figure out when / why to set mode to TX or RX.
    mode = CS_MODE_RX;

    return 1;
}

int NetworkFrame::retrievePayload(unsigned char *data_space, int size)
{
    if (size != payload_size)
    {
        printf("Data space size not equal to payload size (%d != %d).\n", size, payload_size);
        return -1;
    }

    memcpy(data_space, payload, payload_size);

    return 1;
}

int NetworkFrame::checkIntegrity()
{
    if (guid != NETWORK_FRAME_GUID)
    {
        return -1;
    }
    else if (endpoint < 0)
    {
        return -2;
    }
    else if (mode < 0)
    {
        return -3;
    }
    else if (payload_size < 0 || payload_size > NETWORK_FRAME_MAX_PAYLOAD_SIZE)
    {
        return -4;
    }
    else if (type < 0)
    {
        return -5;
    }
    else if (crc1 != crc2)
    {
        return -6;
    }
    else if (crc1 != crc16(payload, NETWORK_FRAME_MAX_PAYLOAD_SIZE))
    {
        return -7;
    }
    else if (termination != 0xAAAA)
    {
        return -8;
    }

    return 1;
}

void NetworkFrame::print()
{
    printf("GUID ------------ 0x%04x\n", guid);
    printf("Endpoint -------- %d\n", endpoint);
    printf("Mode ------------ %d\n", mode);
    printf("Payload Size ---- %d\n", payload_size);
    printf("Type ------------ %d\n", type);
    printf("CRC1 ------------ 0x%04x\n", crc1);
    printf("Payload ---- (HEX)");
    for (int i = 0; i < payload_size; i++)
    {
        printf(" 0x%04x", payload[i]);
    }
    printf("\n");
    printf("CRC2 ------------ 0x%04x\n", crc2);
    printf("NetStat --------- 0x%x\n", netstat);
    printf("Termination ----- 0x%04x\n", termination);
}

ssize_t NetworkFrame::sendFrame(NetworkData *network_data)
{
    if (!(network_data->connection_ready))
    {
        dbprintlf(YELLOW_FG "Connection is not ready.");
        return -1;
    }

    if (network_data->socket < 0)
    {
        dbprintlf(RED_FG "Invalid socket (%d).", network_data->socket);
        return -1;
    }

    if (!checkIntegrity())
    {
        dbprintlf(YELLOW_FG "Integrity check failed, send aborted.");
        return -1;
    }

    printf("Sending the following (%d):\n", network_data->socket);
    print();

    return send(network_data->socket, this, sizeof(NetworkFrame), 0);
}