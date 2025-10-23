# Flux Performance Benchmarks

This directory contains performance benchmarks and testing scripts for the Flux Archive Manager.

## 🎯 Benchmark Categories

### 1. **Compression Benchmarks**
- **File Size Impact**: How compression ratio varies with file types
- **Speed vs Ratio**: Trade-offs between compression speed and ratio
- **Memory Usage**: RAM consumption during compression
- **Multi-threading**: Performance scaling with thread count

### 2. **Extraction Benchmarks**
- **Extraction Speed**: Files per second, MB/s throughput
- **Memory Efficiency**: RAM usage during extraction
- **I/O Performance**: Disk write performance
- **Large Archive Handling**: Performance with archives >1GB

### 3. **GUI Performance**
- **UI Responsiveness**: Interface lag during operations
- **Progress Updates**: Frequency and accuracy of progress reporting
- **File Browser**: Performance with large directory structures
- **Resource Usage**: CPU and memory usage of GUI components

### 4. **CLI Performance**
- **Startup Time**: Time from command execution to operation start
- **Batch Operations**: Performance with multiple archives
- **Scripting Integration**: Performance in automated workflows

## 🔧 Benchmark Tools

### Planned Implementation
```bash
# Example benchmark commands (to be implemented)
./flux-benchmark --test compression --format zip --size 100MB
./flux-benchmark --test extraction --threads 4 --archive large.7z
./flux-benchmark --test gui-responsiveness --files 10000
```

### Test Data Sets
- **Small Files**: 1000 files, 1KB-10KB each
- **Medium Files**: 100 files, 1MB-10MB each  
- **Large Files**: 10 files, 100MB-1GB each
- **Mixed Archive**: Combination of various file sizes
- **Text Files**: Source code, documents (high compression ratio)
- **Binary Files**: Images, executables (low compression ratio)

## 📊 Benchmark Results

> **Note**: Results will be added as benchmarks are implemented and run.

### Target Performance Goals
- **Compression Speed**: >50 MB/s for ZIP format
- **Extraction Speed**: >100 MB/s for most formats
- **Memory Usage**: <100MB for archives up to 1GB
- **GUI Responsiveness**: <100ms UI update intervals
- **Startup Time**: <500ms for CLI operations

### Platform Comparison
Results will be collected for:
- **Windows 11** (MSVC 2022)
- **Ubuntu 22.04** (GCC 13)
- **macOS 14** (Clang 16)

## 🚀 Running Benchmarks

### Prerequisites
```bash
# Install benchmark dependencies
sudo apt-get install time valgrind  # Linux
brew install gnu-time               # macOS
```

### Basic Usage
```bash
# Build with benchmarks enabled
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_BENCHMARKS=ON
cmake --build build --parallel

# Run all benchmarks
cd build
ctest -L benchmark

# Run specific benchmark category
./benchmarks/compression_benchmark
./benchmarks/extraction_benchmark
```

### Custom Test Data
```bash
# Generate test archives
./scripts/generate_test_data.sh

# Run benchmarks with custom data
./flux-benchmark --data-dir ./test_data --output results.json
```

## 📈 Performance Tracking

### Continuous Integration
- Benchmarks run automatically on release tags
- Performance regression detection
- Historical performance data collection

### Reporting Format
```json
{
  "timestamp": "2024-01-15T10:30:00Z",
  "platform": "ubuntu-22.04",
  "compiler": "gcc-13.2",
  "benchmarks": {
    "compression": {
      "zip": {"speed_mbps": 65.2, "ratio": 0.42},
      "7z": {"speed_mbps": 23.1, "ratio": 0.38}
    },
    "extraction": {
      "zip": {"speed_mbps": 145.7},
      "7z": {"speed_mbps": 89.3}
    }
  }
}
```

## 🔍 Profiling and Analysis

### Memory Profiling
```bash
# Valgrind memory analysis
valgrind --tool=memcheck --leak-check=full ./flux-cli extract large.zip

# Heap profiling
valgrind --tool=massif ./flux-gui
```

### CPU Profiling
```bash
# Linux perf profiling
perf record -g ./flux-cli compress *.txt
perf report

# macOS Instruments
instruments -t "Time Profiler" ./flux-gui
```

## 📝 Adding New Benchmarks

### Benchmark Template
```cpp
#include <benchmark/benchmark.h>
#include "flux-core/archive.h"

static void BM_CompressionSpeed(benchmark::State& state) {
    const size_t file_size = state.range(0);
    std::vector<char> data(file_size, 'A');
    
    for (auto _ : state) {
        // Benchmark code here
        auto archive = ArchiveManager::create("test.zip");
        archive.addData("test.txt", data);
        benchmark::DoNotOptimize(archive);
    }
    
    state.SetBytesProcessed(state.iterations() * file_size);
}

BENCHMARK(BM_CompressionSpeed)
    ->Range(1024, 1024*1024*100)  // 1KB to 100MB
    ->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
```

### Integration Steps
1. Add benchmark source to `benchmarks/` directory
2. Update `CMakeLists.txt` to include new benchmark
3. Add test data generation if needed
4. Document expected performance characteristics
5. Include in CI pipeline if appropriate
