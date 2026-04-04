#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

// Hardware intrinsics must be included OUTSIDE of any custom namespace!
#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__x86_64__)
#include <x86intrin.h>
#endif

namespace backtesting_engine::performance {

// Cross-Platform Hardware Timestamp Counter macros
#if defined(__aarch64__)
// Apple Silicon / ARM64
inline uint64_t get_tsc() {
  uint64_t tsc;
  __asm__ volatile("mrs %0, cntvct_el0" : "=r"(tsc));
  return tsc;
}
#elif defined(_MSC_VER)
// Windows MSVC
inline uint64_t get_tsc() { return __rdtsc(); }
#elif defined(__x86_64__)
// Linux / Mac Intel x86_64
inline uint64_t get_tsc() { return __rdtsc(); }
#else
// Fallback (Software Clock)
inline uint64_t get_tsc() {
  return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}
#endif

class TSC_Clock {
public:
  // Calibrate the TSC to nanoseconds conversion constant
  // Run this exactly once when the simulation application starts up.
  static void calibrate() {
    uint64_t start_tsc = get_tsc();
    auto start_time = std::chrono::high_resolution_clock::now();

    // Sleep for 100 milliseconds to observe how many CPU ticks pass
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto end_time = std::chrono::high_resolution_clock::now();
    uint64_t end_tsc = get_tsc();

    uint64_t elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              end_time - start_time)
                              .count();
    uint64_t elapsed_tsc = end_tsc - start_tsc;

    tsc_hz_ = (static_cast<double>(elapsed_tsc) / elapsed_ns) * 1e9;
    ns_per_tsc_ = static_cast<double>(elapsed_ns) / elapsed_tsc;

    std::cout << "[TSC_Clock] Calibrated Hardware Clock. Estimated Frequency: "
              << (tsc_hz_ / 1e9) << " GHz" << std::endl;
  }

  // Zero-overhead inline conversion from CPU ticks to actual Nanoseconds
  static inline uint64_t tsc_to_nanoseconds(uint64_t tsc_delta) {
    return static_cast<uint64_t>(tsc_delta * ns_per_tsc_);
  }

private:
  inline static double tsc_hz_ = 0.0;
  inline static double ns_per_tsc_ = 0.0;
};

} // namespace backtesting_engine::performance
