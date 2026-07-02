# Security Policy

## Scope

microlog is a small synchronous logging library. Security-relevant reports are most useful when they include:

- Memory safety issues
- Undefined-behavior triggers
- Truncation or formatting bugs that corrupt output
- ABI mismatches that lead to incorrect memory access

## Reporting

Please report suspected vulnerabilities privately to the repository maintainer before opening a public issue when coordinated disclosure is appropriate.

## Expectations

- The library does not provide durable delivery, integrity protection, or tamper-resistant storage.
- Backend callbacks define the actual transport and persistence characteristics.
- Consumers remain responsible for thread safety, privileged I/O, and secret handling in log content.
