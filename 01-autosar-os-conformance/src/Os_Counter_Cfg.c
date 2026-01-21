#include "Os_Counter_Cfg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "driver/gptimer.h"


static gptimer_handle_t osTimer;

static volatile uint32_t Os_CounterValue[OS_COUNTER_COUNT];
static portMUX_TYPE OsCounterMux = portMUX_INITIALIZER_UNLOCKED;

void Os_Counter_Init(void)
{
    portENTER_CRITICAL(&OsCounterMux);
    for (uint32_t i = 0; i < OS_COUNTER_COUNT; i++)
    {
        Os_CounterValue[i] = 0U;
    }
    portEXIT_CRITICAL(&OsCounterMux);
}


void Os_IncrementCounter(Os_CounterIdType CounterId)
{
    portENTER_CRITICAL_ISR(&OsCounterMux);

    uint32_t next = Os_CounterValue[CounterId] + 1U;

    if (next > OS_COUNTER_MAX_ALLOWED)
    {
        next = 0U;
    }

    Os_CounterValue[CounterId] = next;

    /* Alarm evaluation will be added here later */

    portEXIT_CRITICAL_ISR(&OsCounterMux);
}

uint32_t GetCounterValue(Os_CounterIdType CounterId)
{
    uint32_t value;

    portENTER_CRITICAL(&OsCounterMux);
    value = Os_CounterValue[CounterId];
    portEXIT_CRITICAL(&OsCounterMux);

    return value;
}

/* ISR callback */
static bool IRAM_ATTR Os_TimerISR(
    gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *user_ctx)
{
    Os_IncrementCounter(OsCounter_System);
    return false;
}

void Os_Timer_Init_1ms(void)
{
    gptimer_config_t cfg = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000  /* 1 µs resolution */
    };

    gptimer_new_timer(&cfg, &osTimer);

    gptimer_alarm_config_t alarm_cfg = {
        .alarm_count = 1000,     /* 1 ms */
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true
    };

    gptimer_set_alarm_action(osTimer, &alarm_cfg);

    gptimer_event_callbacks_t cbs = {
        .on_alarm = Os_TimerISR
    };

    gptimer_register_event_callbacks(osTimer, &cbs, NULL);

    gptimer_enable(osTimer);
    gptimer_start(osTimer);
}

