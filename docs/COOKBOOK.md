# Cookbook

## 1. Minimal global logger setup

```c
mlog_backend_t backend = { .write = uart_write, .ctx = NULL, .level = MLOG_INFO, .color = false };
mlog_add_backend(mlog_global(), &backend);
```

## 2. Explicit logger instance

```c
mlog_t log;
mlog_init(&log);
mlog_add_backend(&log, &backend);
```

## 3. Backend descriptor by-value copy

`mlog_add_backend()` copies `mlog_backend_t` by value. The callback and `ctx` pointer must remain valid after registration.

## 4. Backend context forwarding

```c
struct uart_port *port = &uart2;
mlog_backend_t backend = { .write = uart_write, .ctx = port, .level = MLOG_DEBUG, .color = false };
```

## 5. Plain log call with no format args

```c
MLOG_INFO("BOOT", "ready");
```

## 6. Formatted log call

```c
MLOG_WARN("BATT", "Low battery: %u%%", percent);
```

## 7. Color enabled and disabled

```c
mlog_backend_t term = { .write = term_write, .ctx = NULL, .level = MLOG_TRACE, .color = true };
mlog_backend_t uart = { .write = uart_write, .ctx = NULL, .level = MLOG_INFO, .color = false };
```

## 8. Timestamp callback

```c
static uint32_t app_clock(void) { return HAL_GetTick(); }
mlog_set_clock(mlog_global(), app_clock);
```

## 9. Multiple backends with shared timestamp

One log event samples the clock once and reuses the same timestamp for every interested backend.

## 10. Hex dump

```c
uint8_t bytes[] = { 0x00, 0x0A, 0xFF, 0x42 };
mlog_hexdump(NULL, MLOG_DEBUG, "PKT", bytes, sizeof(bytes));
```

Output order follows source byte order: `00 0A FF 42`.

## 11. Compile-time disabled logging

```c
#define MLOG_LEVEL_MIN MLOG_WARN
```

`TRACE`, `DEBUG`, and `INFO` macros below the threshold expand away and evaluate none of their arguments.

## 12. External locking

```c
lock();
MLOG_ERROR("NET", "send failed");
unlock();
```

## 13. Copy callback buffer before returning

Callbacks must copy the provided buffer before queuing DMA, deferring work, or returning to the caller.

## 14. CMake install and find_package

```cmake
find_package(microlog CONFIG REQUIRED)
target_link_libraries(app PRIVATE microlog::microlog)
```

## 15. C++ consumer

```cpp
extern "C" {
#include "mlog.h"
}
```

## 16. Embedded/freestanding note

microlog has no third-party runtime dependencies, but it does require the C library functions used by the implementation.

## 17. Truncation behavior

Long tags and messages are truncated to preserve a final newline, a terminating NUL, and an ANSI reset sequence when color was emitted.

## 18. Invalid level handling

Invalid message levels, including `MLOG_NONE`, are ignored and do not emit ordinary log records.
