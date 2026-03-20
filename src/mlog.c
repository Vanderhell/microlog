/*
 * microlog — Implementation.
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/microlog
 */

#include "mlog.h"
#include <stdio.h>
#include <string.h>

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

const char *mlog_level_name(mlog_level_t level)
{
    if (level > MLOG_NONE) return "?";
    return level_names[level];
}

char mlog_level_char(mlog_level_t level)
{
    if (level > MLOG_NONE) return '?';
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
    log->global_level = level;
}

int mlog_add_backend(mlog_t *log, const mlog_backend_t *backend)
{
    if (log == NULL) log = mlog_global();
    if (backend == NULL || backend->write == NULL) return -1;
    if (log->num_backends >= MLOG_MAX_BACKENDS) return -1;

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

        char line_buf[MLOG_BUF_SIZE + 64]; /* extra room for prefix */
        int pos = 0;
        int remaining;

        /* Color prefix */
#if MLOG_ENABLE_COLOR
        if (be->color && level <= MLOG_ERROR) {
            const char *clr = level_colors[level];
            int clen = (int)strlen(clr);
            if (pos + clen < (int)sizeof(line_buf)) {
                memcpy(line_buf + pos, clr, (size_t)clen);
                pos += clen;
            }
        }
#endif

        /* Timestamp */
#if MLOG_ENABLE_TIMESTAMP
        if (log->clock != NULL) {
            uint32_t ms = log->clock();
            uint32_t sec = ms / 1000;
            uint32_t frac = ms % 1000;
            remaining = (int)sizeof(line_buf) - pos;
            if (remaining > 0) {
                int n = snprintf(line_buf + pos, (size_t)remaining,
                                 "%lu.%03lu ", (unsigned long)sec,
                                 (unsigned long)frac);
                if (n > 0) pos += (n < remaining) ? n : remaining - 1;
            }
        }
#endif

        /* Level char */
        remaining = (int)sizeof(line_buf) - pos;
        if (remaining > 3) {
            line_buf[pos++] = '[';
            line_buf[pos++] = mlog_level_char(level);
            line_buf[pos++] = ']';
        }

        /* Tag */
        if (tag != NULL && tag[0] != '\0') {
            remaining = (int)sizeof(line_buf) - pos;
            if (remaining > 1) {
                line_buf[pos++] = ' ';
                int tlen = (int)strlen(tag);
                if (tlen > remaining - 1) tlen = remaining - 1;
                memcpy(line_buf + pos, tag, (size_t)tlen);
                pos += tlen;
            }
        }

        /* Separator + message */
        remaining = (int)sizeof(line_buf) - pos;
        if (remaining > 2) {
            line_buf[pos++] = ':';
            line_buf[pos++] = ' ';
        }

        remaining = (int)sizeof(line_buf) - pos;
        if (remaining > 1) {
            int copy = msg_len;
            if (copy > remaining - 1) copy = remaining - 1;
            memcpy(line_buf + pos, msg_buf, (size_t)copy);
            pos += copy;
        }

        /* Color reset */
#if MLOG_ENABLE_COLOR
        if (be->color) {
            remaining = (int)sizeof(line_buf) - pos;
            int rlen = (int)strlen(COLOR_RESET);
            if (remaining > rlen) {
                memcpy(line_buf + pos, COLOR_RESET, (size_t)rlen);
                pos += rlen;
            }
        }
#endif

        /* Newline */
        remaining = (int)sizeof(line_buf) - pos;
        if (remaining > 1) {
            line_buf[pos++] = '\n';
        }

        line_buf[pos] = '\0';

        be->write(line_buf, (uint16_t)pos, level, be->ctx);
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

    const uint8_t *p = (const uint8_t *)data;
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
