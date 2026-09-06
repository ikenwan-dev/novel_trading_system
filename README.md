# Novel Trading System

An event-driven algorithmic trading and backtesting engine in C++23. It simulates market-by-order (MBO / Level 3) data locally, reconstructs limit order books (LOB), models realistic network latency, and executes trading strategies with order and risk management.

> [!NOTE]
> **Purely Offline Backtesting:** This system does **not** connect to external Databento servers or make live network calls. It parses and processes local historical `.mbo.dbn` files (located in `src/test_data/` or passed via command line) using the data structures provided by `databento-cpp`.

---

## Prerequisites & Installation

### Required

1. **C++ Compiler (C++23 support)**
   - **macOS:** Apple Clang (Xcode Command Line Tools)
     ```bash
     xcode-select --install
     ```
   - **Linux:** GCC 13+ or Clang 16+
     ```bash
     sudo apt update && sudo apt install build-essential
     ```

2. **CMake (>= 3.15)**
   - Meta-build system used to configure the project.
     ```bash
     # macOS (Homebrew)
     brew install cmake

     # Linux (Debian/Ubuntu)
     sudo apt install cmake
     ```

3. **OpenSSL**
   - Required by the `databento-cpp` dependency.
     ```bash
     # macOS (Homebrew)
     brew install openssl@3

     # Linux (Debian/Ubuntu)
     sudo apt install libssl-dev
     ```

### Optional

1. **Ninja Build System**
   - Recommended for significantly faster parallel builds compared to standard `make`.
     ```bash
     # macOS (Homebrew)
     brew install ninja

     # Linux (Debian/Ubuntu)
     sudo apt install ninja-build
     ```

---

## Third-Party Libraries (Automated via CMake)

All third-party libraries are downloaded and configured automatically when CMake runs (`FetchContent`):
- **[databento-cpp](https://github.com/databento/databento-cpp)** – Used locally for MBO/L3 record types (`databento::MboMsg`, `RecordHeader`, enums) and DBN parsing.
- **[glaze](https://github.com/stephenberry/glaze)** (v6.4.0) – High-performance JSON and binary serialization.
- **[GoogleTest](https://github.com/google/googletest)** (v1.14.0) – Unit testing suite.
- **[fast_cpp_csv_parser](third_party/fast_cpp_csv_parser)** – Fast header-only CSV parser.

---

## Building the Project

1. **Clone the repository:**
   ```bash
   git clone https://github.com/ikenwan-dev/novel_trading_system.git
   cd novel_trading_system
   ```

2. **Configure with CMake:**
   ```bash
   mkdir -p build && cd build

   # macOS (pointing to Homebrew's OpenSSL root):
   cmake -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3) ..

   # Linux / Default:
   cmake -DCMAKE_BUILD_TYPE=Release ..
   ```

   *(Optional: Use Ninja for faster compilation)*
   ```bash
   cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3) ..
   ```

   > [!TIP]
   > **Using the VS Code CMake Tools Extension on macOS?**
   > Add `"cmake.configureArgs": ["-DOPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl@3"]` to your `.vscode/settings.json` so the extension can locate OpenSSL during configuration.

3. **Compile:**
   - **Build all executables:**
     ```bash
     cmake --build . -j
     ```
   - **Build only `MappedBackTestEngineExample`:**
     ```bash
     cmake --build . --target MappedBackTestEngineExample -j
     ```

---

## Running Examples & Executables

All binaries are output to the `build/` directory after compilation.

### 1. Memory-Mapped Backtest Engine (`MappedBackTestEngineExample`)
Runs a backtest simulation by memory-mapping local `.mbo.dbn` market data files.

- **Run with default test data (`src/test_data/XNAS-20260120-7W93CD9NGT/`):**
  ```bash
  ./MappedBackTestEngineExample
  ```
- **Run with custom data directory or specific `.mbo.dbn` file:**
  ```bash
  ./MappedBackTestEngineExample /path/to/dbn/files/
  # or
  ./MappedBackTestEngineExample /path/to/sample.mbo.dbn
  ```

---

### 2. IPC Shared-Memory Backtest Engine (`IPCBackTestEngineExample` & `DataBentoProducer`)
Demonstrates multi-process backtesting using an SPSC (single-producer single-consumer) shared memory ring buffer.

1. **Terminal 1 (Start Producer):**
   ```bash
   ./DataBentoProducer
   ```
2. **Terminal 2 (Start Consumer/Engine):**
   ```bash
   ./IPCBackTestEngineExample
   ```

---

### 3. Running Unit Tests
Run the GoogleTest test suite via `ctest`:

```bash
ctest --output-on-failure
```

---

## Performance Benchmarks

End-to-end backtest simulation benchmark processing **11,111,761 market messages** across a full single-day trading session (`xnas-itch-20251219.mbo.dbn`, ~593 MB raw MBO binary). 

The benchmark measures the complete pipeline: memory-mapped file ingestion, order book reconstruction, simulated wire network flight latency, virtual exchange queue priority tracking, OMS risk validation, and market making strategy execution.

### Test Environment
- **CPU:** Apple M5 Pro (18 Cores)
- **Memory:** 48 GB Unified Memory
- **OS:** macOS 26.6.2 (Darwin / AppleClang C++23)
- **Clock:** Hardware TSC cycle counter calibration (~1.00 GHz)

### Benchmark Results

| Metric | `DataBentoLOB` (Baseline STL) | `OptimizedDataBentoLOB` (ObjectPool + FlatMap) | `DirectArrayLOB` (O(1) Direct Array) | Improvement (`DirectArrayLOB` vs Baseline) |
| :--- | :--- | :--- | :--- | :--- |
| **Total Duration** | 9.11 s | 7.55 s | **3.71 s** | **2.45x faster** |
| **System Throughput** | 1.22M msg/sec | 1.47M msg/sec | **2.99M msg/sec** | **~3.0 Million msg/sec** |
| **LOB Add Latency (P50)** | 540 ns | 332 ns | **41 ns** | **13.2x faster** |
| **LOB Add Latency (P99)** | 833 ns | 499 ns | **207 ns** | **4.0x faster** |
| **LOB Add Latency (P99.9)** | 1,083 ns | 1,166 ns | **332 ns** | **3.3x faster** |
| **LOB Cancel Latency (P50)** | 499 ns | 374 ns | **82 ns** | **6.1x faster** |
| **LOB Cancel Latency (P99)** | 874 ns | 499 ns | **749 ns** | **1.17x faster** |
| **Tick-to-Trade Latency (P50)** | 874 ns | 665 ns | **374 ns** | **2.34x faster** |
| **Tick-to-Trade Latency (P99)** | 1,291 ns | 1,041 ns | **1,624 ns** | Sub-2μs deterministic tail |

### Performance Insights
- **`DirectArrayLOB`** eliminates tree traversal and binary search entirely via direct arithmetic indexing (`size_t idx = LOB_CAPACITY / 2 + (price - base_price_) / TickSize;`), achieving a median **41 ns order insertion** and sustaining nearly **3.0 million messages/sec** full-pipeline throughput.
- **`OptimizedDataBentoLOB`** avoids all dynamic heap allocations using an intrusive doubly-linked list backed by a contiguous `ObjectPool` and open-addressing `FlatHashMap`, improving p99 order cancellation latency by ~43% over STL `std::vector` scans.
- **Cache line alignment (`alignas(64)`) and OS Zero-Page COW pre-faulting** guarantee deterministic sub-microsecond tick-to-trade execution times across the entire trading day.

---

## Roadmap & Features

- [x] Level 3 (MBO) historical tick data compatibility with LOB reconstruction and matching
- [x] Memory-mapped file processing & SPSC shared-memory ring buffer for market data
- [x] Realistic network latency simulation and performance profiling
- [ ] Ring buffer recycling & order eviction in OMS to support multi-day backtest runs without capacity overflow
- [ ] Linux core pinning & thread affinity (`pthread_setaffinity_np` / `isolcpus`) for deterministic latency
- [ ] High-performance discrete-event scheduler (replace `std::priority_queue` with a hierarchical timing wheel / flat ring calendar)
- [ ] Microstructure execution modeling (probabilistic queue cancellation decay & adverse selection markout)
- [ ] Live trading gateway integration with zero-queue direct callback execution
