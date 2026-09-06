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

## Roadmap & Features

- [x] Level 3 (MBO) historical tick data compatibility with LOB reconstruction and matching
- [x] Memory-mapped file processing & SPSC shared-memory ring buffer for market data
- [x] Realistic network latency simulation and performance profiling
- [ ] Transition from event queue to direct callbacks for latency reduction
- [ ] Advanced limit order execution models & slippage modeling
- [ ] Live trading gateway integration
