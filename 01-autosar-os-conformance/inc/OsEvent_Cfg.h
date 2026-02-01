#ifndef OSEVENT_CFG_H
#define OSEVENT_CFG_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @file    OsEvent_Cfg.h
 * @brief   AUTOSAR OS Event Configuration and Interface
 *
 * @details
 * Implements the AUTOSAR Classic OS Event mechanism for Extended Tasks (ECC2).
 * Each event is a single bit in a per-task bitmask.  Alarm callbacks call
 * OsEvent_SetEvent() from ISR context to flag an event and wake the owning
 * task.  The task then polls its events with OsEvent_IsEventSet(), executes
 * the associated application code, and clears the event with
 * OsEvent_ClearEvent() — exactly matching the AUTOSAR
 * SetEvent / WaitEvent / ClearEvent / IsEventSet pattern.
 *
 * Event-to-Task mapping (configured at compile time):
 *   OsEvent_50ms   → HighPrio task   (bit 0)
 *   OsEvent_200ms  → HighPrio task   (bit 1)
 *   OsEvent_100ms  → LowPrio  task   (bit 0)
 *   OsEvent_500ms  → LowPrio  task   (bit 1)
 *
 * @note
 * SetEvent is ISR-safe and may be called from alarm callbacks.
 * WaitEvent, ClearEvent and IsEventSet must only be called from task context.
 */

/*
 * -----------------------------------------------------------------------
 * Maximum number of events a single task may own.
 * The bitmask is stored in a uint32_t so the hard limit is 32.
 * -----------------------------------------------------------------------
 */
#define OS_EVENT_MAX_PER_TASK   (32U)

/**
 * @brief Event identifiers
 *
 * @note  Each event must have a unique ID.  The sentinel OS_EVENT_COUNT
 *        must remain last.
 */
typedef enum
{
    OsEvent_50ms  = 0,  /**< Event triggered by the  50 ms alarm (HighPrio) */
    OsEvent_200ms,      /**< Event triggered by the 200 ms alarm (HighPrio) */
    OsEvent_100ms,      /**< Event triggered by the 100 ms alarm (LowPrio)  */
    OsEvent_500ms,      /**< Event triggered by the 500 ms alarm (LowPrio)  */
    OS_EVENT_COUNT      /**< Total number of events – must be last          */
} OsEvent_IdType;

/**
 * @brief Per-task event descriptor (internal – used by the implementation)
 *
 * @details
 * Each owning task has one of these.  The bitmask accumulates set-event
 * flags; the task handle is used to issue the notification that wakes
 * the task out of WaitEvent().
 */
typedef struct
{
    TaskHandle_t        TaskHandle;     /**< FreeRTOS handle of owning task */
    volatile uint32_t   EventMask;      /**< Bitmask of pending events      */
} OsEvent_TaskContextType;

/**
 * @brief Static binding of an event ID to its owning task context and bit position
 */
typedef struct
{
    uint8_t     TaskContextIndex;   /**< Index into the task-context array */
    uint32_t    BitMask;            /**< Bit corresponding to this event  */
} OsEvent_BindingType;

/*
 * -----------------------------------------------------------------------
 * Number of distinct task contexts that own events.
 * HighPrio = index 0, LowPrio = index 1.
 * -----------------------------------------------------------------------
 */
#define OS_EVENT_TASK_COUNT     (2U)

/*
 * -----------------------------------------------------------------------
 * Notification index used for event wake-ups (same channel as alarm
 * notifications so the task only has one blocking point).
 * -----------------------------------------------------------------------
 */
#define OSEVENT_NOTIFY_INDEX    (0U)

/*
 * =======================================================================
 * Public API – called by Os.c (task / ISR context as noted)
 * =======================================================================
 */

/**
 * @brief Initialise the event subsystem
 *
 * @details
 * Clears all event bitmasks and NULLs all task handles.
 * Must be called before any other OsEvent function.
 * Called once during StartOS().
 */
void OsEvent_Init(void);

/**
 * @brief Bind an event to an owning task
 *
 * @param[in] EventId       Event identifier
 * @param[in] TaskHandle    FreeRTOS handle of the owning task
 *
 * @details
 * Associates EventId with the given task.  Multiple events can be bound
 * to the same task – they share the same bitmask and notification channel.
 * Must be called after xTaskCreate() and before OsTimer_Start().
 *
 * @note Not ISR-safe – call from task context only (during StartOS).
 */
void OsEvent_BindEvent(OsEvent_IdType EventId, TaskHandle_t TaskHandle);

/**
 * @brief Set an event (ISR-safe)
 *
 * @param[in] EventId   Event identifier
 *
 * @details
 * Sets the event flag in the owning task's bitmask and issues a task
 * notification to wake the task if it is blocked in WaitEvent().
 * Conforms to AUTOSAR OS SetEvent() service.
 *
 * @note MUST be called from ISR context (alarm callback).
 *       Uses vTaskNotifyGiveIndexedFromISR internally.
 */
void OsEvent_SetEvent(OsEvent_IdType EventId);

/**
 * @brief Wait until at least one event in the calling task is set
 *
 * @details
 * Blocks the calling task indefinitely until OsEvent_SetEvent() is called
 * for any event bound to that task.  After returning the task should poll
 * its events individually with OsEvent_IsEventSet().
 * Conforms to AUTOSAR OS WaitEvent() service.
 *
 * @note Must be called from task context only.
 */
void OsEvent_WaitEvent(void);

/**
 * @brief Clear a single event flag
 *
 * @param[in] EventId   Event identifier
 *
 * @details
 * Clears the event flag so that OsEvent_IsEventSet() returns false for
 * that event until the next SetEvent call.
 * Conforms to AUTOSAR OS ClearEvent() service.
 *
 * @note Must be called from the owning task context only.
 */
void OsEvent_ClearEvent(OsEvent_IdType EventId);

/**
 * @brief Query whether a single event flag is currently set
 *
 * @param[in] EventId   Event identifier
 *
 * @return  true  if the event flag is set, false otherwise
 *
 * @details
 * Conforms to AUTOSAR OS IsEventSet() (extension utility).
 *
 * @note Must be called from the owning task context only.
 */
bool OsEvent_IsEventSet(OsEvent_IdType EventId);

#endif /* OSEVENT_CFG_H */
