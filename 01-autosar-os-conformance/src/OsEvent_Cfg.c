#include "OsEvent_Cfg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include <stdio.h>

/**
 * @file    OsEvent_Cfg.c
 * @brief   AUTOSAR OS Event Implementation
 *
 * @details
 * Each task that owns events has an OsEvent_TaskContextType entry.
 * A compile-time binding table (OsEvent_Bindings[]) maps every OsEvent_IdType
 * to its owning task-context index and the specific bit it occupies in that
 * context's EventMask.
 *
 * Flow (matching AUTOSAR Classic ECC2):
 *   1. Alarm ISR fires  → calls OsEvent_SetEvent(id)
 *   2. SetEvent         → sets bit in EventMask, notifies task
 *   3. Task wakes       → returns from OsEvent_WaitEvent()
 *   4. Task polls       → OsEvent_IsEventSet(id) for each owned event
 *   5. Task clears      → OsEvent_ClearEvent(id) after processing
 */

/* --------------------------------------------------------------------------
 * Task context array (one entry per task that owns events)
 *   Index 0 → HighPrio task  (owns OsEvent_50ms, OsEvent_200ms)
 *   Index 1 → LowPrio  task  (owns OsEvent_100ms, OsEvent_500ms)
 * -------------------------------------------------------------------------- */
static OsEvent_TaskContextType OsEvent_TaskContexts[OS_EVENT_TASK_COUNT];

/* --------------------------------------------------------------------------
 * Static binding table – maps each OsEvent_IdType to its task-context
 * index and the single bit it occupies.  Order must match OsEvent_IdType.
 * -------------------------------------------------------------------------- */
static const OsEvent_BindingType OsEvent_Bindings[OS_EVENT_COUNT] =
{
    /* OsEvent_50ms  */ { .TaskContextIndex = 0, .BitMask = (1U << 0) },
    /* OsEvent_200ms */ { .TaskContextIndex = 0, .BitMask = (1U << 1) },
    /* OsEvent_100ms */ { .TaskContextIndex = 1, .BitMask = (1U << 0) },
    /* OsEvent_500ms */ { .TaskContextIndex = 1, .BitMask = (1U << 1) }
};

/* Spinlock for protecting EventMask writes from ISR context */
static portMUX_TYPE OsEventMux = portMUX_INITIALIZER_UNLOCKED;

/* --------------------------------------------------------------------------
 * Internal helper – resolve an event ID to its task-context pointer.
 * Returns NULL on invalid ID.
 * -------------------------------------------------------------------------- */
static OsEvent_TaskContextType *OsEvent_GetContext(OsEvent_IdType EventId)
{
    if (EventId >= OS_EVENT_COUNT)
    {
        return NULL;
    }
    return &OsEvent_TaskContexts[OsEvent_Bindings[EventId].TaskContextIndex];
}

/* ==========================================================================
 * Public API
 * ========================================================================== */

/**
 * @brief Initialise the event subsystem
 */
void OsEvent_Init(void)
{
    portENTER_CRITICAL(&OsEventMux);

    for (uint32_t i = 0; i < OS_EVENT_TASK_COUNT; i++)
    {
        OsEvent_TaskContexts[i].TaskHandle = NULL;
        OsEvent_TaskContexts[i].EventMask  = 0U;
    }

    portEXIT_CRITICAL(&OsEventMux);

    printf("[OsEvent] Event subsystem initialized (%u events, %u task contexts)\n",
           OS_EVENT_COUNT, OS_EVENT_TASK_COUNT);
}

/**
 * @brief Bind an event to an owning task
 *
 * @details
 * Multiple events can share the same owning task – the binding table already
 * routes them to the same task-context index, so this function simply stores
 * the task handle once per unique task context.
 */
void OsEvent_BindEvent(OsEvent_IdType EventId, TaskHandle_t TaskHandle)
{
    if (EventId >= OS_EVENT_COUNT || TaskHandle == NULL)
    {
        printf("[OsEvent] ERROR: Invalid EventId %u or NULL TaskHandle\n", EventId);
        return;
    }

    uint8_t ctxIdx = OsEvent_Bindings[EventId].TaskContextIndex;

    portENTER_CRITICAL(&OsEventMux);
    OsEvent_TaskContexts[ctxIdx].TaskHandle = TaskHandle;
    portEXIT_CRITICAL(&OsEventMux);

    printf("[OsEvent] Event %u bound to task context %u\n", EventId, ctxIdx);
}

/**
 * @brief Set an event – ISR-safe
 *
 * @details
 * Sets the event bit and wakes the owning task via task notification.
 * If multiple events fire before the task processes them the bitmask
 * accumulates all flags – the task will see every pending event on its
 * next WaitEvent() return.
 */
void OsEvent_SetEvent(OsEvent_IdType EventId)
{
    if (EventId >= OS_EVENT_COUNT)
    {
        return;
    }

    OsEvent_TaskContextType *ctx = &OsEvent_TaskContexts[OsEvent_Bindings[EventId].TaskContextIndex];
    uint32_t                 bit = OsEvent_Bindings[EventId].BitMask;
    BaseType_t               xHigherPriorityTaskWoken = pdFALSE;

    /* Set the event flag (atomic bit-OR, ISR context) */
    portENTER_CRITICAL_ISR(&OsEventMux);
    ctx->EventMask |= bit;
    portEXIT_CRITICAL_ISR(&OsEventMux);

    /* Wake the owning task (AUTOSAR ActivateTask / SetEvent equivalent) */
    if (ctx->TaskHandle != NULL)
    {
        vTaskNotifyGiveIndexedFromISR(
            ctx->TaskHandle,
            OSEVENT_NOTIFY_INDEX,
            &xHigherPriorityTaskWoken
        );
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief Wait until at least one owned event is set
 *
 * @details
 * Blocks on the task notification channel.  On return, the task must
 * poll each event with IsEventSet() and clear it after handling.
 */
void OsEvent_WaitEvent(void)
{
    /* Block indefinitely until SetEvent issues a notification */
    ulTaskNotifyTakeIndexed(
        OSEVENT_NOTIFY_INDEX,
        pdTRUE,           /* Clear notification count on exit */
        portMAX_DELAY     /* Wait indefinitely */
    );
}

/**
 * @brief Clear a single event flag
 */
void OsEvent_ClearEvent(OsEvent_IdType EventId)
{
    if (EventId >= OS_EVENT_COUNT)
    {
        return;
    }

    OsEvent_TaskContextType *ctx = OsEvent_GetContext(EventId);
    if (ctx == NULL)
    {
        return;
    }

    uint32_t bit = OsEvent_Bindings[EventId].BitMask;

    portENTER_CRITICAL(&OsEventMux);
    ctx->EventMask &= ~bit;
    portEXIT_CRITICAL(&OsEventMux);
}

/**
 * @brief Query whether a single event flag is set
 */
bool OsEvent_IsEventSet(OsEvent_IdType EventId)
{
    if (EventId >= OS_EVENT_COUNT)
    {
        return false;
    }

    OsEvent_TaskContextType *ctx = OsEvent_GetContext(EventId);
    if (ctx == NULL)
    {
        return false;
    }

    bool isSet;

    portENTER_CRITICAL(&OsEventMux);
    isSet = (ctx->EventMask & OsEvent_Bindings[EventId].BitMask) != 0U;
    portEXIT_CRITICAL(&OsEventMux);

    return isSet;
}
