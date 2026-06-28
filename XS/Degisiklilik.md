## Compilation Strategy

- **HIR → Baseline JIT**
  - Provides fast startup with minimal compilation latency.
  - Applies only basic optimizations to execute code as quickly as possible.

- **MIR → Performance JIT**
  - Uses runtime profiling to identify hot code paths.
  - Applies more aggressive optimizations for improved execution performance.

- **XLIL → AOT (Ahead-of-Time)**
  - Performs full ahead-of-time compilation before execution.
  - Produces optimized native binaries suitable for distribution and deployment.
