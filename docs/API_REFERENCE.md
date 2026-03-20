# API Reference

> **Header:** `#include "mlog.h"`
>
> **Version:** 1.0.0

---

## Log levels

| Level | Value | Char | Color | Use |
|-------|-------|------|-------|-----|
| `MLOG_TRACE` | 0 | T | gray | Hot path details |
| `MLOG_DEBUG` | 1 | D | cyan | Development debugging |
| `MLOG_INFO` | 2 | I | green | Normal events |
| `MLOG_WARN` | 3 | W | yellow | Unexpected but handled |
| `MLOG_ERROR` | 4 | E | red | Failures needing attention |
| `MLOG_NONE` | 5 | - | — | Disable logging (filter only) |

## Output format

```
[timestamp] [level] tag: message\n

Examples:
12.345 [I] MQTT: Connected to broker.local:1883
12.350 [W] SENS: Temperature out of range: 85.3
[E] CONF: CRC failed (no timestamp if clock is NULL)
```

---

## Backend

### mlog_backend_t

```c
typedef struct {
    mlog_write_fn  write;   /* output callback (required) */
    void          *ctx;     /* user context */
    mlog_level_t   level;   /* runtime minimum level */
    bool           color;   /* ANSI color (if MLOG_ENABLE_COLOR) */
} mlog_backend_t;
```

### mlog_write_fn

```c
typedef void (*mlog_write_fn)(const char *buf, uint16_t len,
                               mlog_level_t level, void *ctx);
```

Receives a complete, formatted, NUL-terminated log line including newline.
`level` is provided for backends that need level-specific routing (e.g.,
writing errors to a separate file).

---

## Init / config

### mlog_init

```c
void mlog_init(mlog_t *log);
```

Clear all backends, set global level to TRACE, clock to NULL.

### mlog_global

```c
mlog_t *mlog_global(void);
```

Returns the global singleton. Lazy-initialised on first call.

### mlog_set_clock

```c
void mlog_set_clock(mlog_t *log, mlog_clock_fn clock);
```

Set clock for timestamps. Pass NULL to disable. Same signature as
`mres_clock_fn`: `uint32_t (*)(void)` returning milliseconds.

### mlog_set_level

```c
void mlog_set_level(mlog_t *log, mlog_level_t level);
```

Set runtime global filter. Messages below this are suppressed regardless
of backend levels. Pass NULL for log to use global.

### mlog_add_backend

```c
int mlog_add_backend(mlog_t *log, const mlog_backend_t *backend);
```

Register a backend. Returns index (0-based) or -1 if full or invalid.

### mlog_clear_backends

```c
void mlog_clear_backends(mlog_t *log);
```

Remove all backends.

### mlog_backend_set_level

```c
void mlog_backend_set_level(mlog_t *log, uint8_t index, mlog_level_t level);
```

Change a backend's runtime level.

---

## Logging functions

### mlog_log

```c
void mlog_log(mlog_t *log, mlog_level_t level, const char *tag,
              const char *fmt, ...);
```

Log a printf-style message. Pass NULL for `log` to use global. Pass NULL
for `tag` to omit.

### mlog_vlog

```c
void mlog_vlog(mlog_t *log, mlog_level_t level, const char *tag,
               const char *fmt, va_list args);
```

va_list variant for wrapping in other variadic functions.

### mlog_hexdump

```c
void mlog_hexdump(mlog_t *log, mlog_level_t level, const char *tag,
                   const void *data, uint16_t len);
```

Log binary data as hex: `"00 0A FF 42"`.

---

## Convenience macros

```c
MLOG_TRACE(tag, fmt, ...)
MLOG_DEBUG(tag, fmt, ...)
MLOG_INFO(tag, fmt, ...)
MLOG_WARN(tag, fmt, ...)
MLOG_ERROR(tag, fmt, ...)
```

Use the global logger. Compile-time stripped when below `MLOG_LEVEL_MIN`.

---

## Utility functions

```c
const char *mlog_level_name(mlog_level_t level);   /* "ERROR" */
char mlog_level_char(mlog_level_t level);           /* 'E' */
```

---

## Thread safety

microlog is not thread-safe. If logging from multiple threads/tasks,
protect `mlog_log()` with a mutex, or use a thread-safe backend (e.g.,
write to a lock-free ring buffer, drain from one thread).
