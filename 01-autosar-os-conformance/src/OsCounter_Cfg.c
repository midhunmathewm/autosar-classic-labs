#include "OsCounter_Cfg.h"
#include "OsAlarm_Cfg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include <stdio.h>

/**
 * @file    OsCounter_Cfg.c
 * @brief   AUTOSAR OS Counter Implementation
 */

/* Counter values */
static volatile uint32_t OsCounter_Values[OS_COUNTER_COUNT];

/* Critical section protection */
static portMUX_TYPE OsCounterMux = portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief Initialize counter subsystem
 * 
 * @details
 * Resets all counters to zero.
 */
void OsCounter_Init(void)
{
    portENTER_CRITICAL(&OsCounterMux);
    
    for (uint32_t i = 0; i < OS_COUNTER_COUNT; i++)
    {
        OsCounter_Values[i] = 0U;
    }
    
    portEXIT_CRITICAL(&OsCounterMux);
    
    printf("[OsCounter] Counter subsystem initialized (%u counters)\n", 
           OS_COUNTER_COUNT);
}

/**
 * @brief Increment a counter
 * 
 * @details
 * - Increments counter by 1
 * - Handles wrap-around at OS_COUNTER_MAX_ALLOWED
 * - Triggers alarm processing for this counter
 * - Thread-safe (can be called from ISR)
 * 
 * @param[in] CounterId Counter to increment
 */
void OsCounter_Increment(OsCounter_IdType CounterId)
{
    if (CounterId >= OS_COUNTER_COUNT)
    {
        return;
    }
    
    portENTER_CRITICAL_ISR(&OsCounterMux);
    
    /* Increment counter */
    uint32_t next = OsCounter_Values[CounterId] + 1U;
    
    /* Handle wrap-around */
    if (next > OS_COUNTER_MAX_ALLOWED)
    {
        next = 0U;
    }
    
    OsCounter_Values[CounterId] = next;
    
    portEXIT_CRITICAL_ISR(&OsCounterMux);
    
    /* Process alarms associated with this counter */
    OsAlarm_ProcessCounter(CounterId);
}

/**
 * @brief Get current counter value
 * 
 * @param[in] CounterId Counter to read
 * 
 * @return Current counter value
 */
uint32_t OsCounter_GetValue(OsCounter_IdType CounterId)
{
    if (CounterId >= OS_COUNTER_COUNT)
    {
        return 0U;
    }
    
    uint32_t value;
    
    portENTER_CRITICAL(&OsCounterMux);
    value = OsCounter_Values[CounterId];
    portEXIT_CRITICAL(&OsCounterMux);
    
    return value;
}

/**
 * @brief Set counter value (for testing/debugging)
 * 
 * @param[in] CounterId Counter to set
 * @param[in] Value New counter value
 */
void OsCounter_SetValue(OsCounter_IdType CounterId, uint32_t Value)
{
    if (CounterId >= OS_COUNTER_COUNT)
    {
        return;
    }
    
    portENTER_CRITICAL(&OsCounterMux);
    
    /* Limit to maximum allowed value */
    if (Value > OS_COUNTER_MAX_ALLOWED)
    {
        Value = OS_COUNTER_MAX_ALLOWED;
    }
    
    OsCounter_Values[CounterId] = Value;
    
    portEXIT_CRITICAL(&OsCounterMux);
    
    printf("[OsCounter] Counter %u set to %lu\n", CounterId, Value);
}
