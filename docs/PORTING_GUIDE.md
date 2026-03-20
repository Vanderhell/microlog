# Porting Guide

microlog needs `mlog.h`, `mlog.c`, and a C99 compiler with `vsnprintf`.

---

## Platform backends

### STM32 HAL (UART)

```c
static void uart_write(const char *buf, uint16_t len,
                        mlog_level_t level, void *ctx) {
    (void)level; (void)ctx;
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, 100);
}

void logging_init(void) {
    mlog_set_clock(NULL, HAL_GetTick);
    mlog_backend_t be = { .write = uart_write, .level = MLOG_DEBUG, .color = false };
    mlog_add_backend(NULL, &be);
}
```

### ESP-IDF (uses ESP_LOG internally, but microlog gives you control)

```c
static void esp_write(const char *buf, uint16_t len,
                       mlog_level_t level, void *ctx) {
    (void)level; (void)ctx;
    uart_write_bytes(UART_NUM_0, buf, len);
}

static uint32_t esp_clock(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}
```

### Segger RTT (J-Link debug probe)

```c
#include "SEGGER_RTT.h"

static void rtt_write(const char *buf, uint16_t len,
                       mlog_level_t level, void *ctx) {
    (void)level; (void)ctx;
    SEGGER_RTT_Write(0, buf, len);
}

mlog_backend_t rtt = { .write = rtt_write, .level = MLOG_TRACE, .color = true };
```

### Ring buffer (post-mortem)

```c
#define LOG_RING_SIZE 2048
static char log_ring[LOG_RING_SIZE];
static uint16_t ring_head = 0;

static void ring_write(const char *buf, uint16_t len,
                        mlog_level_t level, void *ctx) {
    (void)level; (void)ctx;
    for (uint16_t i = 0; i < len; i++) {
        log_ring[ring_head] = buf[i];
        ring_head = (ring_head + 1) % LOG_RING_SIZE;
    }
}

/* Errors only — for crash analysis */
mlog_backend_t ring = { .write = ring_write, .level = MLOG_ERROR, .color = false };
```

### Linux / POSIX (stderr)

```c
#include <stdio.h>

static void stderr_write(const char *buf, uint16_t len,
                          mlog_level_t level, void *ctx) {
    (void)level; (void)ctx;
    fwrite(buf, 1, len, stderr);
}
```

### Arduino

```cpp
extern "C" {
    #include "mlog.h"
}

static void serial_write(const char *buf, uint16_t len,
                          mlog_level_t level, void *ctx) {
    (void)level; (void)ctx;
    Serial.write((const uint8_t *)buf, len);
}

static uint32_t arduino_clock(void) { return millis(); }
```

---

## CMake integration

```cmake
add_library(microlog STATIC lib/microlog/src/mlog.c)
target_include_directories(microlog PUBLIC lib/microlog/include)

# Production: strip TRACE/DEBUG, no color
target_compile_definitions(microlog PUBLIC
    MLOG_LEVEL_MIN=2
    MLOG_ENABLE_COLOR=0
)
```

---

## Checklist

1. **C99 + vsnprintf?** → good to go.
2. **Write a backend callback** → UART, RTT, ring buffer, file, etc.
3. **Provide clock** → HAL_GetTick, millis(), esp_timer, or NULL.
4. **Production build** → set `MLOG_LEVEL_MIN=2` or higher.
5. **Multiple threads?** → protect with mutex or use lock-free backend.
