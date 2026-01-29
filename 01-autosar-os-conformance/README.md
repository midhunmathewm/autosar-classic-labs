# AUTOSAR-Style OS Layer for ESP32

## Overview

This implementation provides an AUTOSAR BSW-inspired OS abstraction layer running on ESP32 with FreeRTOS. It features modular architecture with separated Timer, Counter, and Alarm management.

## Features

- ✅ **Hardware Timer** (1ms tick using ESP32 GP Timer)
- ✅ **Counter Management** (AUTOSAR-style counters)
- ✅ **Alarm Mechanism** (cyclic and one-shot alarms)
- ✅ **Task Management** (priority-based scheduling)
- ✅ **Modular Architecture** (separated BSW components)

## Architecture

```
┌────────────────────────────────────────────────┐
│           Application Layer (Os.c)             │
│  - StartOS()                                   │
│  - Application Tasks                           │
└──────────────┬─────────────────────────────────┘
               │
┌──────────────▼─────────────────────────────────┐
│              BSW Layer                         │
├────────────────────────────────────────────────┤
│  OsTimer_Cfg    │  OsCounter_Cfg  │  OsAlarm_Cfg│
│  - Init         │  - Init          │  - Init     │
│  - Start/Stop   │  - Increment     │  - SetRel   │
│  - Callback     │  - GetValue      │  - SetAbs   │
│                 │                  │  - Cancel   │
└────────────────┴──────────────────┴─────────────┘
               │
┌──────────────▼─────────────────────────────────┐
│         Hardware (ESP32 GP Timer)              │
│  - 1 MHz resolution                            │
│  - 1 ms period                                 │
│  - ISR callback                                │
└────────────────────────────────────────────────┘
```

## File Structure

```
├── Os.h                    # Main OS interface
├── Os.c                    # OS implementation & tasks
├── OsTimer_Cfg.h          # Timer interface
├── OsTimer_Cfg.c          # Timer implementation
├── OsCounter_Cfg.h        # Counter interface
├── OsCounter_Cfg.c        # Counter implementation
├── OsAlarm_Cfg.h          # Alarm interface
├── OsAlarm_Cfg.c          # Alarm implementation
└── README.md              # This file
```

## Module Responsibilities

### 1. OsTimer Module (`OsTimer_Cfg.h/c`)

**Purpose:** Hardware timer abstraction

**Functions:**
```c
void OsTimer_Init(void);                           // Initialize GP Timer
void OsTimer_RegisterCallback(Os_TimerCallbackType); // Set ISR callback
void OsTimer_Start(void);                          // Start timer
void OsTimer_Stop(void);                           // Stop timer
bool OsTimer_IsRunning(void);                      // Get status
```

**Configuration:**
- Resolution: 1 MHz (1 µs)
- Period: 1 ms
- Auto-reload: Enabled

### 2. OsCounter Module (`OsCounter_Cfg.h/c`)

**Purpose:** Counter management

**Functions:**
```c
void OsCounter_Init(void);                         // Initialize counters
void OsCounter_Increment(OsCounter_IdType);        // Increment counter
uint32_t OsCounter_GetValue(OsCounter_IdType);     // Read counter
void OsCounter_SetValue(OsCounter_IdType, uint32_t); // Set counter (debug)
```

**Features:**
- Thread-safe access
- Automatic wrap-around
- Multiple counter support

### 3. OsAlarm Module (`OsAlarm_Cfg.h/c`)

**Purpose:** Alarm management

**Functions:**
```c
void OsAlarm_Init(void);                           // Initialize alarms
void OsAlarm_SetRel(AlarmId, Increment, Cycle);    // Set relative alarm
void OsAlarm_SetAbs(AlarmId, Start, Cycle);        // Set absolute alarm
void OsAlarm_Cancel(AlarmId);                      // Cancel alarm
bool OsAlarm_IsActive(AlarmId);                    // Check status
void OsAlarm_RegisterCallback(AlarmId, Callback);  // Set callback
```

**Features:**
- Cyclic and one-shot alarms
- ISR-safe callbacks
- Thread-safe configuration

### 4. Os Module (`Os.h/c`)

**Purpose:** Main OS interface and application tasks

**Functions:**
```c
void StartOS(AppModeType mode);                    // Start OS
uint32_t Os_GetTickCount(void);                    // Get system ticks
void Os_Delay(uint32_t ticks);                     // Delay execution
```

## Initialization Sequence

```c
void StartOS(AppModeType mode)
{
    // 1. Initialize counter subsystem
    OsCounter_Init();
    
    // 2. Initialize alarm subsystem
    OsAlarm_Init();
    
    // 3. Initialize hardware timer
    OsTimer_Init();
    
    // 4. Register timer callback
    OsTimer_RegisterCallback(Os_TimerCallback);
    
    // 5. Start timer
    OsTimer_Start();
    
    // 6. Create application tasks
    xTaskCreate(Os_InitTask, ...);
    xTaskCreate(Os_HighPrioTask, ...);
    xTaskCreate(Os_LowPrioTask, ...);
}
```

## Data Flow

```
Hardware Timer (1ms)
      │
      ├─► OsTimer_ISR()
      │        │
      │        └─► Os_TimerCallback()
      │                  │
      │                  └─► OsCounter_Increment(OsCounter_System)
      │                            │
      │                            ├─► Counter++
      │                            │
      │                            └─► OsAlarm_ProcessCounter()
      │                                      │
      │                                      ├─► Check all alarms
      │                                      │
      │                                      └─► Trigger callbacks if expired
      │                                                │
      └───────────────────────────────────────────────┴─► Application Tasks
```

## Usage Example

### Main Application

```c
#include "Os.h"

void app_main(void)
{
    printf("Starting AUTOSAR OS Layer\n");
    
    // Initialize and start OS
    StartOS(OSDEFAULTAPPMODE);
    
    // Tasks are now running
    // FreeRTOS scheduler handles execution
}
```

### Adding Custom Alarms

**Step 1:** Define alarm ID in `OsAlarm_Cfg.h`:

```c
typedef enum
{
    OsAlarm_200ms = 0,
    OsAlarm_1000ms,
    OsAlarm_MyCustomAlarm,  // New alarm
    OS_ALARM_COUNT
} OsAlarm_IdType;
```

**Step 2:** Register callback:

```c
void MyAlarmCallback(void)
{
    // Called from ISR when alarm expires
    // Keep it fast and ISR-safe!
}

// In initialization:
OsAlarm_RegisterCallback(OsAlarm_MyCustomAlarm, MyAlarmCallback);
OsAlarm_SetRel(OsAlarm_MyCustomAlarm, 500, 500);  // 500ms cyclic
```

### Adding Custom Tasks

```c
static void MyCustomTask(void *arg)
{
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    for (;;)
    {
        // Task body
        
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(100));  // 100ms
    }
}

// In StartOS():
xTaskCreate(MyCustomTask, "MyTask", 2048, NULL, 7, NULL);
```

## Configuration

### Counter Configuration (`OsCounter_Cfg.h`)

```c
#define OS_COUNTER_MAX_ALLOWED   (100000U)  // Wrap at 100,000
#define OS_COUNTER_MIN_CYCLE     (1U)       // Min cycle time
#define OS_COUNTER_TICKS_PER_BASE (1U)      // 1 tick = 1ms
```

### Timer Configuration (`OsTimer_Cfg.h`)

```c
#define OS_TIMER_RESOLUTION_HZ  (1000000U)  // 1 MHz
#define OS_TIMER_PERIOD_US      (1000U)     // 1 ms
```

## Task Configuration

Current task priorities (higher = more important):

| Task | Priority | Period | Description |
|------|----------|--------|-------------|
| InitTask | 9 | One-shot | System initialization |
| HighPrio | 8 | 200ms | Fast control loops |
| LowPrio | 6 | 1000ms | Monitoring & diagnostics |

## Thread Safety

All BSW modules use FreeRTOS spinlocks:

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

## Integration with ESP-IDF

### CMakeLists.txt

```cmake
idf_component_register(
    SRCS 
        "Os.c"
        "OsTimer_Cfg.c"
        "OsCounter_Cfg.c"
        "OsAlarm_Cfg.c"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        freertos
        driver
)
```

### Required Components

- `freertos` - Task management
- `driver` - GP Timer driver

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
if (OsAlarm_IsActive(OsAlarm_200ms))
{
    uint32_t alarmTime;
    OsAlarm_GetAlarmTime(OsAlarm_200ms, &alarmTime);
    printf("Alarm active, expires at: %lu\n", alarmTime);
}
```

### Enable FreeRTOS Stats

In `sdkconfig`:
```
CONFIG_FREERTOS_USE_TRACE_FACILITY=y
CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y
```

## Performance Metrics

Tested on ESP32-WROOM-32 @ 240MHz:

| Operation | Time (µs) | Notes |
|-----------|-----------|-------|
| Timer ISR | <5 | Includes counter increment |
| Alarm check | <3 | Per alarm |
| Counter read | <1 | With spinlock |
| Alarm config | <2 | SetRel/SetAbs |
| Task switch | 5-15 | FreeRTOS overhead |

## Memory Usage

Approximate footprint:

- **Code:** ~3 KB
- **RAM:** 
  - Counters: 4 bytes × count
  - Alarms: 20 bytes × count
  - Handles: ~50 bytes
- **Stack:** 2048 bytes per task (configurable)

## Differences from AUTOSAR OS

| Feature | AUTOSAR OS | This Implementation |
|---------|------------|---------------------|
| Scheduler | OSEK/VDX | FreeRTOS |
| Task activation | ActivateTask() | vTaskDelayUntil() or notify |
| Resources | Resource API | Mutex/Semaphore |
| ISR Categories | Cat 1/2 | FreeRTOS ISR |
| Conformance class | BCC1/2, ECC1/2 | N/A |

## Extension Ideas

- [ ] Add Event mechanism
- [ ] Implement Resource/Mutex API
- [ ] Add Schedule Tables
- [ ] Multi-core support (ESP32 dual-core)
- [ ] Timing protection hooks
- [ ] Error handling (Det module)
- [ ] OS application isolation

## License

MIT License

## Troubleshooting

**Timer not starting:**
- Check ESP_ERROR_CHECK logs
- Verify GP Timer driver is enabled
- Check system clock configuration

**Alarms not firing:**
- Verify OsCounter_Increment() is being called
- Check alarm configuration (increment > 0)
- Ensure callback is registered

**Task timing issues:**
- Increase task priority if preempted
- Check for long-running critical sections
- Monitor CPU usage with stats

## Support

For issues or questions, refer to:
- ESP-IDF documentation: https://docs.espressif.com
- FreeRTOS documentation: https://www.freertos.org
- AUTOSAR specifications: https://www.autosar.org
