#include "LatencyBenchmark.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <climits>

namespace mm {

double LatencyStats::percentile(double p) const {
    if (samples.empty()) return 0.0;
    
    std::vector<long long> sorted_samples = samples;
    std::sort(sorted_samples.begin(), sorted_samples.end());
    
    size_t index = static_cast<size_t>(p * sorted_samples.size());
    if (index >= sorted_samples.size()) index = sorted_samples.size() - 1;
    
    return sorted_samples[index] / 1000.0;  // Return in microseconds
}

std::string LatencyStats::to_string() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << operation_name << ":\n";
    oss << "  Count:   " << count << "\n";
    oss << "  Avg:     " << avg_us() << " μs\n";
    oss << "  Min:     " << min_us() << " μs\n";
    oss << "  Max:     " << max_us() << " μs\n";
    
    if (!samples.empty()) {
        oss << "  P50:     " << percentile(0.50) << " μs\n";
        oss << "  P95:     " << percentile(0.95) << " μs\n";
        oss << "  P99:     " << percentile(0.99) << " μs\n";
    }
    
    return oss.str();
}

void LatencyBenchmark::record(const std::string& operation, long long latency_ns) {
    if (!enabled_) return;
    
    // Create stat entry if it doesn't exist
    if (stats_.find(operation) == stats_.end()) {
        stats_.emplace(operation, LatencyStats(operation));
    }
    
    stats_[operation].add_sample(latency_ns);
}

LatencyStats* LatencyBenchmark::get_stats(const std::string& operation) {
    auto it = stats_.find(operation);
    if (it != stats_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::string LatencyBenchmark::report() const {
    std::ostringstream oss;
    oss << "\n=== Latency Benchmark Report ===\n\n";
    
    for (const auto& pair : stats_) {
        oss << pair.second.to_string() << "\n";
    }
    
    return oss.str();
}

void LatencyBenchmark::reset() {
    stats_.clear();
}

} // namespace mm

