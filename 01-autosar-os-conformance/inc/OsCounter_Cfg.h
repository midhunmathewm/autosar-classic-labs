#ifndef OSCOUNTER_CFG_H
#define OSCOUNTER_CFG_H

#include <stdint.h>

/**
 * @file    OsCounter_Cfg.h
 * @brief   AUTOSAR OS Counter Configuration
 * 
 * @details
 * Implements AUTOSAR Classic OS Counter mechanism.
 * Counters are used to track system time and trigger alarms.
 */

/***************************************************************************
 * Configuration Parameters
 ***************************************************************************/

/**
 * @brief Maximum allowed counter value
 * 
 * @details
 * Counter wraps to 0 after reaching this value.
 * Conforms to AUTOSAR OsCounterMaxAllowedValue parameter.
 */
#define OS_COUNTER_MAX_ALLOWED      (100000U)

/**
 * @brief Minimum cycle time
 * 
 * @details
 * Minimum number of ticks between alarm expirations.
 * Conforms to AUTOSAR OsCounterMinCycle parameter.
 */
#define OS_COUNTER_MIN_CYCLE        (1U)

/**
 * @brief Ticks per base
 * 
 * @details
 * Number of counter ticks per base unit (1 tick = 1 ms).
 * Conforms to AUTOSAR OsCounterTicksPerBase parameter.
 */
#define OS_COUNTER_TICKS_PER_BASE   (1U)

/**
 * @brief Counter type
 * 
 * @details
 * Hardware or Software counter.
 * In this implementation, all counters are software counters
 * driven by a hardware timer.
 */
typedef enum
{
    COUNTER_HARDWARE = 0,  /**< Hardware-driven counter */
    COUNTER_SOFTWARE       /**< Software counter */
} OsCounter_TypeType;

/**
 * @brief Counter identifiers
 * 
 * @note Each counter must have a unique ID
 */
typedef enum
{
    OsCounter_System = 0,   /**< System counter (1ms tick) */
    OS_COUNTER_COUNT        /**< Total number of counters */
} OsCounter_IdType;

/***************************************************************************
 * Function Prototypes
 ***************************************************************************/

/**
 * @brief Initialize counter subsystem
 * 
 * @details
 * Resets all counters to zero.
 * Must be called before using any counter services.
 */
void OsCounter_Init(void);

/**
 * @brief Increment a counter (ISR context)
 * 
 * @param[in] CounterId Counter to increment
 * 
 * @details
 * - Increments counter by 1
 * - Handles wrap-around at OS_COUNTER_MAX_ALLOWED
 * - Triggers alarm processing for this counter
 * - Thread-safe (can be called from ISR)
 * 
 * @note Conforms to AUTOSAR OS IncrementCounter() service
 */
void OsCounter_Increment(OsCounter_IdType CounterId);

/**
 * @brief Get current counter value
 * 
 * @param[in] CounterId Counter to read
 * 
 * @return Current counter value
 * 
 * @note Conforms to AUTOSAR OS GetCounterValue() service
 */
uint32_t OsCounter_GetValue(OsCounter_IdType CounterId);

/**
 * @brief Set counter value (for testing/debugging)
 * 
 * @param[in] CounterId Counter to set
 * @param[in] Value New counter value
 * 
 * @warning Not part of AUTOSAR specification
 *          Use only for testing and debugging purposes
 */
void OsCounter_SetValue(OsCounter_IdType CounterId, uint32_t Value);

#endif /* OSCOUNTER_CFG_H */
