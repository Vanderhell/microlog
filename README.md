# microlog

[![CI](https://github.com/Vanderhell/microlog/actions/workflows/ci.yml/badge.svg)](https://github.com/Vanderhell/microlog/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C99](https://img.shields.io/badge/C-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![Embedded](https://img.shields.io/badge/Target-Embedded%20Systems-0a7ea4.svg)](#why-microlog)

Structured logging for embedded systems in portable C99.

No third-party runtime dependencies. Zero dynamic allocation. Pluggable backends. Predictable output.

## Why microlog?

Embedded projects usually start with scattered `printf` calls and quickly grow into an unstructured mix of debug noise, warnings, and production diagnostics. `microlog` provides a compact logging layer with levels, tags, timestamps, and multi-backend routing while staying simple enough for firmware and low-footprint systems.

```c
MLOG_INFO("MQTT", "Connected to %s:%d", host, port);
MLOG_WARN("SENS", "Temperature out of range: %.1f", temp);
MLOG_ERROR("CONF", "CRC failed, using defaults");
```

Example output:

```text
12.345 [I] MQTT: Connected to broker.local:1883
12.350 [W] SENS: Temperature out of range: 85.3
12.351 [E] CONF: CRC failed, using defaults
```

## Features

- Five log levels: `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`
- Compile-time filtering via `MLOG_LEVEL_MIN`
- Runtime global and per-backend level filtering
- Up to `MLOG_MAX_BACKENDS` simultaneous outputs
- Optional millisecond timestamps through a clock callback
- Optional ANSI colors per backend
- `printf`-style formatting through `vsnprintf`
- Hex dump helper for protocol and transport debugging
- Global singleton and explicit logger instances
- No heap allocations and no third-party runtime dependencies

## Repository Layout

```text
include/mlog.h        Public API
src/mlog.c            Implementation
tests/test_all.c      Test suite
tests/Makefile        Local test runner
docs/API_REFERENCE.md Complete API reference
docs/DESIGN.md        Design rationale
docs/PORTING_GUIDE.md Integration patterns
```

## Quick Start

### 1. Copy the library

Add these files to your project:

- `include/mlog.h`
- `src/mlog.c`

### 2. Register a backend

```c
#include "mlog.h"

static void uart_write(const char *buf, uint16_t len,
                       mlog_level_t level, void *ctx)
{
    (void)level;
    (void)ctx;
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, 100);
}

void logging_init(void)
{
    mlog_t *log = mlog_global();
    mlog_set_clock(log, HAL_GetTick);

    mlog_backend_t uart = {
        .write = uart_write,
        .ctx = NULL,
        .level = MLOG_DEBUG,
        .color = false,
    };

    mlog_add_backend(log, &uart);
}
```

### 3. Log messages

```c
MLOG_INFO("BOOT", "Firmware v%d.%d.%d", MAJOR, MINOR, PATCH);
MLOG_DEBUG("WIFI", "Scanning for networks...");
MLOG_WARN("BATT", "Low battery: %d%%", percent);
MLOG_ERROR("MQTT", "Publish failed: %s", err_str);

uint8_t packet[] = { 0x10, 0x0E, 0x00, 0x04, 'M', 'Q', 'T', 'T' };
mlog_hexdump(NULL, MLOG_DEBUG, "PKT", packet, sizeof(packet));
```

## Build And Test

### GCC or Clang

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic -Iinclude src/mlog.c tests/test_all.c -o test_all
./test_all
```

### Make

```bash
cd tests
make
```

GitHub Actions runs this test flow automatically through `.github/workflows/ci.yml`.

## Configuration

| Macro | Default | Description |
|---|---:|---|
| `MLOG_MAX_BACKENDS` | `3` | Maximum number of registered backends |
| `MLOG_BUF_SIZE` | `128` | Internal formatting buffer size |
| `MLOG_LEVEL_MIN` | `0` | Compile-time minimum level, `0=TRACE` to `5=NONE` |
| `MLOG_ENABLE_COLOR` | `1` | Include ANSI color support |
| `MLOG_ENABLE_TIMESTAMP` | `1` | Include timestamp support |

Production build example:

```bash
gcc -DMLOG_LEVEL_MIN=2 -DMLOG_ENABLE_COLOR=0 -std=c99 -Iinclude src/mlog.c app.c
```

## API Overview

| Symbol | Purpose |
|---|---|
| `mlog_init` | Initialize an explicit logger instance |
| `mlog_global` | Access the global singleton |
| `mlog_set_clock` | Attach a timestamp source |
| `mlog_set_level` | Set runtime global filter |
| `mlog_add_backend` | Register an output backend |
| `mlog_clear_backends` | Remove all registered backends |
| `mlog_backend_set_level` | Change one backend filter |
| `mlog_log` / `mlog_vlog` | Core logging entry points |
| `mlog_hexdump` | Dump binary payloads as hex |
| `MLOG_TRACE` ... `MLOG_ERROR` | Convenience macros for the global logger |

Full API reference: [docs/API_REFERENCE.md](docs/API_REFERENCE.md)

## Documentation

- [API Reference](docs/API_REFERENCE.md)
- [Design Rationale](docs/DESIGN.md)
- [Porting Guide](docs/PORTING_GUIDE.md)
- [Contributing Guide](CONTRIBUTING.md)
- [Changelog](CHANGELOG.md)

## Integration Notes

`microlog` is intended for small systems and companion firmware libraries where deterministic behavior matters. Typical integrations include:

- UART or RTT console output during development
- Ring buffers for in-memory diagnostics while power remains applied
- Transport/protocol tracing during bring-up
- Shared logging across small embedded support libraries

## License

MIT License. Copyright (c) 2026 Vanderhell.
See [LICENSE](LICENSE).
