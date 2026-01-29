#ifndef OS_H
#define OS_H

#include <stdint.h>
/**
 * @file    Os.h
 * @brief   AUTOSAR OS Interface
 *
 * @details
 * Main OS abstraction layer interface.
 * Provides AUTOSAR-style OS services on ESP32/FreeRTOS.
 */

/**
 * @brief Application startup mode
 */
typedef enum
{
    OSDEFAULTAPPMODE = 0  /**< Default application mode */
} AppModeType;

/**
 * @brief   Starts the OS abstraction layer
 *
 * @details
 * Initializes BSW layer components:
 * - Timer subsystem
 * - Counter subsystem
 * - Alarm subsystem
 * - Application tasks
 * 
 * @param[in] mode  Application mode
 *
 * @return  None
 * 
 * @note The FreeRTOS scheduler is already running on ESP-IDF.
 *       This function only initializes BSW and creates tasks.
 */
void StartOS(AppModeType mode);

/**
 * @brief Get OS tick count (system counter value)
 * 
 * @return Current system tick count (1 tick = 1 ms)
 */
uint32_t Os_GetTickCount(void);

/**
 * @brief Delay execution for specified ticks
 * 
 * @param[in] ticks Number of ticks to delay (1 tick = 1 ms)
 * 
 * @return void
 */
void Os_Delay(uint32_t ticks);

#endif /* OS_H */
