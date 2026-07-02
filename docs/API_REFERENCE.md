# API Reference

Header: `#include "mlog.h"`

## Configuration Macros

| Macro | Default | Valid Range | Notes |
|---|---:|---|---|
| `MLOG_MAX_BACKENDS` | `3` | `1..255` | Must match producer and consumer ABI |
| `MLOG_BUF_SIZE` | `128` | `>= 2` | Stack buffer for formatted message text |
| `MLOG_LEVEL_MIN` | `0` | `0..5` | Compile-time stripping threshold |
| `MLOG_ENABLE_COLOR` | `1` | `0` or `1` | Keeps ABI field even when disabled |
| `MLOG_ENABLE_TIMESTAMP` | `1` | `0` or `1` | Controls timestamp formatting code |

Invalid values intentionally fail compilation with `#error`.

## Levels

| Symbol | Value | Meaning |
|---|---:|---|
| `MLOG_TRACE` | `0` | Verbose tracing |
| `MLOG_DEBUG` | `1` | Debugging |
| `MLOG_INFO` | `2` | Normal events |
| `MLOG_WARN` | `3` | Handled problems |
| `MLOG_ERROR` | `4` | Failures requiring attention |
| `MLOG_NONE` | `5` | Filter only, never a message level |

## Output Format

Typical output:

```text
12.345 [I] MQTT: Connected
[E] CONF: CRC failed
```

Rules:

- Each emitted record ends in `\n`.
- If color is enabled for a backend and a color prefix is emitted, the ANSI reset sequence is emitted before the newline.
- Truncation preserves the terminating NUL and the final newline.
- Timestamp text is sampled once per log event and reused for every interested backend.

## Callbacks and Ownership

```c
typedef uint32_t (*mlog_clock_fn)(void);
typedef void (*mlog_write_fn)(const char *buf, uint16_t len, mlog_level_t level, void *ctx);
```

- `mlog_clock_fn` defines wraparound, epoch, and monotonicity.
- `mlog_write_fn` is synchronous.
- `buf` is borrowed stack storage valid only during the callback.
- `ctx` is borrowed caller-owned context.
- microlog does not propagate backend write failures.

## Public Types

```c
typedef struct {
    mlog_write_fn  write;
    void          *ctx;
    mlog_level_t   level;
    bool           color;
} mlog_backend_t;
```

`mlog_backend_t` is copied by value into the logger. The `ctx` pointer must remain valid until the backend is cleared or the logger is no longer used.

```c
typedef struct {
    mlog_backend_t  backends[MLOG_MAX_BACKENDS];
    uint8_t         num_backends;
    mlog_level_t    global_level;
    mlog_clock_fn   clock;
} mlog_t;
```

## Functions

`mlog_t *mlog_global(void);`

- Returns the static global logger.
- Static zero initialization is the default state.

`void mlog_init(mlog_t *log);`

- Clears backends.
- Sets global level to `MLOG_TRACE`.
- Clears the clock callback.

`void mlog_set_clock(mlog_t *log, mlog_clock_fn clock);`

- Assigns the clock callback.
- `NULL` disables timestamps.

`void mlog_set_level(mlog_t *log, mlog_level_t level);`

- Sets the runtime global filter.
- Invalid values are ignored.

`int mlog_add_backend(mlog_t *log, const mlog_backend_t *backend);`

- Copies the backend descriptor by value.
- Returns the backend index or `-1`.
- Rejects invalid levels and missing callbacks.

`void mlog_clear_backends(mlog_t *log);`

- Clears backend slots.

`void mlog_backend_set_level(mlog_t *log, uint8_t index, mlog_level_t level);`

- Updates one backend filter.
- Invalid levels are ignored.

`void mlog_log(mlog_t *log, mlog_level_t level, const char *tag, const char *fmt, ...);`

- Logs one record.
- Accepts only `TRACE` through `ERROR` as message levels.
- Ignores invalid levels and `MLOG_NONE`.

`void mlog_vlog(mlog_t *log, mlog_level_t level, const char *tag, const char *fmt, va_list args);`

- `va_list` form of `mlog_log`.

`void mlog_hexdump(mlog_t *log, mlog_level_t level, const char *tag, const void *data, uint16_t len);`

- Dumps bytes in source order, for example `00 0A FF 42`.
- Uses `const unsigned char *` for byte-wise access.

## Macros

```c
MLOG_TRACE(...)
MLOG_DEBUG(...)
MLOG_INFO(...)
MLOG_WARN(...)
MLOG_ERROR(...)
MLOG_LOG(logger, level, ...)
```

- Active macros evaluate each argument exactly once.
- Compile-time-disabled macros evaluate none of their arguments.
- The macros are safe in ordinary statement contexts such as `if/else`.

## Limits

- No internal locking.
- Not safe for recursive backend logging.
- Not ISR-safe, async-signal-safe, durable, or reset-safe.
- Stack usage in `mlog_vlog` is bounded by:

```text
sizeof(msg_buf) + sizeof(line_buf) + small fixed locals
= MLOG_BUF_SIZE + (MLOG_BUF_SIZE + MLOG_LINE_OVERHEAD) + O(1)
```
