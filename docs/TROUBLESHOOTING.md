# Troubleshooting

## Long tags truncate output

microlog truncates deterministically. The final newline is preserved, and color-enabled records also preserve the ANSI reset sequence.

## Missing newline or missing ANSI reset

This indicates either an old build or a mismatched consumer configuration. Rebuild producer and consumers with the same header configuration.

## GNU variadic macro compile errors

Use the current header. The public logging macros forward standard C99 variadic arguments and support calls such as `MLOG_INFO("TAG", "message")`.

## Disabled macro argument side effects

Compile-time-disabled logging macros evaluate none of their arguments. If side effects still occur, a stale header is being used.

## `MLOG_NONE` as filter only

`MLOG_NONE` is valid as a filter level and invalid as an emitted message level.

## Invalid levels

Invalid message levels are ignored. Invalid backend or configuration levels are rejected or ignored deterministically.

## Backend callback cannot report failure

microlog does not propagate backend write failures. If delivery status matters, the backend must capture it in caller-owned state.

## Callback retained stack pointer

The callback buffer is borrowed stack storage. Copy it before any deferred, DMA, queue, or background use.

## Recursive logging unsupported

Backend callbacks must not recursively log through the same logger unless the application implements a deliberate external policy.

## Concurrent use without a lock

The same logger instance is not internally synchronized. Add external locking around configuration mutation and dispatch.

## Color-disabled layout mismatch

The `color` field remains in `mlog_backend_t` even when color output is compiled out. Producer and consumer must still use identical ABI-affecting configuration values.

## CMake consumer config mismatch

Do not override exported ABI-affecting definitions inconsistently across the package and consumers.

## External consumer cannot find package

Install microlog first and set `CMAKE_PREFIX_PATH` or `microlog_DIR` to the install prefix containing `lib/cmake/microlog`.

## MSVC format attributes

Format checking attributes are enabled only for GCC and Clang. MSVC should compile without those attributes.

## Sanitizer or static-analysis jobs unavailable

Treat missing tool coverage as not verified rather than proof of portability.

## Hex dump order confusion

`mlog_hexdump()` emits bytes in input order, not reversed order.
