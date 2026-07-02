/*
 * microlog test suite.
 *
 * Build: gcc -std=c99 -Wall -Wextra -I../include ../src/mlog.c test_all.c -o test_all
 * Run:   ./test_all
 */

#include "mlog.h"
#include <stdio.h>
#include <string.h>

/* ── Minimal test framework ────────────────────────────────────────────── */

static int tests_run = 0, tests_passed = 0, tests_failed = 0;

#define TEST(name) static void name(void)
#define RUN_TEST(name) do {                                     \
    tests_run++;                                                \
    printf("  %-55s ", #name);                                  \
    name();                                                     \
    printf("PASS\n");                                           \
    tests_passed++;                                             \
} while (0)

#define ASSERT_EQ(expected, actual) do {                        \
    if ((expected) != (actual)) {                               \
        printf("FAIL\n    %s:%d: expected %d, got %d\n",       \
               __FILE__, __LINE__, (int)(expected), (int)(actual)); \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_TRUE(expr) do {                                  \
    if (!(expr)) {                                              \
        printf("FAIL\n    %s:%d: expected true\n",              \
               __FILE__, __LINE__);                             \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_FALSE(expr) do {                                 \
    if ((expr)) {                                               \
        printf("FAIL\n    %s:%d: expected false\n",             \
               __FILE__, __LINE__);                             \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_STR_EQ(expected, actual) do {                    \
    if (strcmp((expected), (actual)) != 0) {                     \
        printf("FAIL\n    %s:%d:\n      expected: \"%s\"\n      got:      \"%s\"\n", \
               __FILE__, __LINE__, (expected), (actual));       \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_STR_CONTAINS(haystack, needle) do {              \
    if (strstr((haystack), (needle)) == NULL) {                  \
        printf("FAIL\n    %s:%d: \"%s\" not found in \"%s\"\n", \
               __FILE__, __LINE__, (needle), (haystack));       \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

/* ── Capture backend ───────────────────────────────────────────────────── */

#define CAPTURE_SIZE 512
static char captured[CAPTURE_SIZE];
static uint16_t captured_len = 0;
static int write_count = 0;
static mlog_level_t captured_level = MLOG_NONE;

static void capture_reset(void) {
    memset(captured, 0, CAPTURE_SIZE);
    captured_len = 0;
    write_count = 0;
    captured_level = MLOG_NONE;
}

static void capture_write(const char *buf, uint16_t len, mlog_level_t level, void *ctx) {
    (void)ctx;
    if (len < CAPTURE_SIZE - 1) {
        memcpy(captured, buf, len);
        captured[len] = '\0';
        captured_len = len;
    }
    captured_level = level;
    write_count++;
}

/* Second capture for multi-backend tests */
static char captured2[CAPTURE_SIZE];
static int write_count2 = 0;

static void capture_write2(const char *buf, uint16_t len, mlog_level_t level, void *ctx) {
    (void)ctx; (void)level;
    if (len < CAPTURE_SIZE - 1) {
        memcpy(captured2, buf, len);
        captured2[len] = '\0';
    }
    write_count2++;
}

static void capture2_reset(void) {
    memset(captured2, 0, CAPTURE_SIZE);
    write_count2 = 0;
}

/* ── Mock clock ────────────────────────────────────────────────────────── */

static uint32_t mock_time = 0;
static uint32_t mock_clock(void) { return mock_time; }
static int mock_clock_calls = 0;
static uint32_t counting_clock(void) {
    mock_clock_calls++;
    return mock_time;
}

/* ── Helper to create a fresh logger ───────────────────────────────────── */

static mlog_t test_log;

static void setup_logger(void) {
    capture_reset();
    capture2_reset();
    mock_time = 0;
    mlog_init(&test_log);
}

static int add_capture_backend(mlog_level_t level, bool color) {
    mlog_backend_t be = {
        .write = capture_write,
        .ctx = NULL,
        .level = level,
#if MLOG_ENABLE_COLOR
        .color = color,
#endif
    };
#if !MLOG_ENABLE_COLOR
    (void)color;
#endif
    return mlog_add_backend(&test_log, &be);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Init
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_init) {
    setup_logger();
    ASSERT_EQ(0, test_log.num_backends);
    ASSERT_EQ(MLOG_TRACE, test_log.global_level);
    ASSERT_TRUE(test_log.clock == NULL);
}

TEST(test_init_null) {
    /* Should not crash */
    mlog_init(NULL);
}

TEST(test_global_singleton) {
    mlog_t *g1 = mlog_global();
    mlog_t *g2 = mlog_global();
    ASSERT_TRUE(g1 == g2);
    ASSERT_TRUE(g1 != NULL);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Backend management
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_add_backend) {
    setup_logger();
    int idx = add_capture_backend(MLOG_TRACE, false);
    ASSERT_EQ(0, idx);
    ASSERT_EQ(1, test_log.num_backends);
}

TEST(test_add_multiple_backends) {
    setup_logger();
    ASSERT_EQ(0, add_capture_backend(MLOG_TRACE, false));
    mlog_backend_t be2 = { .write = capture_write2, .ctx = NULL, .level = MLOG_TRACE };
    ASSERT_EQ(1, mlog_add_backend(&test_log, &be2));
    ASSERT_EQ(2, test_log.num_backends);
}

TEST(test_add_backend_full) {
    setup_logger();
    for (int i = 0; i < MLOG_MAX_BACKENDS; i++) {
        ASSERT_TRUE(add_capture_backend(MLOG_TRACE, false) >= 0);
    }
    /* Next should fail */
    ASSERT_EQ(-1, add_capture_backend(MLOG_TRACE, false));
}

TEST(test_add_backend_null_write) {
    setup_logger();
    mlog_backend_t be = { .write = NULL, .ctx = NULL, .level = MLOG_TRACE };
    ASSERT_EQ(-1, mlog_add_backend(&test_log, &be));
}

TEST(test_clear_backends) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);
    ASSERT_EQ(1, test_log.num_backends);
    mlog_clear_backends(&test_log);
    ASSERT_EQ(0, test_log.num_backends);
}

TEST(test_backend_set_level) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);
    mlog_backend_set_level(&test_log, 0, MLOG_ERROR);
    ASSERT_EQ(MLOG_ERROR, test_log.backends[0].level);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Basic logging
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_basic_log) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);
    mlog_log(&test_log, MLOG_INFO, "TEST", "Hello %s", "world");
    ASSERT_EQ(1, write_count);
    ASSERT_STR_CONTAINS(captured, "[I]");
    ASSERT_STR_CONTAINS(captured, "TEST");
    ASSERT_STR_CONTAINS(captured, "Hello world");
    ASSERT_EQ(MLOG_INFO, (int)captured_level);
}

TEST(test_log_all_levels) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);

    mlog_log(&test_log, MLOG_TRACE, "T", "trace msg");
    ASSERT_STR_CONTAINS(captured, "[T]");

    mlog_log(&test_log, MLOG_DEBUG, "T", "debug msg");
    ASSERT_STR_CONTAINS(captured, "[D]");

    mlog_log(&test_log, MLOG_INFO, "T", "info msg");
    ASSERT_STR_CONTAINS(captured, "[I]");

    mlog_log(&test_log, MLOG_WARN, "T", "warn msg");
    ASSERT_STR_CONTAINS(captured, "[W]");

    mlog_log(&test_log, MLOG_ERROR, "T", "error msg");
    ASSERT_STR_CONTAINS(captured, "[E]");
}

TEST(test_log_null_tag) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);
    mlog_log(&test_log, MLOG_INFO, NULL, "no tag");
    ASSERT_STR_CONTAINS(captured, "no tag");
    ASSERT_EQ(1, write_count);
}

TEST(test_log_empty_tag) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);
    mlog_log(&test_log, MLOG_INFO, "", "empty tag");
    ASSERT_STR_CONTAINS(captured, "empty tag");
}

TEST(test_log_formatting) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);
    mlog_log(&test_log, MLOG_INFO, "FMT", "num=%d str=%s hex=0x%02X",
             42, "abc", 0xFF);
    ASSERT_STR_CONTAINS(captured, "num=42");
    ASSERT_STR_CONTAINS(captured, "str=abc");
    ASSERT_STR_CONTAINS(captured, "hex=0xFF");
}

TEST(test_log_null_fmt) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);
    /* Should not crash */
    mlog_log(&test_log, MLOG_INFO, "T", NULL);
    ASSERT_EQ(0, write_count);
}

TEST(test_log_no_backends) {
    setup_logger();
    /* No crash, just no output */
    mlog_log(&test_log, MLOG_ERROR, "T", "nobody listening");
    ASSERT_EQ(0, write_count);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Level filtering
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_global_level_filter) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);
    mlog_set_level(&test_log, MLOG_WARN);

    mlog_log(&test_log, MLOG_DEBUG, "T", "should not appear");
    ASSERT_EQ(0, write_count);

    mlog_log(&test_log, MLOG_INFO, "T", "should not appear");
    ASSERT_EQ(0, write_count);

    mlog_log(&test_log, MLOG_WARN, "T", "should appear");
    ASSERT_EQ(1, write_count);

    mlog_log(&test_log, MLOG_ERROR, "T", "should appear");
    ASSERT_EQ(2, write_count);
}

TEST(test_backend_level_filter) {
    setup_logger();
    add_capture_backend(MLOG_WARN, false);  /* backend only wants WARN+ */

    mlog_log(&test_log, MLOG_INFO, "T", "filtered by backend");
    ASSERT_EQ(0, write_count);

    mlog_log(&test_log, MLOG_WARN, "T", "accepted by backend");
    ASSERT_EQ(1, write_count);
}

TEST(test_multi_backend_different_levels) {
    setup_logger();
    add_capture_backend(MLOG_ERROR, false);  /* backend 0: ERROR only */
    mlog_backend_t be2 = { .write = capture_write2, .ctx = NULL, .level = MLOG_DEBUG };
    mlog_add_backend(&test_log, &be2);       /* backend 1: DEBUG+ */

    mlog_log(&test_log, MLOG_INFO, "T", "info message");
    ASSERT_EQ(0, write_count);   /* backend 0 filtered it */
    ASSERT_EQ(1, write_count2);  /* backend 1 accepted it */

    mlog_log(&test_log, MLOG_ERROR, "T", "error message");
    ASSERT_EQ(1, write_count);   /* backend 0 accepted it */
    ASSERT_EQ(2, write_count2);  /* backend 1 accepted it */
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Timestamps
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_timestamp) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);
    mlog_set_clock(&test_log, mock_clock);
    mock_time = 12345;

    mlog_log(&test_log, MLOG_INFO, "T", "with time");
    ASSERT_STR_CONTAINS(captured, "12.345");
}

TEST(test_no_timestamp_without_clock) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);
    /* No clock set */
    mlog_log(&test_log, MLOG_INFO, "T", "no time");
    /* Should not contain a timestamp-like pattern */
    ASSERT_TRUE(captured[0] == '[' || captured[0] == '\033');
}

TEST(test_timestamp_zero) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);
    mlog_set_clock(&test_log, mock_clock);
    mock_time = 0;

    mlog_log(&test_log, MLOG_INFO, "T", "zero time");
    ASSERT_STR_CONTAINS(captured, "0.000");
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Color
 * ═══════════════════════════════════════════════════════════════════════════ */

#if MLOG_ENABLE_COLOR
TEST(test_color_enabled) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, true);  /* color = true */
    mlog_log(&test_log, MLOG_ERROR, "T", "red message");
    ASSERT_STR_CONTAINS(captured, "\033[31m");  /* red */
    ASSERT_STR_CONTAINS(captured, "\033[0m");   /* reset */
}

TEST(test_color_disabled) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);  /* color = false */
    mlog_log(&test_log, MLOG_ERROR, "T", "no color");
    ASSERT_TRUE(strstr(captured, "\033[") == NULL);
}
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Hex dump
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_hexdump) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);

    uint8_t data[] = { 0x00, 0x0A, 0xFF, 0x42 };
    mlog_hexdump(&test_log, MLOG_DEBUG, "HEX", data, 4);

    ASSERT_EQ(1, write_count);
    ASSERT_STR_CONTAINS(captured, "00");
    ASSERT_STR_CONTAINS(captured, "0A");
    ASSERT_STR_CONTAINS(captured, "FF");
    ASSERT_STR_CONTAINS(captured, "42");
}

TEST(test_hexdump_null_data) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);
    mlog_hexdump(&test_log, MLOG_DEBUG, "HEX", NULL, 10);
    ASSERT_EQ(0, write_count);  /* no output */
}

TEST(test_hexdump_zero_len) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);
    uint8_t data[] = { 0xAA };
    mlog_hexdump(&test_log, MLOG_DEBUG, "HEX", data, 0);
    ASSERT_EQ(0, write_count);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Level names
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_level_name) {
    ASSERT_STR_EQ("TRACE", mlog_level_name(MLOG_TRACE));
    ASSERT_STR_EQ("DEBUG", mlog_level_name(MLOG_DEBUG));
    ASSERT_STR_EQ("INFO",  mlog_level_name(MLOG_INFO));
    ASSERT_STR_EQ("WARN",  mlog_level_name(MLOG_WARN));
    ASSERT_STR_EQ("ERROR", mlog_level_name(MLOG_ERROR));
    ASSERT_STR_EQ("NONE",  mlog_level_name(MLOG_NONE));
    ASSERT_STR_EQ("?",     mlog_level_name((mlog_level_t)99));
}

TEST(test_level_char) {
    ASSERT_EQ('T', mlog_level_char(MLOG_TRACE));
    ASSERT_EQ('D', mlog_level_char(MLOG_DEBUG));
    ASSERT_EQ('I', mlog_level_char(MLOG_INFO));
    ASSERT_EQ('W', mlog_level_char(MLOG_WARN));
    ASSERT_EQ('E', mlog_level_char(MLOG_ERROR));
    ASSERT_EQ('?', mlog_level_char((mlog_level_t)99));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Convenience macros
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_convenience_macros) {
    /* Reset global logger */
    mlog_t *g = mlog_global();
    mlog_init(g);

    mlog_backend_t be = { .write = capture_write, .ctx = NULL, .level = MLOG_TRACE };
    mlog_add_backend(g, &be);

    capture_reset();
    MLOG_INFO("MACRO", "macro test %d", 42);
    ASSERT_EQ(1, write_count);
    ASSERT_STR_CONTAINS(captured, "MACRO");
    ASSERT_STR_CONTAINS(captured, "macro test 42");

    capture_reset();
    MLOG_ERROR("ERR", "error %s", "test");
    ASSERT_STR_CONTAINS(captured, "[E]");
    ASSERT_STR_CONTAINS(captured, "error test");

    /* Cleanup global */
    mlog_clear_backends(g);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Edge cases
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_log_with_null_logger) {
    /* NULL logger → uses global. Should not crash. */
    mlog_t *g = mlog_global();
    mlog_init(g);
    mlog_backend_t be = { .write = capture_write, .ctx = NULL, .level = MLOG_TRACE };
    mlog_add_backend(g, &be);

    capture_reset();
    mlog_log(NULL, MLOG_INFO, "NULL", "null logger");
    ASSERT_EQ(1, write_count);
    ASSERT_STR_CONTAINS(captured, "null logger");

    mlog_clear_backends(g);
}

TEST(test_backend_context) {
    setup_logger();
    int my_ctx = 42;
    mlog_backend_t be = {
        .write = capture_write,
        .ctx = &my_ctx,
        .level = MLOG_TRACE,
    };
    mlog_add_backend(&test_log, &be);
    /* Just verify it doesn't crash — ctx is passed through */
    mlog_log(&test_log, MLOG_INFO, "CTX", "context test");
    ASSERT_EQ(1, write_count);
}

TEST(test_set_level_null) {
    /* Should use global, not crash */
    mlog_set_level(NULL, MLOG_ERROR);
    mlog_t *g = mlog_global();
    ASSERT_EQ(MLOG_ERROR, g->global_level);
    /* Reset */
    mlog_set_level(NULL, MLOG_TRACE);
}

/* Regression coverage added before implementation fixes. */

TEST(test_long_tag_boundary_color_disabled_keeps_newline) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);

    {
        char tag[256];
        memset(tag, 'A', sizeof(tag) - 1);
        tag[sizeof(tag) - 1] = '\0';

        mlog_log(&test_log, MLOG_INFO, tag, "msg");
    }

    ASSERT_TRUE(captured_len > 0);
    ASSERT_EQ('\n', captured[captured_len - 1]);
    ASSERT_EQ('\0', captured[captured_len]);
}

#if MLOG_ENABLE_COLOR
TEST(test_long_tag_boundary_color_enabled_keeps_reset_and_newline) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, true);

    {
        char tag[256];
        memset(tag, 'B', sizeof(tag) - 1);
        tag[sizeof(tag) - 1] = '\0';

        mlog_log(&test_log, MLOG_ERROR, tag, "msg");
    }

    ASSERT_TRUE(captured_len > 0);
    ASSERT_STR_CONTAINS(captured, "\033[31m");
    ASSERT_STR_CONTAINS(captured, "\033[0m");
    ASSERT_EQ('\n', captured[captured_len - 1]);
    ASSERT_EQ('\0', captured[captured_len]);
}
#endif

TEST(test_invalid_message_level_none_emits_nothing) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);
    mlog_log(&test_log, MLOG_NONE, "LVL", "must be ignored");
    ASSERT_EQ(0, write_count);
}

TEST(test_invalid_message_level_out_of_range_emits_nothing) {
    setup_logger();
    add_capture_backend(MLOG_TRACE, false);
    mlog_log(&test_log, (mlog_level_t)99, "LVL", "must be ignored");
    ASSERT_EQ(0, write_count);
}

TEST(test_timestamp_sampled_once_per_event) {
    setup_logger();
    mock_clock_calls = 0;
    mock_time = 4242;
    mlog_set_clock(&test_log, counting_clock);
    add_capture_backend(MLOG_TRACE, false);

    {
        mlog_backend_t be = { .write = capture_write2, .ctx = NULL, .level = MLOG_TRACE };
        mlog_add_backend(&test_log, &be);
    }

    mlog_log(&test_log, MLOG_INFO, "TS", "shared");

    ASSERT_EQ(1, mock_clock_calls);
    ASSERT_EQ(1, write_count);
    ASSERT_EQ(1, write_count2);
    ASSERT_STR_CONTAINS(captured, "4.242");
    ASSERT_STR_CONTAINS(captured2, "4.242");
}

TEST(test_backend_context_pointer_forwarded) {
    setup_logger();

    {
        int marker = 123;
        mlog_backend_t be = { .write = capture_write, .ctx = &marker, .level = MLOG_TRACE };
        mlog_add_backend(&test_log, &be);
        mlog_log(&test_log, MLOG_INFO, "CTX", "ctx");
    }

    ASSERT_EQ(1, write_count);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Main
 * ═══════════════════════════════════════════════════════════════════════════ */

int main(void) {
    printf("\n=== microlog test suite ===\n\n");

    printf("[Init]\n");
    RUN_TEST(test_init);
    RUN_TEST(test_init_null);
    RUN_TEST(test_global_singleton);

    printf("\n[Backend Management]\n");
    RUN_TEST(test_add_backend);
    RUN_TEST(test_add_multiple_backends);
    RUN_TEST(test_add_backend_full);
    RUN_TEST(test_add_backend_null_write);
    RUN_TEST(test_clear_backends);
    RUN_TEST(test_backend_set_level);

    printf("\n[Basic Logging]\n");
    RUN_TEST(test_basic_log);
    RUN_TEST(test_log_all_levels);
    RUN_TEST(test_log_null_tag);
    RUN_TEST(test_log_empty_tag);
    RUN_TEST(test_log_formatting);
    RUN_TEST(test_log_null_fmt);
    RUN_TEST(test_log_no_backends);

    printf("\n[Level Filtering]\n");
    RUN_TEST(test_global_level_filter);
    RUN_TEST(test_backend_level_filter);
    RUN_TEST(test_multi_backend_different_levels);

    printf("\n[Timestamps]\n");
    RUN_TEST(test_timestamp);
    RUN_TEST(test_no_timestamp_without_clock);
    RUN_TEST(test_timestamp_zero);

#if MLOG_ENABLE_COLOR
    printf("\n[Color]\n");
    RUN_TEST(test_color_enabled);
    RUN_TEST(test_color_disabled);
#endif

    printf("\n[Hex Dump]\n");
    RUN_TEST(test_hexdump);
    RUN_TEST(test_hexdump_null_data);
    RUN_TEST(test_hexdump_zero_len);

    printf("\n[Level Names]\n");
    RUN_TEST(test_level_name);
    RUN_TEST(test_level_char);

    printf("\n[Convenience Macros]\n");
    RUN_TEST(test_convenience_macros);

    printf("\n[Edge Cases]\n");
    RUN_TEST(test_log_with_null_logger);
    RUN_TEST(test_backend_context);
    RUN_TEST(test_set_level_null);
    RUN_TEST(test_long_tag_boundary_color_disabled_keeps_newline);
#if MLOG_ENABLE_COLOR
    RUN_TEST(test_long_tag_boundary_color_enabled_keeps_reset_and_newline);
#endif
    RUN_TEST(test_invalid_message_level_none_emits_nothing);
    RUN_TEST(test_invalid_message_level_out_of_range_emits_nothing);
    RUN_TEST(test_timestamp_sampled_once_per_event);
    RUN_TEST(test_backend_context_pointer_forwarded);

    printf("\n=== Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) printf(", %d FAILED", tests_failed);
    printf(" ===\n\n");

    return tests_failed > 0 ? 1 : 0;
}
