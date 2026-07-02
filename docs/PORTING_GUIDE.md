# Porting Guide

microlog requires `mlog.h`, `mlog.c`, a C99 compiler, and the C library facilities used by the implementation: `vsnprintf`, `snprintf`, `memcpy`, `memset`, and `strlen`.

## Checklist

1. Pick ABI-affecting config values and keep them identical across the library and consumers.
2. Implement one or more synchronous backend callbacks.
3. Decide whether timestamps are enabled and provide a `uint32_t` clock if needed.
4. Add external locking if multiple threads or tasks can access the same logger.
5. Do not retain callback buffer pointers after the callback returns.

## UART Example

```c
static void uart_write(const char *buf, uint16_t len, mlog_level_t level, void *ctx)
{
    (void)level;
    (void)ctx;
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, 100);
}
```

## Volatile Ring Buffer Example

```c
#define LOG_RING_SIZE 2048
static char log_ring[LOG_RING_SIZE];
static uint16_t ring_head;

static void ring_write(const char *buf, uint16_t len, mlog_level_t level, void *ctx)
{
    (void)level;
    (void)ctx;
    for (uint16_t i = 0; i < len; ++i) {
        log_ring[ring_head] = buf[i];
        ring_head = (uint16_t)((ring_head + 1u) % LOG_RING_SIZE);
    }
}
```

This is volatile in-memory capture only. It does not imply reset-safe or power-loss-safe persistence.

## CMake Integration

```cmake
find_package(microlog CONFIG REQUIRED)
add_executable(app main.c)
target_link_libraries(app PRIVATE microlog::microlog)
```

ABI-affecting definitions are exported publicly through the package target. Consumers must use the same values as the compiled library.
