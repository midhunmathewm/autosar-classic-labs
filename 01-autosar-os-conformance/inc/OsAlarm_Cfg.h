#ifndef OSALARM_CFG_H
#define OSALARM_CFG_H

#include <stdint.h>
#include <stdbool.h>
#include "OsCounter_Cfg.h"

/**
 * @file    OsAlarm_Cfg.h
 * @brief   AUTOSAR OS Alarm Configuration
 * 
 * @details
 * Implements AUTOSAR Classic OS Alarm mechanism.
 * Alarms are used to trigger periodic or one-shot events
 * based on counter values.
 */

/**
 * @brief Alarm identifiers
 * 
 * @note Each alarm must have a unique ID
 */
typedef enum
{
    OsAlarm_Task200ms = 0,  /**< Alarm for 200ms periodic task */
    OsAlarm_Task1000ms,     /**< Alarm for 1000ms periodic task */
    OS_ALARM_COUNT          /**< Total number of alarms */
} OsAlarm_IdType;

/**
 * @brief Alarm callback function type
 * 
 * @note Callbacks are executed from ISR context
 *       Must be short and ISR-safe
 */
typedef void (*OsAlarm_CallbackType)(void);

/**
 * @brief Alarm configuration structure
 */
typedef struct
{
    OsCounter_IdType CounterId;        /**< Associated counter */
    uint32_t AlarmTime;                /**< Absolute expiration time */
    uint32_t CycleTime;                /**< Cycle time (0 for one-shot) */
    bool IsActive;                     /**< Alarm active state */
    OsAlarm_CallbackType Callback;     /**< Expiration callback */
} OsAlarm_ConfigType;

/***************************************************************************
 * Function Prototypes
 ***************************************************************************/

/**
 * @brief Initialize alarm subsystem
 * 
 * @details
 * Resets all alarm configurations to inactive state.
 * Must be called before using any alarm services.
 * 
 * @startuml
 * start
 * :Reset all alarms;
 * :Set IsActive = false;
 * :Clear callbacks;
 * stop
 * @enduml
 */
void OsAlarm_Init(void);

/**
 * @brief Set a relative alarm
 * 
 * @param[in] AlarmId Alarm identifier
 * @param[in] Increment Ticks until first expiration (relative to current counter)
 * @param[in] Cycle Period for cyclic alarms (0 for one-shot)
 * 
 * @details
 * Sets an alarm to expire after 'Increment' ticks from current counter value.
 * If Cycle > 0, alarm automatically rearms after each expiration.
 * 
 * Example:
 * @code
 * // Set alarm to trigger in 200 ticks, then every 200 ticks
 * OsAlarm_SetRel(OsAlarm_Task200ms, 200, 200);
 * @endcode
 * 
 * @note Conforms to AUTOSAR OS SetRelAlarm() service
 */
void OsAlarm_SetRel(OsAlarm_IdType AlarmId, uint32_t Increment, uint32_t Cycle);

/**
 * @brief Set an absolute alarm
 * 
 * @param[in] AlarmId Alarm identifier
 * @param[in] Start Absolute counter value for expiration
 * @param[in] Cycle Period for cyclic alarms (0 for one-shot)
 * 
 * @details
 * Sets an alarm to expire when counter reaches 'Start' value.
 * If Cycle > 0, alarm automatically rearms after each expiration.
 * 
 * @note Conforms to AUTOSAR OS SetAbsAlarm() service
 */
void OsAlarm_SetAbs(OsAlarm_IdType AlarmId, uint32_t Start, uint32_t Cycle);

/**
 * @brief Cancel an active alarm
 * 
 * @param[in] AlarmId Alarm identifier
 * 
 * @details
 * Deactivates the specified alarm. No callback will be triggered.
 * 
 * @note Conforms to AUTOSAR OS CancelAlarm() service
 */
void OsAlarm_Cancel(OsAlarm_IdType AlarmId);

/**
 * @brief Check if alarm is active
 * 
 * @param[in] AlarmId Alarm identifier
 * 
 * @return true if alarm is active, false otherwise
 * 
 * @note Similar to AUTOSAR OS GetAlarm() service
 */
bool OsAlarm_IsActive(OsAlarm_IdType AlarmId);

/**
 * @brief Get alarm expiration time
 * 
 * @param[in] AlarmId Alarm identifier
 * @param[out] Tick Pointer to store alarm expiration time
 * 
 * @details
 * Returns the absolute counter value when alarm will expire.
 * 
 * @note Conforms to AUTOSAR OS GetAlarm() service
 */
void OsAlarm_GetAlarmTime(OsAlarm_IdType AlarmId, uint32_t *Tick);

/**
 * @brief Register alarm callback function
 * 
 * @param[in] AlarmId Alarm identifier
 * @param[in] Callback Function to call when alarm expires
 * 
 * @details
 * Registers a callback that will be executed from ISR context
 * when the alarm expires. Callback must be short and ISR-safe.
 */
void OsAlarm_RegisterCallback(OsAlarm_IdType AlarmId, OsAlarm_CallbackType Callback);

/**
 * @brief Process alarms for a counter (ISR context)
 * 
 * @param[in] CounterId Counter that was incremented
 * 
 * @details
 * Called automatically by counter increment function.
 * Checks all alarms and triggers callbacks for expired alarms.
 * 
 * @note This function is called from ISR context
 */
void OsAlarm_ProcessCounter(OsCounter_IdType CounterId);

#endif /* OSALARM_CFG_H */
