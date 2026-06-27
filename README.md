# undoCore

![C++20](https://img.shields.io/badge/C++20-blue.svg)
![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)
[![Docs](https://github.com/undoRT/undoCore/actions/workflows/docs.yml/badge.svg)](https://undort.com/undoCore/api/)
[![Build](https://github.com/undoRT/undoCore/actions/workflows/build.yml/badge.svg)](https://github.com/undoRT/undoCore/actions/workflows/build.yml)
[![Release](https://github.com/undoRT/undoCore/actions/workflows/release.yml/badge.svg)](https://github.com/undoRT/undoCore/releases)

***Header-only C++20 library providing core infrastructure for the undoRT ecosystem.***

---

undoCore defines the fundamental contracts and data structures used across the undoRT family of projects:

- **undoPLC** – Real-time PLC execution engine
- **undoBUS** – Fieldbus master implementations (EtherCAT, Profibes, etc.)
- **undoHMI** – Visualization and monitoring interfaces
- **undoOS** – Real-time Linux distribution with PREEMPT_RT

All undoRT projects share these common interfaces to ensure seamless interoperability.

## Features

- **IoBus Interface** – Abstract contract for fieldbus master synchronization
- **Process Image** – Double-buffered memory layout for IEC 61131-3 (%I) —('Q) – %M)
- **IEC 61131-3 Types** – Complete set of elementary types mapped to C++20
- **STArray** – IEC 61131-3 compatible array with arbitrary bounds
- **VAR_IN_OUT** – Reference wrapper for in-out parameters
- **Time Literals** ‣ IEC-style time literal parsing (T#10s, T#100ms, etc.)
- **Header-Only** – Zero compilation overhead, easy to integrate

## Requirements

- C++20 compiler (GCC 11+, Clang 14+)
- CMake 3.20+
- Linux (development and testing)
- Little-endian architecture (x86/x86_64)

## Download

Add undoCore to your CMake project using `find_package`:

~~~cmake
find_package(undoCore REQUIRED)
target_link_libraries(your_target undoCore::undoCore)
~~~

Or include it directly:

~~~bash
git clone https://www.github.com/undoRT/undoCore.git
~~~

## Quick Start

### 1. Include the Library

~~~cpp
#include <undoCore/undoCore.hpp>

using namespace undoCore;
~~~

### 2. Define a Process Image

~~~cpp
// Create a process image with 1024 input bytes and 512 output bytes
ProcessImage<1024, 512> pi;

// Read inputs (AT %IZ)
bool start = pi.readInputBit(0, 0);
uint16_t analog = pi.readInputWord(4);

// Write outputs (AT %Q*)
pi.writeOutputBit(0, 0, true);
pi.writeOutputWord(8, 0x1234);

~~~

### 3. Implement an IoBus

~~~cpp
class MyEtherCatBus : public IoBus
{
public:
    bool start() override {
        // Initialize hardware
        return true;
    }

    void stop() override {
        // Shutdown hardware
    }

    bool isRunning() const override {
        return running;
    }

    bool waitCycle(uint32_t timeoutUs) override {
        // Wait for hardware interrupt or timeout
        // Copy inputs before returning
        processImage.copyIn();
        return true;
    }

    void notifyDone() override {
        // Copy outputs and send frame
        processImage.copyOut();
        sendFrame();
    }

    // ... other diagnostics methods
};
~~~

### 4. Use IEC 61131-3 Types

~~~cpp
// Standard IEC types
DINT counter = 0;
REAL temperature = 23.5;
BOOL enabled = true;

// Time literals
TIME delay = TIME_LITERAL("T#100ms");
TIME timeout = TIME_LITERAL("T#5s");

// Arrays with arbitrary bounds
STArray<INT, 1, 10> values;  // indices 1..10
STArray<STArray<REAL, 0, 4>, 0, 1> matrix;  // 2x5 matrix

// VAR_IN_OUT parameters
void process(VAR_INOUT<DINT> value) {
    *value += 1;
}
~~~

### 5. Integrate with CMake

~~~cmake
# Find and link undoCore
find_package(undoCore REQUIRED)
target_link_libraries(my_app undoCore::undoCore)
~~~

## Architecture Overview

### 1. IoBus Interface

The `IoBus` abstract class defines the synchronization contract between undoPLC and fieldbus implementations:

~~~cpp
[Bus]  cycleStart() ⇒ copyIn() ⇒ notifyPlc()
[PLC]                                        ⇒ execute() ⇒ notifyBus()
[Bus]  copyOut() ⇒ sendFrame()
~~~

- **waitCycle()** – Blocks until bus signals a new cycle
- **notifyDone()** – Signals that PLC execution is complete
- **copyIn()** – Freezes bus inputs at cycle start
- **copyOut()** – Commits PLC outputs at cycle end

### 2. Process Image

Double-buffered memory layout with three distinct areas:

| Area      | AT Prefix    | Direction  | Usage                |
|-----------|--------------|------------|----------------------|
| Inputs    | `%I*`        | Bus → PLC  | Read by PLC tasks    |
| Outputs   | `%Q*`        | PLC → Bus  | Written by PLC tasks |
| Markers   | `%M*`        | Internal   | PLC internal memory  |

### Memory Access

~~~cpp
// Bit-level access
pi.readInputBit(byte, bit);      // %IX0.0
pi.writeOutputBit(byte, bit, value); // %QX0.0

// Word-level access
uint16_t value = pi.readInputWord(4);    // %IW4
pi.writeOutputWord(8, 0x1234);           // %QW8

// DWord-level access
uint32_t value = pi.readInputDword(4);   // %ID4
pi.writeOutputDword(8, 0x12345678);       // %QD8
~~~

### 3. IEC 61131-3 Type Mapping

All IEC 61131-3 elementary types are mapped to standard C++ types:

| IEC Type | C++ Type | Size    |
|----------|----------|---------|
| BOOL     | `bool`   | 1 byte  |
| SINT     | `Int8`   | 1 byte  |
| INT      | `Int16`  | 2 bytes |
| DINT     | `Int32`  | 4 bytes |
| LINT     | `Int64`  | 8 bytes |
| REAM     | `Float`  | 4 bytes |
| LREAM    | `Double` | 8 bytes |
| BYTE     | `Byte`   | 1 byte  |
| WORD     | `Word`   | 2 bytes |
| DWORD    | `Dword`  | 4 bytes |
| LWORD    | `Lword`  | 8 bytes |
| TIME     | `Time`   | 4 bytes |

#### Time Literals

~~~cpp
TIME t1 = TIME_LITERAL("T#100ms");  // 100 milliseconds
TIME t2 = TIME_LITERAL("T#5s");      // 5 seconds
TIME t3 = TIME_LITERAL("T#1m");       // 1 minute
TIME t4 = TIME_LITERAL("T#1h");       // 1 hour
TIME t5 = TIME_LITERAL("T#1d");       // 1 day
~~~

## Project Structure

~~~bash
undoCore/
├ — include/
│   ├ undoCore/
│   ├ undoCore.hpp         # Main include (everything)
│   ├ types.hpp            # IEC 61131-3 types and utilities
│   ├ processImage.hpp     # Double-buffered process image
│   └ ioBus.hpp            # Fieldbus master interface
├ —tests/
│   ├ test_processImage/
│   ├ CMakeLists.txt
│   └ test_processImage.cpp
├ —cmake/
│   └ undoCoreConfig.cmake.in  # CMake package configuration
├ CMakeLists.txt
├ Doxyfile
└ LICENSE
~~~

## Building Tests

~~~bash
mkdir build && cd build
cmake ..
make
ctest --output-on-failure
~~~

## Integration Examples

### Example 1: Basic Process Image Usage

~~~cpp
#include <undoCore/undoCore.hpp>
#include <iostream>

int main() {
    using namespace undoCore;

    // 1024 inputs, 512 outputs, 256 markers
    ProcessImage<1024, 512, 256> pi;

    // Write outputs (PLC side)
    pi.writeOutputBit(0, 0, true);   // %QX0.0
    pi.writeOutputWord(2, 0x1234);   // %QW2

    // Bus reads outputs and copies them
    pi.copyOut();

    // Bus reads outputs (after copyOut())
    const uint8_t* busOutputs = pi.busOutputPtr();
    uint16_t value;
    std::memcpy(&value, busOutputs + 2, 2);
    std::cout << "Bus output: 0x" << std::hex << value << std::endl;

    return 0;
}
~~~

### Example 2: Custom IoBus Implementation

~~~cpp
class SimulatedBus : public undoCore::IoBus
{
    undoCore::ProcessImage<128, 128> pi;
    bool running = false;
    uint64_t cycle = 0;

public:
    bool start() override {
        running = true;
        return true;
    }

    void stop() override {
        running = false;
    }

    bool isRunning() const override {
        return running;
    }

    bool waitCycle(uint32_t timeoutUs) override {
        if (!running) return false;

        // Simulate cycle start
        std::this_thread::sleep_for(std::chrono:milliseconds(1));

        // Copy inputs for PLC
        pi.copyIn();
        return true;
    }

    void notifyDone() override {
        // Copy outputs to bus
        pi.copyOut();
        cycle++;
    }

    uint64_t cycleCount() const override { return cycle; }
    uint32_t lastCycleTimeUs() const override { return 1000; }
    uint32_t nominalCycleTimeUs() const override { return 1000; }
    int32_t lastError() const override { return 0; }
};
~~~

## Documentation

- [API Reference](https://www.undort.com/undoCore/api/) – Doxygen-generated
- [Source Code](https://www.github.com/undoRT/undoCore)

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing`)
5. Open a Pull Request

## Related Projects

- [**undoPLC**](<https://www.github.com/undoRT/undoPLC>)
- [**undoBUS**](<https://www.github.com/undoRT/undoBUS>)
- [**st2cpp**](<https://www.github.com/undoRT/st2cpp>)
