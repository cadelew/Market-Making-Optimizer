#pragma once
#include <chrono>
#include <map>
#include <string>
#include <vector>


namespace mm {

// Timer for measuring single operations
class Timer {
public:
  Timer() : start_time_(std::chrono::high_resolution_clock::now()) {}

  // Get elapsed time in nanoseconds
  long long elapsed_ns() const {
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end -
                                                                start_time_)
        .count();
  }

  // Get elapsed time in microseconds
  double elapsed_us() const { return elapsed_ns() / 1000.0; }

  // Get elapsed time in milliseconds
  double elapsed_ms() const { return elapsed_ns() / 1000000.0; }

  // Reset timer
  void reset() { start_time_ = std::chrono::high_resolution_clock::now(); }

private:
  std::chrono::high_resolution_clock::time_point start_time_;
};

// Statistics for a specific operation
struct LatencyStats {
  std::string operation_name;
  long long count = 0;
  long long total_ns = 0;
  long long min_ns = LLONG_MAX;
  long long max_ns = 0;
  std::vector<long long>
      samples; // Store recent samples for percentile calculation

  LatencyStats() = default; // Default constructor for std::map
  LatencyStats(const std::string &name) : operation_name(name) {}

  void add_sample(long long latency_ns) {
    count++;
    total_ns += latency_ns;

    if (latency_ns < min_ns)
      min_ns = latency_ns;
    if (latency_ns > max_ns)
      max_ns = latency_ns;

    // Keep last 1000 samples for percentile calculation
    samples.push_back(latency_ns);
    if (samples.size() > 1000) {
      samples.erase(samples.begin());
    }
  }

  double avg_ns() const {
    return count > 0 ? static_cast<double>(total_ns) / count : 0.0;
  }

  double avg_us() const { return avg_ns() / 1000.0; }
  double min_us() const { return min_ns / 1000.0; }
  double max_us() const { return max_ns / 1000.0; }

  // Calculate percentile (e.g., 0.99 for 99th percentile)
  double percentile(double p) const;

  std::string to_string() const;
};

// Global benchmark manager
class LatencyBenchmark {
public:
  static LatencyBenchmark &instance() {
    static LatencyBenchmark inst;
    return inst;
  }

  // Record a measurement
  void record(const std::string &operation, long long latency_ns);

  // Get stats for an operation
  LatencyStats *get_stats(const std::string &operation);

  // Print all statistics
  std::string report() const;

  // Reset all statistics
  void reset();

  // Enable/disable benchmarking
  void set_enabled(bool enabled) { enabled_ = enabled; }
  bool is_enabled() const { return enabled_; }

private:
  LatencyBenchmark() : enabled_(true) {}

  std::map<std::string, LatencyStats> stats_;
  bool enabled_;
};

// RAII timer that auto-records on destruction
class ScopedTimer {
public:
  ScopedTimer(const std::string &operation_name)
      : operation_(operation_name), timer_() {
    // Timer starts automatically in constructor
  }

  ~ScopedTimer() {
    // Auto-record when scope ends
    if (LatencyBenchmark::instance().is_enabled()) {
      LatencyBenchmark::instance().record(operation_, timer_.elapsed_ns());
    }
  }

private:
  std::string operation_;
  Timer timer_;
};

// Convenience macro for scoped timing
#define BENCHMARK_SCOPE(name) mm::ScopedTimer _benchmark_timer_##__LINE__(name)

} // namespace mm
