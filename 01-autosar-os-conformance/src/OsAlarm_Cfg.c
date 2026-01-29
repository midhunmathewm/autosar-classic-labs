#include "OsAlarm_Cfg.h"
#include "OsCounter_Cfg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include <stdio.h>

/**
 * @file    OsAlarm_Cfg.c
 * @brief   AUTOSAR OS Alarm Implementation
 */

/* Alarm configurations */
static OsAlarm_ConfigType OsAlarm_Configs[OS_ALARM_COUNT];

/* Critical section protection */
static portMUX_TYPE OsAlarmMux = portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief Initialize alarm subsystem
 * 
 * @details
 * Resets all alarm configurations to inactive state.
 */
void OsAlarm_Init(void)
{
    portENTER_CRITICAL(&OsAlarmMux);
    
    for (uint32_t i = 0; i < OS_ALARM_COUNT; i++)
    {
        OsAlarm_Configs[i].CounterId = OsCounter_System;
        OsAlarm_Configs[i].AlarmTime = 0U;
        OsAlarm_Configs[i].CycleTime = 0U;
        OsAlarm_Configs[i].IsActive = false;
        OsAlarm_Configs[i].Callback = NULL;
    }
    
    portEXIT_CRITICAL(&OsAlarmMux);
    
    printf("[OsAlarm] Alarm subsystem initialized (%u alarms)\n", OS_ALARM_COUNT);
}

/**
 * @brief Set a relative alarm
 * 
 * @param[in] AlarmId Alarm identifier
 * @param[in] Increment Ticks until first expiration
 * @param[in] Cycle Period for cyclic alarms (0 for one-shot)
 */
void OsAlarm_SetRel(OsAlarm_IdType AlarmId, uint32_t Increment, uint32_t Cycle)
{
    if (AlarmId >= OS_ALARM_COUNT)
    {
        printf("[OsAlarm] ERROR: Invalid alarm ID %u\n", AlarmId);
        return;
    }
    
    if (Increment < OS_COUNTER_MIN_CYCLE)
    {
        printf("[OsAlarm] ERROR: Increment %lu less than minimum cycle %u\n", 
               Increment, OS_COUNTER_MIN_CYCLE);
        return;
    }
    
    portENTER_CRITICAL(&OsAlarmMux);
    
    /* Get current counter value */
    uint32_t currentCount = OsCounter_GetValue(OsAlarm_Configs[AlarmId].CounterId);
    
    /* Calculate absolute alarm time */
    uint32_t alarmTime = (currentCount + Increment) % (OS_COUNTER_MAX_ALLOWED + 1);
    
    /* Configure alarm */
    OsAlarm_Configs[AlarmId].AlarmTime = alarmTime;
    OsAlarm_Configs[AlarmId].CycleTime = Cycle;
    OsAlarm_Configs[AlarmId].IsActive = true;
    
    portEXIT_CRITICAL(&OsAlarmMux);
    
    printf("[OsAlarm] Alarm %u set (relative): current=%lu, trigger=%lu, cycle=%lu\n",
           AlarmId, currentCount, alarmTime, Cycle);
}

/**
 * @brief Set an absolute alarm
 * 
 * @param[in] AlarmId Alarm identifier
 * @param[in] Start Absolute counter value for expiration
 * @param[in] Cycle Period for cyclic alarms (0 for one-shot)
 */
void OsAlarm_SetAbs(OsAlarm_IdType AlarmId, uint32_t Start, uint32_t Cycle)
{
    if (AlarmId >= OS_ALARM_COUNT)
    {
        printf("[OsAlarm] ERROR: Invalid alarm ID %u\n", AlarmId);
        return;
    }
    
    if (Start > OS_COUNTER_MAX_ALLOWED)
    {
        printf("[OsAlarm] ERROR: Start value %lu exceeds max %u\n", 
               Start, OS_COUNTER_MAX_ALLOWED);
        return;
    }
    
    portENTER_CRITICAL(&OsAlarmMux);
    
    /* Configure alarm */
    OsAlarm_Configs[AlarmId].AlarmTime = Start;
    OsAlarm_Configs[AlarmId].CycleTime = Cycle;
    OsAlarm_Configs[AlarmId].IsActive = true;
    
    portEXIT_CRITICAL(&OsAlarmMux);
    
    printf("[OsAlarm] Alarm %u set (absolute): trigger=%lu, cycle=%lu\n",
           AlarmId, Start, Cycle);
}

/**
 * @brief Cancel an active alarm
 * 
 * @param[in] AlarmId Alarm identifier
 */
void OsAlarm_Cancel(OsAlarm_IdType AlarmId)
{
    if (AlarmId >= OS_ALARM_COUNT)
    {
        printf("[OsAlarm] ERROR: Invalid alarm ID %u\n", AlarmId);
        return;
    }
    
    portENTER_CRITICAL(&OsAlarmMux);
    OsAlarm_Configs[AlarmId].IsActive = false;
    portEXIT_CRITICAL(&OsAlarmMux);
    
    printf("[OsAlarm] Alarm %u cancelled\n", AlarmId);
}

/**
 * @brief Check if alarm is active
 * 
 * @param[in] AlarmId Alarm identifier
 * 
 * @return true if alarm is active, false otherwise
 */
bool OsAlarm_IsActive(OsAlarm_IdType AlarmId)
{
    if (AlarmId >= OS_ALARM_COUNT)
    {
        return false;
    }
    
    bool isActive;
    
    portENTER_CRITICAL(&OsAlarmMux);
    isActive = OsAlarm_Configs[AlarmId].IsActive;
    portEXIT_CRITICAL(&OsAlarmMux);
    
    return isActive;
}

/**
 * @brief Get alarm time
 * 
 * @param[in] AlarmId Alarm identifier
 * @param[out] Tick Pointer to store alarm expiration time
 */
void OsAlarm_GetAlarmTime(OsAlarm_IdType AlarmId, uint32_t *Tick)
{
    if (AlarmId >= OS_ALARM_COUNT || Tick == NULL)
    {
        return;
    }
    
    portENTER_CRITICAL(&OsAlarmMux);
    *Tick = OsAlarm_Configs[AlarmId].AlarmTime;
    portEXIT_CRITICAL(&OsAlarmMux);
}

/**
 * @brief Register alarm callback
 * 
 * @param[in] AlarmId Alarm identifier
 * @param[in] Callback Function to call when alarm expires
 */
void OsAlarm_RegisterCallback(OsAlarm_IdType AlarmId, OsAlarm_CallbackType Callback)
{
    if (AlarmId >= OS_ALARM_COUNT)
    {
        printf("[OsAlarm] ERROR: Invalid alarm ID %u\n", AlarmId);
        return;
    }
    
    portENTER_CRITICAL(&OsAlarmMux);
    OsAlarm_Configs[AlarmId].Callback = Callback;
    portEXIT_CRITICAL(&OsAlarmMux);
    
    printf("[OsAlarm] Callback registered for alarm %u\n", AlarmId);
}

/**
 * @brief Process alarms for a counter
 * 
 * @details
 * Called from ISR context when a counter is incremented.
 * Checks all alarms associated with the counter and triggers
 * callbacks for expired alarms.
 * 
 * @param[in] CounterId Counter that was incremented
 * 
 * @note This function is called from ISR context (by OsCounter_Increment)
 */
void OsAlarm_ProcessCounter(OsCounter_IdType CounterId)
{
    /* Get current counter value */
    uint32_t currentCount = OsCounter_GetValue(CounterId);
    
    portENTER_CRITICAL_ISR(&OsAlarmMux);
    
    /* Check all alarms */
    for (uint32_t i = 0; i < OS_ALARM_COUNT; i++)
    {
        /* Skip inactive alarms or alarms for different counters */
        if (!OsAlarm_Configs[i].IsActive || 
            OsAlarm_Configs[i].CounterId != CounterId)
        {
            continue;
        }
        
        /* Check if alarm expired */
        if (OsAlarm_Configs[i].AlarmTime == currentCount)
        {
            /* Trigger callback if registered */
            if (OsAlarm_Configs[i].Callback != NULL)
            {
                /* Call callback from ISR context */
                OsAlarm_Configs[i].Callback();
            }
            
            /* Handle cyclic alarm */
            if (OsAlarm_Configs[i].CycleTime > 0)
            {
                /* Rearm alarm for next cycle */
                OsAlarm_Configs[i].AlarmTime = 
                    (currentCount + OsAlarm_Configs[i].CycleTime) % 
                    (OS_COUNTER_MAX_ALLOWED + 1);
            }
            else
            {
                /* One-shot alarm - deactivate */
                OsAlarm_Configs[i].IsActive = false;
            }
        }
    }
    
    portEXIT_CRITICAL_ISR(&OsAlarmMux);
}
