# Verification

Document revision commit: `d589538bc8741437db063859c2d4c559305c7722`

This file records repository verification status conservatively. Documentation updates alone are not treated as runtime proof.

## Verified

- Documentation set exists for API, cookbook, design, porting, troubleshooting, contributing, security, changelog, and release policy.
- `.github/workflows/release.yml` is present and configured for tag-only trigger pattern `v*`.
- Local `git tag --list --sort=version:refname` output was empty at the time of the last closeout, so the repository currently has no local tag evidence for a published release in this checkout.

## Not Verified

- Runtime test execution from `tests/test_all.c`
- Compile fixtures under `tests/compile/`
- CMake configure, build, install, and `ctest` flow
- External `find_package(microlog CONFIG REQUIRED)` consumers
- GitHub Actions matrix execution results
- GCC and Clang sanitizer execution
- `clang-tidy`, `cppcheck`, and `-fanalyzer` results
- MSVC build and test execution
- Embedded cross-compile job execution
- WSL audit requested by the prompt pack
