# CFA regression test fixtures

The initial repository contained only `Assignment-2-CFA/Test-Cases`.
This directory provides the requested submission location, `Assignment-2-IR/Test-Cases`.
`branch.c`, `buffer.c`, `loop.c`, `multi-call.c`, and `simple-call.c` copy those five
original fixtures with whitespace normalized. The other three files are additional regression tests,
not a separately supplied official IR test suite.

| Fixture | Expected paths | Purpose |
| --- | ---: | --- |
| simple-call | 1 | One internal function call |
| multi-call | 1 | Two calls to the same function must return to their own callers |
| loop | 1 | Terminate traversal when a node repeats in the same call context |
| branch | 2 | Preserve both branches, including calls to a shared function |
| buffer | 4 | Branch and loop handling across two calls to the same function |
| nested-call | 1 | Nested calls and repeated callers require a stack |
| shared-callee-branch | 2 | Merge branches, then call the shared function again |
| external-call | 1 | Preserve external-call intra edges without pushing a call |

These are structural ICFG paths, without constraint solving. A path never repeats
the same `(node, call stack)` state. Thus loops are not unrolled; a body that can
only reach the exit by revisiting a loop header is not included. The buffer test
has two structural branch choices per invocation, hence four paths, even though
its concrete argument values would select particular branches.

## Reproduce

Requires Linux, Python 3, CMake >= 3.23, a C++17 compiler, SVF built for LLVM 16,
and Z3. From the repository root, set `SVF_DIR`, `LLVM_DIR`, and `Z3_DIR` to your
installation directories, then run:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCFGA_BUILD_TESTS=ON
cmake --build build -j4
python3 Assignment-2-CFA/tests/run_tests.py \
  --clang "$LLVM_DIR/bin/clang" \
  --cfga "$PWD/Assignment-2-CFA/cfga" \
  --checker "$PWD/build/Assignment-2-CFA/cfga_regression" \
  --output-dir "$PWD/build/test-results"
```

The runner compiles each fixture to LLVM bitcode and executes both the submitted
analyzer and a checker that compiles the same `CFGA.cpp`. It checks output path
counts and duplicates. The checker verifies every path's main entry/exit,
adjacent graph edges, call-site identity at each return, empty final call stack,
and stable results on repeated analysis. Each subprocess has a 30-second timeout.
Logs, bitcode, analyzer output, and `results.json` stay in the selected output directory.

## Verified result (2026-10-28)

All eight cases passed using Ubuntu 22.04 in WSL2, GCC 11.4.0, CMake 3.31.10,
SVF 3.2 (source revision `bbd2636`), LLVM/Clang 16.0.4, and Z3 4.8.8.0.
Both the analyzer and checker compiled successfully with the project's `-Werror` flags.
Recursive-call termination and arbitrary loop unrolling are outside this suite.
