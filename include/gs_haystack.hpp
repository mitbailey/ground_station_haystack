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
#include "adf4355.h"
#include "network.hpp"

#define SERVER_POLL_RATE 5 // Once per this many seconds
#define SEC *1000000
#define RECV_TIMEOUT 15
#define SERVER_PORT 54230

enum XBAND_STATUS
{
    XSTAT_NOT_READY = 0,
    XSTAT_INITD,
    XSTAT_ARMED
};

typedef struct
{
    rxmodem rx_modem[1];
    adf4355 ADF[1];
    network_data_t network_data[1];
    XBAND_STATUS rx_status; // 0 = not initd nor armed, 1 = initd but not armed, 2 = ready
    uint8_t netstat;
} global_data_t;

/**
 * @brief X-Band data structure.
 * 
 * From line 113 of https://github.com/SPACE-HAUC/shflight/blob/flight_test/src/cmd_parser.c
 * Used for:
 *  XBAND_SET_TX
 *  XBAND_SET_RX
 * 
 * Also, what the GUI Client sends to Roof X-Band / Haystack for configurations.
 * 
 */
typedef struct __attribute__((packed))
{
    float LO;
    float bw;
    uint16_t samp;
    uint8_t phy_gain;
    uint8_t adar_gain;
    uint8_t ftr;
    short phase[16];
} xband_set_data_t;

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

#endif // GS_HAYSTACK_HPP