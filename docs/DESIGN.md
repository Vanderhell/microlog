# Design

## Goals

- Small embedded-friendly API
- No heap allocation
- Straightforward integration into C99 codebases
- Deterministic truncation instead of unbounded writes

## Core Choices

Global and explicit loggers:

- `mlog_global()` provides a zero-initialized singleton.
- Explicit `mlog_t` instances allow test and subsystem isolation.

Callback-based backends:

- Output is delegated to user code rather than `FILE *`.
- Backends are synchronous and caller-controlled.
- Delivery guarantees depend entirely on the backend implementation.

Two-stage filtering:

- `MLOG_LEVEL_MIN` strips code at compile time.
- `global_level` and backend `level` filter at runtime.

Deterministic truncation:

- Formatting uses bounded append helpers.
- Final `\n` is preserved.
- When a color prefix is emitted, the reset sequence is reserved before truncation finishes.

Timestamp consistency:

- The clock callback is sampled once per log event.
- Every interested backend receives the same timestamp text.
- `uint32_t` wrap behavior is callback-defined and exposed as-is.

## Non-Goals

- Persistent storage guarantees
- Async/background flushing
- Log rotation
- Internal thread synchronization
- Recovery-safe crash logging

## C Library Requirements

The implementation depends on C library facilities including:

- `vsnprintf`
- `snprintf`
- `memcpy`
- `memset`
- `strlen`

There are no third-party runtime dependencies.
