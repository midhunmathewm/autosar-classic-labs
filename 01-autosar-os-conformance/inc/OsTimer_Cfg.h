#ifndef OSTIMER_CFG_H
#define OSTIMER_CFG_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file    OsTimer_Cfg.h
 * @brief   AUTOSAR OS Timer Configuration
 * 
 * @details
 * Hardware timer abstraction for ESP32.
 * Provides 1ms tick for system counter.
 */

/***************************************************************************
 * Configuration Parameters
 ***************************************************************************/

/**
 * @brief Timer resolution in Hz (1 MHz = 1 µs tick)
 */
#define OS_TIMER_RESOLUTION_HZ  (1000000U)

/**
 * @brief Timer period in microseconds (1000 µs = 1 ms)
 */
#define OS_TIMER_PERIOD_US      (1000U)

/**
 * @brief Timer callback function type
 * 
 * @note Called from ISR context every 1ms
 */
typedef void (*Os_TimerCallbackType)(void);

/***************************************************************************
 * Function Prototypes
 ***************************************************************************/

/**
 * @brief Initialize hardware timer
 * 
 * @details
 * - Configures ESP32 GP Timer with 1 MHz resolution
 * - Sets alarm at 1000 µs (1 ms period)
 * - Enables auto-reload for continuous operation
 */
void OsTimer_Init(void);

/**
 * @brief Register timer callback
 * 
 * @param[in] callback Function to call on each timer interrupt
 * 
 * @details
 * Registers a callback that will be executed from ISR context
 * every 1ms. The callback typically increments the system counter.
 */
void OsTimer_RegisterCallback(Os_TimerCallbackType callback);

/**
 * @brief Start hardware timer
 * 
 * @details
 * Starts the hardware timer. Timer begins generating 1ms interrupts.
 */
void OsTimer_Start(void);

/**
 * @brief Stop hardware timer
 * 
 * @details
 * Stops the hardware timer. No more interrupts will be generated.
 */
void OsTimer_Stop(void);

/**
 * @brief Get timer running status
 * 
 * @return true if timer is running, false otherwise
 */
bool OsTimer_IsRunning(void);

#endif /* OSTIMER_CFG_H */
