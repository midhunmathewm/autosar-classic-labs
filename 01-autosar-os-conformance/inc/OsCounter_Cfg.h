#ifndef OS_COUNTER_CFG_H
#define OS_COUNTER_CFG_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file    OsCounter_Cfg.h
 * @brief   AUTOSAR OS Counter Configuration Interface
 *
 * @details
 * Provides counter management similar to AUTOSAR OS.
 * Counters are incremented by timer ISR or software.
 */

/* AUTOSAR OS counter attributes */
#define OS_COUNTER_MAX_ALLOWED   (100000U)   /* Maximum counter value (wrap point) */
#define OS_COUNTER_MIN_CYCLE     (1U)        /* Minimum cycle time */
#define OS_COUNTER_TICKS_PER_BASE (1U)       /* Ticks per base unit */

/**
 * @brief Counter identifiers
 */
typedef enum
{
    OsCounter_System = 0,   /**< System counter (1ms tick) */
    OS_COUNTER_COUNT        /**< Number of configured counters */
} OsCounter_IdType;

/**
 * @brief Initialize counter subsystem
 * 
 * @details
 * Resets all counters to zero and initializes internal state.
 * 
 * @return void
 */
void OsCounter_Init(void);

/**
 * @brief Increment a counter
 * 
 * @details
 * Increments the specified counter by 1.
 * Handles wrap-around at OS_COUNTER_MAX_ALLOWED.
 * Thread-safe, can be called from ISR context.
 * 
 * @param[in] CounterId Counter to increment
 * 
 * @return void
 * 
 * @note Called from timer ISR for system counter
 */
void OsCounter_Increment(OsCounter_IdType CounterId);

/**
 * @brief Get current counter value
 * 
 * @details
 * Thread-safe read of counter value.
 * 
 * @param[in] CounterId Counter to read
 * 
 * @return Current counter value
 */
uint32_t OsCounter_GetValue(OsCounter_IdType CounterId);

/**
 * @brief Set counter value (for testing/debugging)
 * 
 * @param[in] CounterId Counter to set
 * @param[in] Value New counter value
 * 
 * @return void
 */
void OsCounter_SetValue(OsCounter_IdType CounterId, uint32_t Value);

#endif /* OS_COUNTER_CFG_H */
