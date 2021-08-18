/**
 * @file phy.hpp
 * @author Mit Bailey (mitbailey99@gmail.com)
 * @brief 
 * @version See Git tags for version information.
 * @date 2021.08.18
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef PHY_HPP
#define PHY_HPP

#include "stdint.h"

/**
 * @brief Sent to Roof X-Band / Haystack for configurations.
 * 
 */
typedef struct __attribute__((packed))
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
    uint8_t pll_lock;
    uint32_t MTU;
} phy_config_t;

/**
 * @brief Sent to GUI client for status updates.
 * 
 */
typedef struct __attribute__((packed))
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
    uint8_t pll_lock;
    uint8_t modem_ready;
    uint8_t PLL_ready;
    uint8_t radio_ready;
    uint8_t rx_armed;
    uint32_t MTU;
    int32_t last_rx_status;
    int32_t last_read_status;
} phy_status_t;

#endif // PHY_HPP