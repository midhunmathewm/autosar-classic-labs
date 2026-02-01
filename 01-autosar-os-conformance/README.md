# AUTOSAR-Style OS Layer for ESP32

## Overview

This implementation provides an AUTOSAR Classic OS-compliant abstraction layer running on ESP32 with FreeRTOS. It features **event-based task activation** following the AUTOSAR Classic OS Extended Task (ECC2) specification, with modular architecture and separated Timer, Counter, Alarm, and Event management.

Tasks are no longer activated directly by alarm callbacks. Instead, alarms trigger **events** (SetEvent), and tasks poll their events (IsEventSet), execute application code, and clear them (ClearEvent) — the standard AUTOSAR Classic ECC2 pattern.

## Features

- ✅ **Hardware Timer** (1ms tick using ESP32 GP Timer)
- ✅ **Counter Management** (AUTOSAR-compliant counters)
- ✅ **Alarm Mechanism** (cyclic and one-shot alarms)
- ✅ **Event Mechanism** (AUTOSAR SetEvent / WaitEvent / ClearEvent / IsEventSet)
- ✅ **Event-Based Task Activation** (AUTOSAR ECC2 Extended Task pattern)
- ✅ **Multiple Events Per Task** (bitmask-based, up to 32 events per task)
- ✅ **Task Management** (priority-based scheduling)
- ✅ **Modular Architecture** (separated BSW components)
- ✅ **AUTOSAR Classic OS Compliance** (SetRelAlarm, SetAbsAlarm, CancelAlarm, SetEvent, WaitEvent, ClearEvent)

## Key Design: Event-Based Task Activation (ECC2)

### How It Works

1. **Hardware Timer** generates 1ms ticks
2. **Timer ISR** increments the system counter
3. **Counter increment** triggers alarm processing
4. **Expired alarm** callback calls `OsEvent_SetEvent()` — sets the event flag in a per-task bitmask and wakes the owning task via task notification
5. **Task wakes** from `OsEvent_WaitEvent()`
6. **Task polls** each of its events with `OsEvent_IsEventSet()`
7. **Task clears** each processed event with `OsEvent_ClearEvent()` and executes the application function
8. **Task loops** back to `OsEvent_WaitEvent()`

### Why Events Instead of Direct Activation

In the previous version, alarm callbacks directly notified tasks (one alarm → one task activation). This works for simple cases but does not match AUTOSAR Classic OS semantics. AUTOSAR defines:

- **Events** as independently settable, independently clearable flags per task
- **WaitEvent** as a single blocking point that returns when *any* owned event is set
- **ClearEvent** as an explicit per-event clear *after* the task has read and processed it

This allows a single task to own multiple periodic activities at different rates — exactly what ECC2 Extended Tasks provide. If two events fire before the task processes them, both flags accumulate in the bitmask and are handled in the same wake cycle without loss.

### AUTOSAR ECC2 Pattern (in code)

```c
// Alarm callback (ISR context) – called when alarm expires
static void Os_Alarm50msCallback(void)
{
    OsEvent_SetEvent(OsEvent_50ms);   // set flag + wake task
}

// Extended Task (task context) – ECC2 loop
static void Os_HighPrioPeriodicTask(void *arg)
{
    for (;;)
    {
        OsEvent_WaitEvent();                      // block until any event set

        if (OsEvent_IsEventSet(OsEvent_50ms))     // poll event
        {
            OsEvent_ClearEvent(OsEvent_50ms);     // clear flag
            App_HighPrio_50ms();                   // application code
        }

        if (OsEvent_IsEventSet(OsEvent_200ms))
        {
            OsEvent_ClearEvent(OsEvent_200ms);
            App_HighPrio_200ms();
        }
        // implicit TerminateTask – loops back to WaitEvent
    }
}
```

### AUTOSAR Compliance Table

| AUTOSAR Service | Implementation | Notes |
|----------------|----------------|-------|
| `SetRelAlarm()` | `OsAlarm_SetRel()` | |
| `SetAbsAlarm()` | `OsAlarm_SetAbs()` | |
| `CancelAlarm()` | `OsAlarm_Cancel()` | |
| `GetAlarm()` | `OsAlarm_GetAlarmTime()` | |
| `IncrementCounter()` | `OsCounter_Increment()` | |
| `GetCounterValue()` | `OsCounter_GetValue()` | |
| `SetEvent()` | `OsEvent_SetEvent()` | ISR-safe, bitmask + notification |
| `WaitEvent()` | `OsEvent_WaitEvent()` | Blocks on task notification |
| `ClearEvent()` | `OsEvent_ClearEvent()` | Clears single bit in bitmask |
| `IsEventSet()` | `OsEvent_IsEventSet()` | Queries single bit |
| `ActivateTask()` | Implicit in `SetEvent()` | Notification issued inside SetEvent |
| `TerminateTask()` | Return to WaitEvent loop | Implicit – loop restarts |

## Architecture

```
┌────────────────────────────────────────────────────────┐
│              Application Layer (Os.c)                  │
│  - StartOS()                                           │
│  - HighPrioTask  (WaitEvent / IsEventSet / ClearEvent) │
│  - LowPrioTask   (WaitEvent / IsEventSet / ClearEvent) │
└──────────────┬─────────────────────────────────────────┘
               │
┌──────────────▼─────────────────────────────────────────┐
│                    BSW Layer                            │
├────────────────────────────────────────────────────────┤
│  OsTimer_Cfg  │  OsCounter_Cfg │ OsAlarm_Cfg │OsEvent  │
│  - Init       │  - Init        │ - Init      │ - Init  │
│  - Start/Stop │  - Increment   │ - SetRel    │ - Bind  │
│  - Callback   │  - GetValue    │ - SetAbs    │ - Set   │
│  (ISR Handler)│  (ISR-safe)    │ - Process   │ - Wait  │
│               │                │ - Callback  │ - Clear │
│               │                │             │ - IsSet │
└────────────────────────────────────────────────────────┘
               │
┌──────────────▼─────────────────────────────────────────┐
│           Hardware (ESP32 GP Timer)                    │
│  - 1 MHz resolution                                   │
│  - 1 ms period                                        │
│  - ISR callback                                       │
└────────────────────────────────────────────────────────┘
```

## File Structure

```
├── Os.h                    # Main OS interface
├── Os.c                    # OS implementation – event-based tasks (ECC2)
├── OsTimer_Cfg.h           # Timer interface
├── OsTimer_Cfg.c           # Timer implementation (ESP32 GP Timer)
├── OsCounter_Cfg.h         # Counter interface (AUTOSAR-compliant)
├── OsCounter_Cfg.c         # Counter implementation
├── OsAlarm_Cfg.h           # Alarm interface (AUTOSAR-compliant)
├── OsAlarm_Cfg.c           # Alarm implementation
├── OsEvent_Cfg.h           # Event interface (AUTOSAR SetEvent/WaitEvent/ClearEvent)
├── OsEvent_Cfg.c           # Event implementation (bitmask + task notification)
└── README.md               # This file
```

## Module Responsibilities

### 1. OsTimer Module (`OsTimer_Cfg.h/c`)

**Purpose:** Hardware timer abstraction

**Functions:**
```c
void OsTimer_Init(void);                            // Initialize ESP32 GP Timer
void OsTimer_RegisterCallback(Os_TimerCallbackType); // Set ISR callback
void OsTimer_Start(void);                           // Start timer
void OsTimer_Stop(void);                            // Stop timer
bool OsTimer_IsRunning(void);                       // Get status
```

**Configuration:**
- Resolution: 1 MHz (1 µs tick)
- Period: 1 ms
- Auto-reload: Enabled
- Trigger: System counter increment

### 2. OsCounter Module (`OsCounter_Cfg.h/c`)

**Purpose:** AUTOSAR-compliant counter management

**Functions:**
```c
void OsCounter_Init(void);                          // Initialize counters
void OsCounter_Increment(OsCounter_IdType);         // Increment counter (ISR)
uint32_t OsCounter_GetValue(OsCounter_IdType);      // Read counter
void OsCounter_SetValue(OsCounter_IdType, uint32_t); // Set counter (debug)
```

**AUTOSAR Parameters:**
- `OsCounterMaxAllowedValue`: 100,000
- `OsCounterMinCycle`: 1
- `OsCounterTicksPerBase`: 1
- `OsCounterType`: SOFTWARE (hardware-driven)

**Features:**
- Thread-safe access (ISR and task context)
- Automatic wrap-around
- Triggers alarm processing on increment

### 3. OsAlarm Module (`OsAlarm_Cfg.h/c`)

**Purpose:** AUTOSAR-compliant alarm management

**Functions:**
```c
void OsAlarm_Init(void);                            // Initialize alarms
void OsAlarm_SetRel(AlarmId, Increment, Cycle);     // Set relative alarm
void OsAlarm_SetAbs(AlarmId, Start, Cycle);         // Set absolute alarm
void OsAlarm_Cancel(AlarmId);                       // Cancel alarm
bool OsAlarm_IsActive(AlarmId);                     // Check status
void OsAlarm_GetAlarmTime(AlarmId, Tick*);          // Get expiration time
void OsAlarm_RegisterCallback(AlarmId, Callback);   // Set callback
void OsAlarm_ProcessCounter(CounterId);             // Process alarms (ISR)
```

**Configured Alarms:**

| Alarm ID | Period | Callback Sets Event | Owning Task |
|----------|--------|---------------------|-------------|
| `OsAlarm_Task50ms` | 50 ms | `OsEvent_50ms` | HighPrio |
| `OsAlarm_Task100ms` | 100 ms | `OsEvent_100ms` | LowPrio |
| `OsAlarm_Task200ms` | 200 ms | `OsEvent_200ms` | HighPrio |
| `OsAlarm_Task500ms` | 500 ms | `OsEvent_500ms` | LowPrio |

**Features:**
- Cyclic and one-shot alarms
- ISR-safe callbacks (each callback calls `OsEvent_SetEvent()`)
- Thread-safe configuration
- Automatic rearming for cyclic alarms

### 4. OsEvent Module (`OsEvent_Cfg.h/c`) — NEW

**Purpose:** AUTOSAR-compliant event mechanism for Extended Tasks (ECC2)

**Functions:**
```c
void OsEvent_Init(void);                            // Initialize event subsystem
void OsEvent_BindEvent(EventId, TaskHandle);        // Bind event to owning task
void OsEvent_SetEvent(OsEvent_IdType);              // Set event flag (ISR-safe)
void OsEvent_WaitEvent(void);                       // Block until any event set
void OsEvent_ClearEvent(OsEvent_IdType);            // Clear single event flag
bool OsEvent_IsEventSet(OsEvent_IdType);            // Query single event flag
```

**Configured Events:**

| Event ID | Bit | Owning Task | Triggered By |
|----------|-----|-------------|--------------|
| `OsEvent_50ms` | bit 0 | HighPrio | `OsAlarm_Task50ms` |
| `OsEvent_200ms` | bit 1 | HighPrio | `OsAlarm_Task200ms` |
| `OsEvent_100ms` | bit 0 | LowPrio | `OsAlarm_Task100ms` |
| `OsEvent_500ms` | bit 1 | LowPrio | `OsAlarm_Task500ms` |

**Design:**
- Each task owns a `uint32_t` bitmask — each bit is one event
- `SetEvent()` performs an atomic bit-OR on the mask and issues a task notification (ISR-safe)
- `WaitEvent()` blocks on the task notification channel; returns when any owned event is set
- `IsEventSet()` reads a single bit — task checks each event independently
- `ClearEvent()` clears a single bit — task clears after processing
- If multiple events fire before the task runs, all flags accumulate — none are lost

### 5. Os Module (`Os.h/c`)

**Purpose:** Main OS interface and event-based task management

**Functions:**
```c
void StartOS(AppModeType mode);                     // Start OS
uint32_t Os_GetTickCount(void);                     // Get system ticks
void Os_Delay(uint32_t ticks);                      // Delay execution
```

## Initialization Sequence

```c
void StartOS(AppModeType mode)
{
    // 1. Initialize counter subsystem
    OsCounter_Init();

    // 2. Initialize alarm subsystem
    OsAlarm_Init();

    // 3. Initialize event subsystem
    OsEvent_Init();

    // 4. Initialize hardware timer
    OsTimer_Init();
    OsTimer_RegisterCallback(Os_TimerCallback);

    // 5. Create application tasks
    xTaskCreate(Os_InitTask, ...);
    xTaskCreate(Os_HighPrioPeriodicTask, ...);
    xTaskCreate(Os_LowPrioPeriodicTask, ...);

    // 6. Bind events to owning tasks (after task creation)
    OsEvent_BindEvent(OsEvent_50ms,  Os_HighPrioTaskHandle);
    OsEvent_BindEvent(OsEvent_200ms, Os_HighPrioTaskHandle);
    OsEvent_BindEvent(OsEvent_100ms, Os_LowPrioTaskHandle);
    OsEvent_BindEvent(OsEvent_500ms, Os_LowPrioTaskHandle);

    // 7. Register alarm callbacks (each calls SetEvent)
    OsAlarm_RegisterCallback(OsAlarm_Task50ms,  Os_Alarm50msCallback);
    OsAlarm_RegisterCallback(OsAlarm_Task100ms, Os_Alarm100msCallback);
    OsAlarm_RegisterCallback(OsAlarm_Task200ms, Os_Alarm200msCallback);
    OsAlarm_RegisterCallback(OsAlarm_Task500ms, Os_Alarm500msCallback);

    // 8. Start timer (enables counter increment)
    OsTimer_Start();

    // 9. Activate cyclic alarms (StartupHook equivalent)
    OsAlarm_SetRel(OsAlarm_Task50ms,   50,   50);
    OsAlarm_SetRel(OsAlarm_Task100ms, 100,  100);
    OsAlarm_SetRel(OsAlarm_Task200ms, 200,  200);
    OsAlarm_SetRel(OsAlarm_Task500ms, 500,  500);
}
```

## Data Flow

```
Hardware Timer (1ms)
      |
      +-> OsTimer_ISR()
      |        |
      |        +-> Os_TimerCallback()
      |                  |
      |                  +-> OsCounter_Increment(OsCounter_System)
      |                            |
      |                            +-> Counter++
      |                            |
      |                            +-> OsAlarm_ProcessCounter()
      |                                      |
      |                        +------------+------------+------------+
      |                        |            |            |            |
      |                  50ms alarm   100ms alarm  200ms alarm  500ms alarm
      |                        |            |            |            |
      |                  SetEvent     SetEvent     SetEvent     SetEvent
      |                (50ms flag)  (100ms flag) (200ms flag) (500ms flag)
      |                        |            |            |            |
      |                  +-----+            |      +-----+            |
      |                  |                  |      |                  |
      |           HighPrio task      LowPrio task  |           LowPrio task
      |           wakes up          wakes up       |           (same task)
      |                                            |
      |                                      HighPrio task
      |                                      (same task)
      |
      +---> Tasks wake, poll IsEventSet(), clear, execute app code
```

## Usage Example

### Main Application

```c
#include "Os.h"

void app_main(void)
{
    printf("Starting AUTOSAR OS Layer\n");

    // Initialize and start OS
    // This creates tasks, binds events, configures alarms, and starts the timer
    StartOS(OSDEFAULTAPPMODE);

    // Tasks are now running, activated by events triggered by alarms
    // FreeRTOS scheduler handles execution
}
```

### Adding a New Event to an Existing Task

**Step 1:** Add the event ID in `OsEvent_Cfg.h`:

```c
typedef enum
{
    OsEvent_50ms  = 0,
    OsEvent_200ms,
    OsEvent_100ms,
    OsEvent_500ms,
    OsEvent_MyNewEvent,   // <-- new event
    OS_EVENT_COUNT
} OsEvent_IdType;
```

**Step 2:** Add the binding entry in `OsEvent_Cfg.c` (must match the enum order):

```c
static const OsEvent_BindingType OsEvent_Bindings[OS_EVENT_COUNT] =
{
    /* OsEvent_50ms        */ { .TaskContextIndex = 0, .BitMask = (1U << 0) },
    /* OsEvent_200ms       */ { .TaskContextIndex = 0, .BitMask = (1U << 1) },
    /* OsEvent_100ms       */ { .TaskContextIndex = 1, .BitMask = (1U << 0) },
    /* OsEvent_500ms       */ { .TaskContextIndex = 1, .BitMask = (1U << 1) },
    /* OsEvent_MyNewEvent  */ { .TaskContextIndex = 0, .BitMask = (1U << 2) }  // HighPrio, bit 2
};
```

**Step 3:** Add a new alarm in `OsAlarm_Cfg.h`:

```c
typedef enum
{
    OsAlarm_Task50ms  = 0,
    OsAlarm_Task100ms,
    OsAlarm_Task200ms,
    OsAlarm_Task500ms,
    OsAlarm_MyNewAlarm,   // <-- new alarm
    OS_ALARM_COUNT
} OsAlarm_IdType;
```

**Step 4:** Add the alarm callback and wire everything in `Os.c`:

```c
// Alarm callback
static void Os_AlarmMyNewCallback(void)
{
    OsEvent_SetEvent(OsEvent_MyNewEvent);
}

// In StartOS():
OsEvent_BindEvent(OsEvent_MyNewEvent, Os_HighPrioTaskHandle);
OsAlarm_RegisterCallback(OsAlarm_MyNewAlarm, Os_AlarmMyNewCallback);
OsAlarm_SetRel(OsAlarm_MyNewAlarm, 300, 300);  // 300 ms cyclic

// In Os_HighPrioPeriodicTask(), add after existing event checks:
if (OsEvent_IsEventSet(OsEvent_MyNewEvent))
{
    OsEvent_ClearEvent(OsEvent_MyNewEvent);
    // App_HighPrio_300ms();
}
```

### Adding a New Task with Its Own Events

**Step 1:** Add two new event IDs in `OsEvent_Cfg.h` and increase `OS_EVENT_TASK_COUNT` to 3 in the header.

**Step 2:** Add binding entries pointing to task context index 2 in `OsEvent_Cfg.c`.

**Step 3:** Add two new alarm IDs in `OsAlarm_Cfg.h`.

**Step 4:** In `Os.c`, create the task, bind events, register callbacks, activate alarms, and write the ECC2 task loop.

### Using One-Shot Alarms with Events

```c
// Trigger an event once after 5 seconds
OsAlarm_SetRel(OsAlarm_MyOneShot, 5000, 0);  // Cycle=0 for one-shot

// Cancel before it fires
OsAlarm_Cancel(OsAlarm_MyOneShot);
```

## Configuration

### Counter Configuration (`OsCounter_Cfg.h`)

```c
#define OS_COUNTER_MAX_ALLOWED    (100000U)  // Max counter value
#define OS_COUNTER_MIN_CYCLE      (1U)       // Min alarm cycle
#define OS_COUNTER_TICKS_PER_BASE (1U)       // 1 tick = 1ms
```

### Timer Configuration (`OsTimer_Cfg.h`)

```c
#define OS_TIMER_RESOLUTION_HZ  (1000000U)  // 1 MHz
#define OS_TIMER_PERIOD_US      (1000U)     // 1 ms
```

### Alarm Configuration (`OsAlarm_Cfg.h`)

```c
typedef enum
{
    OsAlarm_Task50ms  = 0,   //  50ms cyclic alarm
    OsAlarm_Task100ms,       // 100ms cyclic alarm
    OsAlarm_Task200ms,       // 200ms cyclic alarm
    OsAlarm_Task500ms,       // 500ms cyclic alarm
    OS_ALARM_COUNT           // Total alarm count
} OsAlarm_IdType;
```

### Event Configuration (`OsEvent_Cfg.h`)

```c
typedef enum
{
    OsEvent_50ms  = 0,   // HighPrio task – bit 0
    OsEvent_200ms,       // HighPrio task – bit 1
    OsEvent_100ms,       // LowPrio  task – bit 0
    OsEvent_500ms,       // LowPrio  task – bit 1
    OS_EVENT_COUNT       // Total event count
} OsEvent_IdType;

#define OS_EVENT_TASK_COUNT   (2U)  // Number of tasks that own events
#define OS_EVENT_MAX_PER_TASK (32U) // Max events per task (uint32_t bitmask)
```

## Task Configuration

| Task | Priority | Owns Events | Alarm Periods | Description |
|------|----------|-------------|---------------|-------------|
| InitTask | 9 | None | One-shot | System initialization |
| HighPrio | 8 | OsEvent_50ms, OsEvent_200ms | 50 ms, 200 ms | Fast control loops |
| LowPrio | 6 | OsEvent_100ms, OsEvent_500ms | 100 ms, 500 ms | Monitoring & diagnostics |

## Thread Safety

All BSW modules use FreeRTOS spinlocks for thread safety:

```c
// Task context
portENTER_CRITICAL(&Mutex);
// Critical section
portEXIT_CRITICAL(&Mutex);

// ISR context
portENTER_CRITICAL_ISR(&Mutex);
// Critical section
portEXIT_CRITICAL_ISR(&Mutex);
```

**Critical sections protect:**
- Counter increment and read
- Alarm configuration and processing
- Event bitmask set and clear operations

## Integration with ESP-IDF

### CMakeLists.txt

```cmake
idf_component_register(
    SRCS
        "Os.c"
        "OsTimer_Cfg.c"
        "OsCounter_Cfg.c"
        "OsAlarm_Cfg.c"
        "OsEvent_Cfg.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        freertos
        driver
)
```

### Required Components

- `freertos` – Task management and notifications
- `driver` – ESP32 GP Timer driver

## Debugging

### Check System Status

```c
// Get current tick
uint32_t tick = Os_GetTickCount();
printf("System tick: %lu ms\n", tick);

// Check timer status
if (OsTimer_IsRunning())
{
    printf("Timer is running\n");
}

// Check alarm status
if (OsAlarm_IsActive(OsAlarm_Task50ms))
{
    uint32_t alarmTime;
    OsAlarm_GetAlarmTime(OsAlarm_Task50ms, &alarmTime);
    printf("Alarm active, expires at: %lu\n", alarmTime);
}

// Check event status
if (OsEvent_IsEventSet(OsEvent_50ms))
{
    printf("OsEvent_50ms is pending\n");
}
```

### Enable FreeRTOS Stats

In `sdkconfig`:
```
CONFIG_FREERTOS_USE_TRACE_FACILITY=y
CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS=y
```

Then use:
```c
char buffer[512];
vTaskGetRunTimeStats(buffer);
printf("%s\n", buffer);
```

## Performance Metrics

Tested on ESP32-WROOM-32 @ 240MHz:

| Operation | Time (µs) | Context | Notes |
|-----------|-----------|---------|-------|
| Timer ISR | <5 | ISR | Includes counter increment |
| Counter increment | <2 | ISR | With spinlock |
| Alarm check | <3 | ISR | Per alarm |
| Alarm callback (SetEvent) | <7 | ISR | Bit-OR + task notification |
| Event IsEventSet | <1 | Task | Single bit read |
| Event ClearEvent | <1 | Task | Single bit clear |
| Counter read | <1 | Task | With spinlock |
| Alarm config | <2 | Task | SetRel/SetAbs |
| Task activation | 5-15 | – | FreeRTOS overhead |
| Context switch | 10-20 | – | Depends on CPU load |

**Total ISR overhead (worst case – all 4 alarms fire in same tick):**
- Timer ISR: 5 µs
- Counter increment: 2 µs
- Check 4 alarms: 12 µs (3 µs each)
- 4 SetEvent calls: 28 µs (7 µs each – bit-OR + notification)
- **Total: ~47 µs** (4.7% of 1ms tick)

## Memory Usage

Approximate footprint:

**Code:**
- Timer: ~800 bytes
- Counter: ~500 bytes
- Alarm: ~1500 bytes
- Event: ~900 bytes
- Os: ~1400 bytes
- **Total: ~5.1 KB**

**RAM:**
- Counters: 4 bytes × count (4 bytes for 1 counter)
- Alarms: 20 bytes × count (80 bytes for 4 alarms)
- Event task contexts: 8 bytes × task count (16 bytes for 2 tasks)
- Event binding table: 5 bytes × event count (20 bytes for 4 events)
- Handles: ~50 bytes
- Mutexes: ~120 bytes (3 spinlocks)
- **Total: ~290 bytes**

**Stack:**
- 2048 bytes per task (configurable)
- 3 tasks = 6144 bytes total

## AUTOSAR Classic OS Compliance

### Supported Features

| Feature | Status | Implementation |
|---------|--------|----------------|
| Counters | ✅ | Software counter, hardware-driven |
| Alarms | ✅ | Cyclic and one-shot |
| Events | ✅ | Bitmask per task + task notification |
| Extended Tasks (ECC2) | ✅ | WaitEvent / IsEventSet / ClearEvent loop |
| Task Activation | ✅ | Implicit in SetEvent (notification) |
| Task Termination | ✅ | Return to WaitEvent (implicit) |
| Interrupt Categories | ⚠️ | All ISR are Cat 2 (FreeRTOS) |
| Resources | ❌ | Use FreeRTOS mutex/semaphore |
| Schedule Tables | ❌ | Not implemented |
| OS Applications | ❌ | Not implemented |

### AUTOSAR API Mapping

```c
// AUTOSAR                 → Implementation

SetRelAlarm()              → OsAlarm_SetRel()
SetAbsAlarm()              → OsAlarm_SetAbs()
CancelAlarm()              → OsAlarm_Cancel()
GetAlarm()                 → OsAlarm_GetAlarmTime()
IncrementCounter()         → OsCounter_Increment()
GetCounterValue()          → OsCounter_GetValue()
SetEvent()                 → OsEvent_SetEvent()
WaitEvent()                → OsEvent_WaitEvent()
ClearEvent()               → OsEvent_ClearEvent()
IsEventSet()               → OsEvent_IsEventSet()
ActivateTask()             → Implicit inside OsEvent_SetEvent()
TerminateTask()            → Return from task body (implicit loop)
GetTaskID()                → xTaskGetCurrentTaskHandle()
```

## Differences from Pure AUTOSAR OS

| Feature | AUTOSAR OS | This Implementation |
|---------|------------|---------------------|
| Scheduler | OSEK/VDX | FreeRTOS |
| Task activation | ActivateTask() | SetEvent() + task notification |
| Events | Bitmask per task | uint32_t bitmask per task context |
| Resources | Resource API | Mutex/Semaphore |
| ISR Categories | Cat 1/2 | FreeRTOS ISR (Cat 2-like) |
| Conformance class | BCC1/2, ECC1/2 | ECC2-style (RTOS-based) |
| Error handling | ErrorHook | printf (development) |
| Timing protection | Built-in | Not implemented |

## Extension Ideas

- [ ] Implement Resource/Mutex API (AUTOSAR-style)
- [ ] Add Schedule Tables
- [ ] Multi-core support (ESP32 dual-core)
- [ ] Timing protection hooks
- [ ] Error handling (Det module)
- [ ] OS application isolation
- [ ] Stack monitoring
- [ ] ISR Category 1 emulation

## Best Practices

### ✅ DO

- Use alarms to trigger events, events to activate task code
- Keep alarm callbacks short (<10 µs) – they only call SetEvent()
- Clear events *before* executing application code (matches AUTOSAR pattern)
- Poll all owned events after each WaitEvent() return
- Use relative alarms for periodic tasks
- Protect shared resources with critical sections
- Follow AUTOSAR naming conventions

### ❌ DON'T

- Don't perform long operations in alarm callbacks (only SetEvent)
- Don't call blocking functions from alarm callbacks
- Don't modify alarm or event configuration from ISR
- Don't exceed counter maximum value
- Don't use cycle time less than `OS_COUNTER_MIN_CYCLE`
- Don't forget to bind events to tasks before starting the timer
- Don't forget to register alarm callbacks before activating alarms
- Don't use `vTaskDelayUntil()` for periodic tasks (use alarms + events)

## Troubleshooting

**Timer not starting:**
- Check ESP_ERROR_CHECK logs
- Verify GP Timer driver is enabled in menuconfig
- Check system clock configuration
- Ensure timer callback is registered before start

**Alarms not firing:**
- Verify counter is incrementing (`Os_GetTickCount()`)
- Check alarm configuration (increment > 0, cycle >= 0)
- Ensure callback is registered
- Verify alarm is activated
- Check for counter wrap-around issues

**Events not being set:**
- Verify the alarm callback is executing (add debug print)
- Check that `OsEvent_BindEvent()` was called with a valid task handle
- Ensure `OsEvent_Init()` was called before any other event function

**Tasks not waking up:**
- Verify `OsEvent_BindEvent()` was called after `xTaskCreate()`
- Check that the task is blocked in `OsEvent_WaitEvent()`
- Verify SetEvent is being called (check alarm callback)

**Events appearing to be lost:**
- Events accumulate in the bitmask – if the task is slow, multiple events may be pending simultaneously. This is by design. Ensure you check all events after each WaitEvent() return.
- If ClearEvent is called before IsEventSet, the event will appear unset.

**Timing inaccuracies:**
- Check CPU frequency (should be 240 MHz)
- Monitor ISR execution time
- Reduce number of active alarms if ISR overhead is high
- Check for priority inversion

**Memory issues:**
- Increase task stack size if overflow occurs
- Monitor heap usage
- Check for resource leaks
- Use `CONFIG_FREERTOS_CHECK_STACKOVERFLOW`

## Support

For issues or questions, refer to:
- ESP-IDF documentation: https://docs.espressif.com
- FreeRTOS documentation: https://www.freertos.org
- AUTOSAR Classic OS specs: https://www.autosar.org/standards/classic-platform
