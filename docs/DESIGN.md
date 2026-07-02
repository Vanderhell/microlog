# Design Rationale

Why microlog is built the way it is.

---

## 1. Global singleton + instances

**Decision:** microlog provides a global singleton via `mlog_global()` for
convenience, plus the ability to create independent `mlog_t` instances.

**Why:**

- 95% of projects need one logger. A global makes the `MLOG_INFO()` macros
  zero-config: include the header and start logging.
- The remaining 5% need isolation: test harnesses that capture output,
  library code that shouldn't touch the application's logger, or multiple
  subsystems with independent routing.
- The global relies on static zero initialisation. No explicit init needed.

---

## 2. Backend callback vs FILE* / fd

**Decision:** Output goes through a user-provided `write` callback, not
`fprintf` or `write()`.

**Why:**

- Bare-metal has no `FILE*`. Many MCUs route `printf` through semihosting
  which is unusably slow.
- UART, SWD/RTT, USB CDC, BLE characteristics, ring buffers, flash logs —
  each needs a different output path. Callbacks handle all of them.
- Multiple backends with different filters are natural: UART gets DEBUG+,
  ring buffer gets ERROR only.

---

## 3. Two-level filtering (compile-time + runtime)

**Decision:** `MLOG_LEVEL_MIN` strips code at compile time. `global_level`
and per-backend `level` filter at runtime.

**Why:**

- **Compile-time:** Production firmware should not contain debug strings
  or call overhead. Setting `MLOG_LEVEL_MIN=2` eliminates all TRACE and
  DEBUG code — the `MLOG_DEBUG()` macro expands to `((void)0)`.
- **Runtime global:** Quick level changes during debugging without
  recompiling. Set `mlog_set_level(log, MLOG_WARN)` via a debug shell.
- **Per-backend:** The terminal might want DEBUG, but the flash log only
  needs ERROR. Independent filters per output.

**Filtering order:**
1. Compile-time: is the macro even present? (cheapest)
2. Runtime global: is the level >= global_level? (one comparison)
3. Any backend interested? (scan, skip formatting if none care)
4. Per-backend: deliver only to backends that want this level.

---

## 4. Single-char level indicator

**Decision:** Log lines show `[I]`, `[W]`, `[E]` instead of `[INFO]`,
`[WARN]`, `[ERROR]`.

**Why:**

- UART bandwidth is precious on slow baud rates. 3 bytes vs 7 bytes.
- Visually consistent column alignment.
- Easy to grep: `grep "\[E\]"` for errors.
- The full name is available via `mlog_level_name()` when needed.

---

## 5. Static format buffer

**Decision:** Messages are formatted into a stack-allocated buffer of
`MLOG_BUF_SIZE` bytes (default 128). Messages longer than this are
truncated.

**Why:**

- No dynamic allocation. Stack usage is deterministic.
- 128 bytes covers the vast majority of log messages. A typical line:
  `"12.345 [I] MQTT: Connected to broker.local:1883\n"` = ~50 bytes.
- For the rare long message, truncation is better than heap allocation
  or stack overflow.
- The user can increase `MLOG_BUF_SIZE` if needed.

---

## 6. C library dependency

**Decision:** microlog uses C library facilities such as `vsnprintf`,
`snprintf`, `memcpy`, `memset`, and `strlen`.

**Why:**

- `vsnprintf` is universally available on platforms with a C99 libc.
- Writing a custom printf is ~500 lines of bug-prone code for minimal
  benefit.
- On bare-metal with newlib-nano, `vsnprintf` is ~2 KB of code. This is
  acceptable for a logging library.
- Platforms with no libc (very small MCUs) typically don't use printf-style
  logging anyway.

**Tradeoff accepted:** `vsnprintf` pulls in libc formatting code (~2 KB).
For the tiniest MCUs, the user may need a different logger design.

---

## 7. ANSI color as opt-in per backend

**Decision:** Color is enabled per-backend, not globally. Disabled at
compile time with `MLOG_ENABLE_COLOR=0`.

**Why:**

- A terminal emulator benefits from color. A file log or ring buffer
  does not.
- The same logger can have one colored backend (terminal) and one plain
  backend (flash) simultaneously.
- Stripping color at compile time removes the string literals and branch.

---

## 8. Hex dump utility

**Decision:** Include a `mlog_hexdump()` function that formats binary
data as hex strings.

**Why:**

- Protocol debugging (MQTT, Modbus, I2C) constantly requires hex dumps.
- Without a helper, every developer writes their own hex formatting loop
  with subtle bugs (off-by-one, buffer overflow, missing NUL).
- The function respects the same level filtering and backends as regular
  log messages.

---

## Summary of tradeoffs

| Decision | Gains | Costs |
|----------|-------|-------|
| Global singleton | Zero-config convenience macros | One static global |
| Backend callbacks | Any output destination | User writes callbacks |
| Two-level filtering | Strip code + runtime flexibility | Two config points |
| Static buffer | Deterministic, no heap | Truncation at MLOG_BUF_SIZE |
| vsnprintf | Full printf support | ~2 KB libc dependency |
| Per-backend color | Mixed color/plain output | Extra bool per backend |
