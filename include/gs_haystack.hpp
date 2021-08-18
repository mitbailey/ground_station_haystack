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
#include "libiio.h"

#define SERVER_POLL_RATE 5 // Once per this many seconds
#define SEC *1000000
#define RECV_TIMEOUT 15
#define SERVER_PORT 54230

typedef struct
{
    // Three separate objects for a single x-band radio.
    rxmodem rx_modem[1]; // from rxmodem.h
    adf4355 PLL[1]; // from adf4355.h, aka pll
    adradio_t radio[1];// from libiio.h

    bool rx_modem_ready;
    bool rx_armed;
    bool PLL_ready;
    bool radio_ready;
    int last_rx_status;
    int last_read_status;

    NetDataClient *network_data;
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
 * THIS IS NOT sent to Roof X-Band / Haystack for configurations.
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
 * @brief Sent to Roof X-Band / Haystack for configurations.
 * 
 */
typedef struct
{
    // libiio.h: ensm_mode
    int mode;               // SLEEP, FDD, TDD 
    int pll_freq;           // PLL Frequency
    int64_t LO;            // LO freq
    int64_t samp;          // sampling rate
    int64_t bw;            // bandwidth
    char ftr_name[64];      // filter name
    int64_t temp;               // temperature
    double rssi;            // RSSI
    double gain;            // TX Gain
    char curr_gainmode[16]; // fast_attack or slow_attack
    bool pll_lock;
    uint32_t MTU;
} phy_config_t;

/**
 * @brief Sent to GUI client for status updates.
 * 
 */
typedef struct
{
    // libiio.h: ensm_mode
    int mode;               // SLEEP, FDD, TDD 
    int pll_freq;           // PLL Frequency
    int64_t LO;            // LO freq
    int64_t samp;          // sampling rate
    int64_t bw;            // bandwidth
    char ftr_name[64];      // filter name
    int64_t temp;               // temperature
    double rssi;            // RSSI
    double gain;            // TX Gain
    char curr_gainmode[16]; // fast_attack or slow_attack
    bool pll_lock;
    bool modem_ready;
    bool PLL_ready;
    bool radio_ready;
    bool rx_armed; // only applicable to haystack
    uint32_t MTU;
    int32_t last_rx_status;
    int32_t last_read_status;
} phy_status_t;

enum XBAND_COMMAND
{
    XBC_INIT_PLL = 0,
    XBC_DISABLE_PLL = 1,
    XBC_ARM_RX = 2,
    XBC_DISARM_RX = 3,
};

/**
 * @brief Initializes radio.
 * 
 * Called from X-Band RX thread if necessary.
 * 
 * @param global_data 
 * @return int 
 */
int gs_xband_init(global_data_t *global_data);

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
 * @brief 
 * 
 * @return int 
 */
int gs_xband_apply_config();

/**
 * @brief 
 * 
 * @return void* 
 */
void *xband_status_thread(void *);

#endif // GS_HAYSTACK_HPP