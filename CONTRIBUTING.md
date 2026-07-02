# Contributing to microlog

microlog is a C99 runtime library with zero dynamic allocation and no third-party runtime dependencies.

## Ground Rules

- Do not weaken tests.
- Keep the runtime implementation C99.
- Do not add internal threads, async flushing, deferred logging, log rotation, or persistence guarantees.
- Do not add RTOS dependencies or third-party runtime dependencies.
- Preserve the documented callback ownership and truncation rules.
- Do not create tags or releases unless explicitly requested.

## Expected Workflow

1. Start with a focused issue or defect description.
2. Add or strengthen tests before broad implementation changes.
3. Keep public ABI changes deliberate and documented.
4. Ensure producer and consumer configuration remains compatible.
5. Update docs when behavior or constraints change.

## Local Checks

- `make -C tests`
- `make -C tests compile-checks`
- `cmake -S . -B build -DMICROLOG_BUILD_TESTS=ON`

By contributing, you agree to the MIT License.
