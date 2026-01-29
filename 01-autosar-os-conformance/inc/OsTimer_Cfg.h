#ifndef OS_TIMER_CFG_H
#define OS_TIMER_CFG_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file    OsTimer_Cfg.h
 * @brief   AUTOSAR OS Timer Configuration Interface
 *
 * @details
 * Provides hardware timer abstraction for ESP32 GP Timer.
 * Generates periodic interrupts for system tick generation.
 */

/* Timer configuration */
#define OS_TIMER_RESOLUTION_HZ  (1000000U)   /* 1 MHz = 1 µs resolution */
#define OS_TIMER_PERIOD_US      (1000U)      /* 1000 µs = 1 ms period */

/**
 * @brief Timer callback type
 * 
 * @note Called from ISR context, must be fast and ISR-safe
 */
typedef void (*Os_TimerCallbackType)(void);

/**
 * @brief Initialize hardware timer
 * 
 * @details
 * Configures ESP32 GP Timer with 1ms period.
 * Registers ISR callback for periodic tick generation.
 * 
 * @return void
 */
void OsTimer_Init(void);

/**
 * @brief Register timer callback
 * 
 * @param[in] callback Function to call on each timer interrupt
 * 
 * @return void
 */
void OsTimer_RegisterCallback(Os_TimerCallbackType callback);

/**
 * @brief Start hardware timer
 * 
 * @return void
 */
void OsTimer_Start(void);

/**
 * @brief Stop hardware timer
 * 
 * @return void
 */
void OsTimer_Stop(void);

/**
 * @brief Get timer status
 * 
 * @return true if timer is running, false otherwise
 */
bool OsTimer_IsRunning(void);

#endif /* OS_TIMER_CFG_H */
