# microlog

[![CI](https://github.com/Vanderhell/microlog/actions/workflows/ci.yml/badge.svg)](https://github.com/Vanderhell/microlog/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C99](https://img.shields.io/badge/C-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)

Structured logging for embedded systems in portable C99.

No third-party runtime dependencies. Zero dynamic allocation. Pluggable backends. Predictable output.

## Features

- Five runtime levels: `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`
- Compile-time stripping via `MLOG_LEVEL_MIN`
- Runtime global and per-backend filters
- Multiple backends with per-backend color selection
- Optional timestamps via one clock callback sample per event
- Deterministic truncation with newline termination
- C99 logging macros that compile without GNU variadic extensions
- Hex dump helper for byte-oriented diagnostics

## Quick Start

```c
#include "mlog.h"

static void uart_write(const char *buf, uint16_t len, mlog_level_t level, void *ctx)
{
    (void)level;
    (void)ctx;
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, 100);
}

void logging_init(void)
{
    mlog_backend_t backend = {
        .write = uart_write,
        .ctx = NULL,
        .level = MLOG_INFO,
        .color = false,
    };

    mlog_t *log = mlog_global();
    mlog_set_clock(log, HAL_GetTick);
    mlog_add_backend(log, &backend);
}

void app_run(void)
{
    MLOG_INFO("BOOT", "ready");
    MLOG_WARN("BATT", "Low battery: %u%%", 17u);
}
```

## Behavior Notes

- Every emitted record ends with `\n`.
- If ANSI color output is enabled for a backend, microlog reserves room for the reset sequence before the newline.
- Long tags and messages are truncated deterministically to preserve a terminating NUL and the final line ending.
- `MLOG_NONE` is a filter-only level. It must not be used as an emitted message level.
- Backend callbacks are synchronous. The callback buffer is borrowed stack storage and must be copied before deferred use.
- Backend write failures are not propagated through the API.
- The library is not internally synchronized. Concurrent access and recursive logging require external discipline.

## Build

Manual build:

```sh
cc -std=c99 -Wall -Wextra -Wpedantic -Iinclude src/mlog.c tests/test_all.c -o test_all
./test_all
```

Local make-based test flow:

```sh
make -C tests
make -C tests compile-checks
```

CMake package flow:

```sh
cmake -S . -B build -DMICROLOG_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix install
```

## Documentation

- [API Reference](docs/API_REFERENCE.md)
- [Cookbook](docs/COOKBOOK.md)
- [Design](docs/DESIGN.md)
- [Porting Guide](docs/PORTING_GUIDE.md)
- [Troubleshooting](docs/TROUBLESHOOTING.md)
- [Verification](docs/VERIFICATION.md)
- [Contributing](CONTRIBUTING.md)
- [Security](SECURITY.md)
- [Changelog](CHANGELOG.md)
- [Release Workflow](.github/workflows/release.yml)

Releases are tag-based. Branch pushes do not create releases; only pushed `v*` tags trigger [`.github/workflows/release.yml`](.github/workflows/release.yml).

## License

MIT License. See [LICENSE](LICENSE).
