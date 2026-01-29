#ifndef OS_ALARM_CFG_H
#define OS_ALARM_CFG_H

#include <stdint.h>
#include <stdbool.h>
#include "OsCounter_Cfg.h"

/**
 * @file    OsAlarm_Cfg.h
 * @brief   AUTOSAR OS Alarm Configuration Interface
 *
 * @details
 * Provides alarm management similar to AUTOSAR OS.
 * Alarms can be cyclic or one-shot and trigger callbacks.
 */

/**
 * @brief Alarm callback type
 * 
 * @note Called from ISR context when alarm expires
 */
typedef void (*OsAlarm_CallbackType)(void);

/**
 * @brief Alarm identifiers
 */
typedef enum
{
    OsAlarm_200ms = 0,      /**< 200ms periodic alarm */
    OsAlarm_1000ms,         /**< 1000ms periodic alarm */
    OS_ALARM_COUNT          /**< Number of configured alarms */
} OsAlarm_IdType;

/**
 * @brief Alarm configuration structure
 */
typedef struct
{
    OsCounter_IdType        CounterId;      /**< Associated counter */
    uint32_t                AlarmTime;      /**< Expiration time (ticks) */
    uint32_t                CycleTime;      /**< Cycle period (0 = one-shot) */
    bool                    IsActive;       /**< Alarm active status */
    OsAlarm_CallbackType    Callback;       /**< Expiration callback */
} OsAlarm_ConfigType;

/**
 * @brief Initialize alarm subsystem
 * 
 * @return void
 */
void OsAlarm_Init(void);

/**
 * @brief Set a relative alarm
 * 
 * @details
 * Configures an alarm to expire after 'Increment' ticks from now.
 * If 'Cycle' > 0, the alarm automatically rearms with that period.
 * 
 * @param[in] AlarmId Alarm identifier
 * @param[in] Increment Ticks until first expiration (relative to current counter)
 * @param[in] Cycle Period for cyclic alarms (0 for one-shot)
 * 
 * @return void
 */
void OsAlarm_SetRel(OsAlarm_IdType AlarmId, uint32_t Increment, uint32_t Cycle);

/**
 * @brief Set an absolute alarm
 * 
 * @details
 * Configures an alarm to expire at an absolute counter value.
 * 
 * @param[in] AlarmId Alarm identifier
 * @param[in] Start Absolute counter value for expiration
 * @param[in] Cycle Period for cyclic alarms (0 for one-shot)
 * 
 * @return void
 */
void OsAlarm_SetAbs(OsAlarm_IdType AlarmId, uint32_t Start, uint32_t Cycle);

/**
 * @brief Cancel an active alarm
 * 
 * @param[in] AlarmId Alarm identifier
 * 
 * @return void
 */
void OsAlarm_Cancel(OsAlarm_IdType AlarmId);

/**
 * @brief Check if alarm is active
 * 
 * @param[in] AlarmId Alarm identifier
 * 
 * @return true if alarm is active, false otherwise
 */
bool OsAlarm_IsActive(OsAlarm_IdType AlarmId);

/**
 * @brief Get alarm time
 * 
 * @param[in] AlarmId Alarm identifier
 * @param[out] Tick Pointer to store alarm expiration time
 * 
 * @return void
 */
void OsAlarm_GetAlarmTime(OsAlarm_IdType AlarmId, uint32_t *Tick);

/**
 * @brief Register alarm callback
 * 
 * @param[in] AlarmId Alarm identifier
 * @param[in] Callback Function to call when alarm expires
 * 
 * @return void
 */
void OsAlarm_RegisterCallback(OsAlarm_IdType AlarmId, OsAlarm_CallbackType Callback);

/**
 * @brief Process alarms for a counter (called from counter increment)
 * 
 * @param[in] CounterId Counter that was incremented
 * 
 * @return void
 * 
 * @note This is called from ISR context by OsCounter_Increment
 */
void OsAlarm_ProcessCounter(OsCounter_IdType CounterId);

#endif /* OS_ALARM_CFG_H */
