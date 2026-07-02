/*
 * microlog — Implementation.
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/microlog
 */

#include "mlog.h"
#include <stdio.h>
#include <string.h>

#define MLOG_LINE_BUF_SIZE ((size_t)MLOG_BUF_SIZE + (size_t)MLOG_LINE_OVERHEAD)

/* ── ANSI color codes ──────────────────────────────────────────────────── */

#if MLOG_ENABLE_COLOR
static const char *level_colors[] = {
    "\033[90m",    /* TRACE: dark gray   */
    "\033[36m",    /* DEBUG: cyan        */
    "\033[32m",    /* INFO:  green       */
    "\033[33m",    /* WARN:  yellow      */
    "\033[31m",    /* ERROR: red         */
};
#define COLOR_RESET "\033[0m"
#endif

/* ── Level names ───────────────────────────────────────────────────────── */

static const char *level_names[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "NONE"
};

static const char level_chars[] = { 'T', 'D', 'I', 'W', 'E', '-' };

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
} mlog_buf_writer_t;

static int mlog_level_is_filter(mlog_level_t level)
{
    return (unsigned int)level <= (unsigned int)MLOG_NONE;
}

static int mlog_level_is_message(mlog_level_t level)
{
    return (unsigned int)level <= (unsigned int)MLOG_ERROR;
}

static size_t mlog_writer_room(const mlog_buf_writer_t *writer)
{
    if (writer->len >= writer->cap) {
        return 0;
    }
    return writer->cap - writer->len;
}

static size_t mlog_writer_room_with_reserve(const mlog_buf_writer_t *writer,
                                            size_t reserve)
{
    size_t room = mlog_writer_room(writer);
    if (room <= reserve) {
        return 0;
    }
    return room - reserve;
}

static void mlog_writer_init(mlog_buf_writer_t *writer, char *buf, size_t size)
{
    writer->buf = buf;
    writer->len = 0;
    writer->cap = (size > 0) ? (size - 1u) : 0u;
    if (size > 0) {
        writer->buf[0] = '\0';
    }
}

static void mlog_writer_terminate(mlog_buf_writer_t *writer)
{
    writer->buf[writer->len] = '\0';
}

static void mlog_writer_append_n(mlog_buf_writer_t *writer, const char *src,
                                 size_t src_len, size_t reserve)
{
    size_t copy = mlog_writer_room_with_reserve(writer, reserve);
    if (copy == 0) {
        return;
    }
    if (copy > src_len) {
        copy = src_len;
    }
    memcpy(writer->buf + writer->len, src, copy);
    writer->len += copy;
    writer->buf[writer->len] = '\0';
}

static void mlog_writer_append_cstr(mlog_buf_writer_t *writer, const char *src,
                                    size_t reserve)
{
    mlog_writer_append_n(writer, src, strlen(src), reserve);
}

static void mlog_writer_append_char(mlog_buf_writer_t *writer, char c,
                                    size_t reserve)
{
    mlog_writer_append_n(writer, &c, 1u, reserve);
}

static size_t mlog_bounded_cstr_len(const char *s, size_t max_len)
{
    size_t len = 0;
    while (len < max_len && s[len] != '\0') {
        len++;
    }
    return len;
}

const char *mlog_level_name(mlog_level_t level)
{
    if (!mlog_level_is_filter(level)) return "?";
    return level_names[level];
}

char mlog_level_char(mlog_level_t level)
{
    if (!mlog_level_is_filter(level)) return '?';
    return level_chars[level];
}

/* ── Global logger singleton ───────────────────────────────────────────── */

static mlog_t g_logger;
static bool g_initialized = false;

mlog_t *mlog_global(void)
{
    if (!g_initialized) {
        mlog_init(&g_logger);
        g_initialized = true;
    }
    return &g_logger;
}

/* ── Init / config ─────────────────────────────────────────────────────── */

void mlog_init(mlog_t *log)
{
    if (log == NULL) return;
    memset(log, 0, sizeof(*log));
    log->global_level = MLOG_TRACE;
    log->clock = NULL;
    log->num_backends = 0;
}

void mlog_set_clock(mlog_t *log, mlog_clock_fn clock)
{
    if (log == NULL) log = mlog_global();
    log->clock = clock;
}

void mlog_set_level(mlog_t *log, mlog_level_t level)
{
    if (log == NULL) log = mlog_global();
    if (!mlog_level_is_filter(level)) return;
    log->global_level = level;
}

int mlog_add_backend(mlog_t *log, const mlog_backend_t *backend)
{
    if (log == NULL) log = mlog_global();
    if (backend == NULL || backend->write == NULL) return -1;
    if (log->num_backends >= MLOG_MAX_BACKENDS) return -1;
    if (!mlog_level_is_filter(backend->level)) return -1;

    log->backends[log->num_backends] = *backend;
    return (int)log->num_backends++;
}

void mlog_clear_backends(mlog_t *log)
{
    if (log == NULL) log = mlog_global();
    log->num_backends = 0;
    memset(log->backends, 0, sizeof(log->backends));
}

void mlog_backend_set_level(mlog_t *log, uint8_t index, mlog_level_t level)
{
    if (log == NULL) log = mlog_global();
    if (!mlog_level_is_filter(level)) return;
    if (index < log->num_backends) {
        log->backends[index].level = level;
    }
}

/* ── Core logging ──────────────────────────────────────────────────────── */

void mlog_vlog(mlog_t *log, mlog_level_t level, const char *tag,
               const char *fmt, va_list args)
{
    if (log == NULL) log = mlog_global();
    if (fmt == NULL) return;
    if (!mlog_level_is_message(level)) return;

    /* Global level filter */
    if (level < log->global_level) return;

    /* Check if any backend cares about this level */
    bool any_interested = false;
    for (uint8_t i = 0; i < log->num_backends; i++) {
        if (level >= log->backends[i].level) {
            any_interested = true;
            break;
        }
    }
    if (!any_interested) return;

    /* Format the user message */
    char msg_buf[MLOG_BUF_SIZE];
    int msg_len = vsnprintf(msg_buf, sizeof(msg_buf), fmt, args);
    if (msg_len < 0) msg_len = 0;
    if ((size_t)msg_len >= sizeof(msg_buf)) msg_len = sizeof(msg_buf) - 1;

    /* Build the output line for each backend */
    for (uint8_t i = 0; i < log->num_backends; i++) {
        mlog_backend_t *be = &log->backends[i];

        if (level < be->level) continue;
        if (be->write == NULL) continue;

        char line_buf[MLOG_LINE_BUF_SIZE];
        mlog_buf_writer_t writer;
        size_t suffix_reserve;
        size_t tag_limit;
        size_t tag_len;
        int emit_color = 0;

        mlog_writer_init(&writer, line_buf, sizeof(line_buf));
        suffix_reserve = 1u;
#if MLOG_ENABLE_COLOR
        emit_color = (be->color != false);
        if (emit_color) {
            suffix_reserve += strlen(COLOR_RESET);
        }
#endif

        /* Color prefix */
#if MLOG_ENABLE_COLOR
        if (emit_color) {
            const char *clr = level_colors[level];
            mlog_writer_append_cstr(&writer, clr, suffix_reserve);
        }
#endif

        /* Timestamp */
#if MLOG_ENABLE_TIMESTAMP
        if (log->clock != NULL) {
            uint32_t ms = log->clock();
            uint32_t sec = ms / 1000;
            uint32_t frac = ms % 1000;
            char ts_buf[16];
            int n = snprintf(ts_buf, sizeof(ts_buf), "%lu.%03lu ",
                             (unsigned long)sec, (unsigned long)frac);
            if (n > 0) {
                size_t ts_len = (size_t)n;
                if (ts_len >= sizeof(ts_buf)) {
                    ts_len = sizeof(ts_buf) - 1u;
                }
                mlog_writer_append_n(&writer, ts_buf, ts_len, suffix_reserve);
            }
        }
#endif

        /* Level char */
        mlog_writer_append_char(&writer, '[', suffix_reserve);
        mlog_writer_append_char(&writer, mlog_level_char(level), suffix_reserve);
        mlog_writer_append_char(&writer, ']', suffix_reserve);

        /* Tag */
        if (tag != NULL && tag[0] != '\0') {
            mlog_writer_append_char(&writer, ' ', suffix_reserve);
            tag_limit = mlog_writer_room_with_reserve(&writer, suffix_reserve);
            tag_len = mlog_bounded_cstr_len(tag, tag_limit);
            mlog_writer_append_n(&writer, tag, tag_len, suffix_reserve);
        }

        /* Separator + message */
        mlog_writer_append_char(&writer, ':', suffix_reserve);
        mlog_writer_append_char(&writer, ' ', suffix_reserve);
        mlog_writer_append_n(&writer, msg_buf, (size_t)msg_len, suffix_reserve);

        /* Color reset */
#if MLOG_ENABLE_COLOR
        if (emit_color) {
            mlog_writer_append_cstr(&writer, COLOR_RESET, 1u);
        }
#endif

        /* Newline */
        mlog_writer_append_char(&writer, '\n', 0u);
        mlog_writer_terminate(&writer);

        be->write(line_buf, (uint16_t)writer.len, level, be->ctx);
    }
}

void mlog_log(mlog_t *log, mlog_level_t level, const char *tag,
              const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    mlog_vlog(log, level, tag, fmt, args);
    va_end(args);
}

/* ── Hex dump ──────────────────────────────────────────────────────────── */

void mlog_hexdump(mlog_t *log, mlog_level_t level, const char *tag,
                   const void *data, uint16_t len)
{
    if (data == NULL || len == 0) return;

    const unsigned char *p = (const unsigned char *)data;
    char hex_buf[MLOG_BUF_SIZE];
    int pos = 0;

    /* Cap to what fits: 3 chars per byte ("XX ") */
    uint16_t max_bytes = (MLOG_BUF_SIZE - 1) / 3;
    if (len > max_bytes) len = max_bytes;

    for (uint16_t i = 0; i < len; i++) {
        if (pos + 3 >= (int)sizeof(hex_buf)) break;
        int n = snprintf(hex_buf + pos, (size_t)(sizeof(hex_buf) - (size_t)pos),
                         "%02X ", p[i]);
        if (n > 0) pos += n;
    }

    /* Remove trailing space */
    if (pos > 0 && hex_buf[pos - 1] == ' ') {
        pos--;
    }
    hex_buf[pos] = '\0';

    mlog_log(log, level, tag, "%s", hex_buf);
}
