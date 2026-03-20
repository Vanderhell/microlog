# Changelog

## [1.0.0] — 2026-03-20

### Added

- 5 log levels: TRACE, DEBUG, INFO, WARN, ERROR.
- Module tag support for source identification.
- Multiple backends (up to 3) with independent level filters.
- Compile-time level stripping via `MLOG_LEVEL_MIN`.
- Runtime global and per-backend level filtering.
- Optional millisecond timestamps via clock callback.
- Optional ANSI color per backend.
- Printf-style formatting via `vsnprintf`.
- Hex dump utility for protocol debugging.
- Global singleton with lazy init + independent instances.
- Convenience macros: MLOG_ERROR, MLOG_WARN, MLOG_INFO, MLOG_DEBUG, MLOG_TRACE.
- Full documentation: API reference, design rationale, porting guide.
- Test suite: 33 tests covering all features.
- Platform recipes for STM32, ESP32, Segger RTT, Arduino, Linux.
