# async-promise

*A Modern C++20 Promise Library for Asynchronous Programming*

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)]()

## 📖 Overview

**async-promise** is a high-performance, header-only C++20 library that brings JavaScript-style promises to modern C++ applications. Built from the ground up with modern C++20 features, this library provides a clean, type-safe, and efficient way to handle asynchronous operations.

Unlike traditional callback-based approaches, async-promise offers:
- **Promise/A+ compliant API** - Familiar JavaScript-style promise chaining
- **Type-safe operations** - Compile-time type checking for resolve/reject values
- **Exception handling** - Automatic conversion of C++ exceptions to promise rejections
- **Thread-safe by default** - Robust multi-threading with modern STL concurrency primitives
- **Zero external dependencies** - Only uses the C++ standard library

## ✨ Key Features

- 🚀 **High Performance**: Optimized promise resolution with minimal memory allocations
- 🔒 **Thread-Safe**: Uses `std::recursive_mutex` and `std::condition_variable_any` for robust concurrency
- 🎯 **Type-Safe**: Leverages C++20 features for compile-time type checking
- 🧵 **Modern Concurrency**: Built with STL threading primitives, no custom synchronization code
- 📦 **Header-Only**: Easy integration with `#define PROMISE_HEADONLY`
- 🔧 **Framework Integration**: Seamless support for Qt, MFC, Boost.Asio, and other async frameworks
- ⚡ **Low Overhead**: Efficient type-erasure and optimized promise chaining
- 🧪 **Well Tested**: Comprehensive test suite with working examples

## 🚀 Quick Start

### Basic Usage

```cpp
#include "async-promise/promise.hpp"
#include <iostream>

using namespace promise;

int main() {
    // Create a promise
    auto myPromise = newPromise([](Defer& d) {
        // Simulate async work
        std::cout << "Starting async operation..." << std::endl;
        d.resolve(42, "Success!");
    });

    // Chain promises
    myPromise
        .then([](int value, const std::string& message) {
            std::cout << "Result: " << value << " - " << message << std::endl;
            return resolve(value * 2);
        })
        .then([](int doubled) {
            std::cout << "Doubled: " << doubled << std::endl;
        })
        .fail([](const std::string& error) {
            std::cerr << "Error: " << error << std::endl;
        });

    return 0;
}
```

### Advanced Features

```cpp
#include "async-promise/promise.hpp"
#include <vector>
#include <thread>
#include <chrono>

using namespace promise;

// Wait for multiple promises
Promise waitForAll(std::vector<Promise> promises) {
    return all(promises).then([]() {
        std::cout << "All promises completed!" << std::endl;
    });
}

// Race promises (first to complete wins)
Promise racePromises(std::vector<Promise> promises) {
    return race(promises).then([](const any& result) {
        std::cout << "First promise completed!" << std::endl;
    });
}

// Promise-based loops
Promise countdown(int from) {
    return doWhile([from](DeferLoop& loop) mutable {
        if (from > 0) {
            std::cout << "Countdown: " << from << std::endl;
            from--;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            loop.doContinue();
        } else {
            loop.doBreak("Countdown finished!");
        }
    });
}
```

## 📦 Installation

### Header-Only Integration

For the simplest integration, define `PROMISE_HEADONLY` before including the header:

```cpp
#define PROMISE_HEADONLY
#include "async-promise/promise.hpp"
```

### Manual Compilation

Compile with your application:

```bash
g++ -std=c++20 -I/path/to/async-promise/include your_app.cpp src/promise.cpp -pthread
```

### CMake Integration

Add to your CMakeLists.txt:

```cmake
# Add async-promise
add_subdirectory(path/to/async-promise)

# Link to your target
target_link_libraries(your_target async-promise)
```

## 📚 API Reference

### Core Classes

- **`Promise`**: The main promise class with chaining methods
- **`Defer`**: Used in promise executors to resolve/reject promises
- **`DeferLoop`**: Used in `doWhile` loops for iteration control

### Global Functions

| Function | Description |
|----------|-------------|
| `newPromise(func)` | Create a new promise with executor function |
| `resolve(args...)` | Create an immediately resolved promise |
| `reject(args...)` | Create an immediately rejected promise |
| `all(promises)` | Wait for all promises to resolve |
| `race(promises)` | Wait for first promise to resolve/reject |
| `doWhile(func)` | Create a promise-based loop |

### Promise Methods

| Method | Description |
|--------|-------------|
| `.then(onResolve, onReject?)` | Chain success/failure handlers |
| `.fail(onReject)` | Handle rejection only |
| `.always(onAlways)` | Execute regardless of resolve/reject |
| `.finally(onFinally)` | Execute after promise settles |
| `.resolve(args...)` | Manually resolve the promise |
| `.reject(args...)` | Manually reject the promise |
| `.clear()` | Reset the promise state |

### Thread Safety

The library is thread-safe by default, using:
- `std::recursive_mutex` for critical sections
- `std::condition_variable_any` for synchronization
- Proper lock ordering to prevent deadlocks

## 🔧 Building from Source

### Prerequisites

- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- CMake 3.12+ (for build system)
- POSIX threads (usually included with compiler)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/your-repo/async-promise.git
cd async-promise

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
make -j$(nproc)

# Run tests
./test0
./multithread_test
./simple_timer
```

### Build Options

- `PROMISE_HEADONLY`: Define to use header-only mode
- `PROMISE_MULTITHREAD`: Define to enable multi-threading (default: enabled)

## 🧪 Examples

The `example/` directory contains working examples:

- **`test0.cpp`**: Basic promise operations and chaining
- **`multithread_test.cpp`**: Multi-threaded promise usage
- **`simple_timer.cpp`**: Timer functionality with task scheduler
- **`chain_defer_test.cpp`**: Advanced promise chaining patterns
- **`simple_benchmark_test.cpp`**: Performance benchmarking

### Running Examples

```bash
# Compile and run basic test
g++ -std=c++20 -Iinclude example/test0.cpp src/promise.cpp -pthread -o test0
./test0

# Compile with extensions
g++ -std=c++20 -Iinclude -Iextensions example/simple_timer.cpp src/promise.cpp -pthread -o timer
./timer
```

## 🏗️ Architecture

### Core Components

```
async-promise/
├── include/async-promise/
│   ├── promise.hpp              # Main API headers
│   ├── promise_implementation.hpp # Implementation details
│   ├── any.hpp                  # Type-erasure utilities
│   ├── any_type.hpp            # Type system helpers
│   ├── call_traits.hpp         # Function trait utilities
│   ├── extensions.hpp          # Extension system
│   └── add_ons.hpp            # Compatibility helpers
├── src/
│   └── promise.cpp             # Library implementation
└── extensions/
    ├── qt/                     # Qt framework integration
    ├── task_scheduler/         # Timer and task scheduling
    └── windows/                # Windows-specific features
```

### Design Principles

1. **Modern C++**: Leverages C++20 features for optimal performance
2. **Type Safety**: Compile-time type checking wherever possible
3. **Exception Safety**: Proper exception handling and propagation
4. **Thread Safety**: Lock-free where possible, properly synchronized elsewhere
5. **Performance**: Minimal allocations, optimized promise resolution paths

## 🤝 Contributing

We welcome contributions! Here's how to get involved:

### Development Setup

```bash
# Fork and clone
git clone https://github.com/your-fork/async-promise.git
cd async-promise

# Build and test
mkdir build && cd build
cmake ..
make
make test
```

### Guidelines

- Follow modern C++20 idioms and best practices
- Add tests for new functionality
- Update documentation for API changes
- Maintain thread safety and performance
- Use descriptive commit messages

### Reporting Issues

- Use GitHub issues for bug reports and feature requests
- Include minimal reproducible examples
- Specify compiler version and platform
- Check existing issues before creating new ones

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Inspired by JavaScript Promise/A+ specification
- Built with modern C++20 features and STL
- Community contributions and feedback

---

**async-promise** - Bringing modern asynchronous programming to C++ with style and performance! 🚀