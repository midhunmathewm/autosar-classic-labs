#ifndef OS_COUNTER_CFG_H
#define OS_COUNTER_CFG_H

#include <stdint.h>

/* AUTOSAR OS counter attributes */
#define OS_COUNTER_MAX_ALLOWED   (100000U)   /* wrap value */
#define OS_COUNTER_MIN_CYCLE     (1U)
#define OS_COUNTER_TICKS_PER_BASE (1U)

/* Counter IDs */
typedef enum
{
    OsCounter_System = 0,
    OS_COUNTER_COUNT
} Os_CounterIdType;


void Os_IncrementCounter(Os_CounterIdType CounterId);
uint32_t GetCounterValue(Os_CounterIdType CounterId);
void Os_Timer_Init_1ms(void);

#endif
