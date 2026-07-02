/*
 * microlog — Structured logging for embedded systems.
 *
 * C99 · Zero dependencies · Zero allocations · Pluggable backends · Portable
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/microlog
 */

#ifndef MLOG_H
#define MLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <limits.h>

#if defined(__clang__) || defined(__GNUC__)
#define MLOG_PRINTF_LIKE(fmt_index, first_arg) __attribute__((format(printf, fmt_index, first_arg)))
#else
#define MLOG_PRINTF_LIKE(fmt_index, first_arg)
#endif

/* ── Configuration ─────────────────────────────────────────────────────── */

/** Maximum number of backends that can be registered simultaneously. */
#ifndef MLOG_MAX_BACKENDS
#define MLOG_MAX_BACKENDS 3
#endif

/** Internal formatting buffer size. Messages longer than this are truncated. */
#ifndef MLOG_BUF_SIZE
#define MLOG_BUF_SIZE 128
#endif

/**
 * Compile-time minimum log level. Messages below this level are stripped
 * from the binary entirely — no function call, no string literal.
 *
 * 0 = TRACE (everything), 1 = DEBUG, 2 = INFO, 3 = WARN, 4 = ERROR, 5 = NONE
 */
#ifndef MLOG_LEVEL_MIN
#define MLOG_LEVEL_MIN 0
#endif

/** Enable ANSI color codes in output. Set 0 for raw UART / file output. */
#ifndef MLOG_ENABLE_COLOR
#define MLOG_ENABLE_COLOR 1
#endif

/** Enable timestamps in output. Requires a clock callback. */
#ifndef MLOG_ENABLE_TIMESTAMP
#define MLOG_ENABLE_TIMESTAMP 1
#endif

#define MLOG_LINE_OVERHEAD 32u

#if MLOG_MAX_BACKENDS < 1 || MLOG_MAX_BACKENDS > 255
#error "MLOG_MAX_BACKENDS must be an integer constant in the range 1..255."
#endif

#if MLOG_BUF_SIZE < 2
#error "MLOG_BUF_SIZE must be at least 2 bytes."
#endif

#if ((unsigned long)MLOG_BUF_SIZE + (unsigned long)MLOG_LINE_OVERHEAD) > 65535ul
#error "MLOG_BUF_SIZE is too large: maximum emitted line length must fit in uint16_t."
#endif

#if MLOG_LEVEL_MIN < 0 || MLOG_LEVEL_MIN > 5
#error "MLOG_LEVEL_MIN must be an integer constant in the range 0..5."
#endif

#if MLOG_ENABLE_COLOR != 0 && MLOG_ENABLE_COLOR != 1
#error "MLOG_ENABLE_COLOR must be exactly 0 or 1."
#endif

#if MLOG_ENABLE_TIMESTAMP != 0 && MLOG_ENABLE_TIMESTAMP != 1
#error "MLOG_ENABLE_TIMESTAMP must be exactly 0 or 1."
#endif

/* ── Log levels ────────────────────────────────────────────────────────── */

typedef enum {
    MLOG_TRACE = 0,   /**< Verbose tracing (hot path details).       */
    MLOG_DEBUG = 1,   /**< Development debugging.                    */
    MLOG_INFO  = 2,   /**< Normal operational events.                */
    MLOG_WARN  = 3,   /**< Unexpected but handled conditions.        */
    MLOG_ERROR = 4,   /**< Failures requiring attention.             */
    MLOG_NONE  = 5,   /**< Logging disabled (filter only).           */
} mlog_level_t;

/** Get short name for a level: "TRACE", "DEBUG", "INFO", "WARN", "ERROR". */
const char *mlog_level_name(mlog_level_t level);

/** Get single-char abbreviation: 'T', 'D', 'I', 'W', 'E'. */
char mlog_level_char(mlog_level_t level);

/* ── Platform callback ─────────────────────────────────────────────────── */

/**
 * Clock function — returns current time in milliseconds.
 * Same signature as microres/microfsm. May be NULL (timestamps disabled).
 */
typedef uint32_t (*mlog_clock_fn)(void);

/* ── Backend ───────────────────────────────────────────────────────────── */

/**
 * Backend write callback — outputs a complete, formatted log line.
 *
 * @param buf   NUL-terminated string containing the formatted message.
 * @param len   Length of string (not counting NUL).
 * @param level Log level of this message.
 * @param ctx   User context from mlog_backend_t.
 */
typedef void (*mlog_write_fn)(const char *buf, uint16_t len,
                               mlog_level_t level, void *ctx);

/**
 * Backend descriptor.
 */
typedef struct {
    mlog_write_fn  write;     /**< Output callback (required).           */
    void          *ctx;       /**< User context passed to write.         */
    mlog_level_t   level;     /**< Runtime minimum level for this backend.*/
    bool           color;     /**< Enable ANSI color codes.              */
} mlog_backend_t;

/* ── Logger instance ───────────────────────────────────────────────────── */

/**
 * Logger state. Typically one global instance per application.
 * Can also be used as multiple independent loggers.
 */
typedef struct {
    mlog_backend_t  backends[MLOG_MAX_BACKENDS];
    uint8_t         num_backends;
    mlog_level_t    global_level;   /**< Runtime global filter.           */
    mlog_clock_fn   clock;          /**< Optional clock for timestamps.   */
} mlog_t;

/* ── Global logger ─────────────────────────────────────────────────────── */

/**
 * Get pointer to the global logger singleton.
 * Initialised to zero (no backends, TRACE level, no clock).
 */
mlog_t *mlog_global(void);

/* ── Init / config ─────────────────────────────────────────────────────── */

/**
 * Initialise a logger instance.
 * Clears all backends, sets global level to TRACE, clock to NULL.
 */
void mlog_init(mlog_t *log);

/**
 * Set the clock function for timestamps.
 */
void mlog_set_clock(mlog_t *log, mlog_clock_fn clock);

/**
 * Set the runtime global level filter.
 * Messages below this level are suppressed (even if a backend allows them).
 */
void mlog_set_level(mlog_t *log, mlog_level_t level);

/**
 * Register a backend. Returns backend index (0-based) or -1 if full.
 */
int mlog_add_backend(mlog_t *log, const mlog_backend_t *backend);

/**
 * Remove all backends.
 */
void mlog_clear_backends(mlog_t *log);

/**
 * Change a backend's runtime level filter.
 */
void mlog_backend_set_level(mlog_t *log, uint8_t index, mlog_level_t level);

/* ── Core logging ──────────────────────────────────────────────────────── */

/**
 * Log a message (printf-style).
 *
 * @param log    Logger instance (or NULL for global).
 * @param level  Log level.
 * @param tag    Module tag (e.g., "MQTT", "FSM", "CONF"). May be NULL.
 * @param fmt    printf format string.
 * @param ...    Format arguments.
 */
void mlog_log(mlog_t *log, mlog_level_t level, const char *tag,
              const char *fmt, ...) MLOG_PRINTF_LIKE(4, 5);

/** va_list variant of mlog_log. */
void mlog_vlog(mlog_t *log, mlog_level_t level, const char *tag,
               const char *fmt, va_list args) MLOG_PRINTF_LIKE(4, 0);

/* ── Convenience macros (use global logger) ────────────────────────────── */

/**
 * Compile-time filtered logging macros.
 *
 * Usage:
 *   MLOG_ERROR("MQTT", "Connect failed: %d", err);
 *   MLOG_INFO("FSM", "Transition: %s -> %s", from, to);
 *   MLOG_DEBUG("SENS", "Raw ADC: %u", adc_val);
 *
 * Messages below MLOG_LEVEL_MIN are completely stripped from the binary.
 */

#if MLOG_LEVEL_MIN <= 0
#define MLOG_TRACE(...) \
    mlog_log(NULL, MLOG_TRACE, __VA_ARGS__)
#else
#define MLOG_TRACE(tag, fmt, ...) ((void)0)
#endif

#if MLOG_LEVEL_MIN <= 1
#define MLOG_DEBUG(...) \
    mlog_log(NULL, MLOG_DEBUG, __VA_ARGS__)
#else
#define MLOG_DEBUG(tag, fmt, ...) ((void)0)
#endif

#if MLOG_LEVEL_MIN <= 2
#define MLOG_INFO(...) \
    mlog_log(NULL, MLOG_INFO, __VA_ARGS__)
#else
#define MLOG_INFO(tag, fmt, ...) ((void)0)
#endif

#if MLOG_LEVEL_MIN <= 3
#define MLOG_WARN(...) \
    mlog_log(NULL, MLOG_WARN, __VA_ARGS__)
#else
#define MLOG_WARN(tag, fmt, ...) ((void)0)
#endif

#if MLOG_LEVEL_MIN <= 4
#define MLOG_ERROR(...) \
    mlog_log(NULL, MLOG_ERROR, __VA_ARGS__)
#else
#define MLOG_ERROR(tag, fmt, ...) ((void)0)
#endif

/* ── Instance-specific macros ──────────────────────────────────────────── */

#define MLOG_LOG(logger, level, ...) \
    mlog_log((logger), (level), __VA_ARGS__)

/* ── Hex dump utility ──────────────────────────────────────────────────── */

/**
 * Log a hex dump of a buffer. Useful for protocol debugging.
 * Output: "TAG [I] 00 01 02 0A FF ..."
 *
 * @param log    Logger instance (or NULL for global).
 * @param level  Log level.
 * @param tag    Module tag.
 * @param data   Buffer to dump.
 * @param len    Length in bytes (capped at MLOG_BUF_SIZE/3).
 */
void mlog_hexdump(mlog_t *log, mlog_level_t level, const char *tag,
                   const void *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* MLOG_H */
