# Discadelta: The High-Precision Rescaling Engine 🦊

[![C++23](https://img.shields.io/badge/C++-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## Overview
### Discadelta (Distance + Cascade + Delta)
Is a high-precision 1D rescaling algorithm designed to partition linear space into constrained, proportional segments. It provides a robust alternative to standard layout solvers by ensuring zero precision loss and fair redistribution of space.

### The Anatomy: Base & Delta
In Discadelta, every segment is treated as a dynamic entity composed of two distinct parts:

* **The Base (Your Compressible Budget)**
  Think of the **Base** as the segment's "ideal" starting size. This is the default compressible distance. When the window gets smaller and there isn't enough room to fit everyone, the engine "squeezes" the Base. It represents the primary distance the segment occupies.


* **The Delta (Your Fair Share of Growth)**
  The **Delta** is the engine's expansion valve. When the root distance is larger than the sum of all bases, the engine has "excess" distance to give away. The Delta is the fair share value split among sibling segments to refill that expanding distance.

### The 3-Pass Lifecycle: Decoupling Logic from Reality
Discadelta doesn't just "calculate and draw." It uses a strictly decoupled 3-pass architecture. This makes the system fast, predictable, and incredibly flexible.

* **Pass 1: Pre-compute (The Analyst)**:
  This pass validates all inputs (`min`, `max`, `base`) and establishing the "pressure" of the layout.
  It identifies whether the system needs to enter a Compression or Expansion state and establishes the priority order for processing.


* **Pass 2: Scaling (The Resolver)**:
  The core mathematical engine. It uses a `Precision Cascade` to resolve the exact final size of every segment.
  By processing segments based on their constraint priority, it ensures that "fair share" is maintained even when segments hit their min/max limits.


* **Pass 3: Dynamic Placing (The Builder)**:
  Once every segment knows its size, the Placing Pass translates those distances into world-space offsets (positions).
  Because this is a separate pass, you can swap or reorder segments visually without ever needing to recalculate the complex scaling math.

### More Details About [Discadelta Algorithm](docs/discadelta-algorithm.md)

## Integration with CPM (CMake Package Manager)
Add to your CMakeLists.txt:
```cmake
CPMAddPackage(NAME Discadelta GITHUB_REPOSITORY UFox-Engine/Discadelta GIT_TAG v1.0.1 OPTIONS "DISCADELTA_SAMPLES OFF")

add_library(Discadelta-module)
target_sources(Discadelta-module
        PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS ${Discadelta_SOURCE_DIR}
        FILES ${Discadelta_SOURCE_DIR}/core/ufox_discadelta_lib.cppm ${Discadelta_SOURCE_DIR}/core/ufox_discadelta_core.cppm
)

target_link_libraries(your_target PRIVATE Discadelta-module)
```

## Usage as Library & API (C++23 Modules)
```cpp
import ufox_discadelta_lib;  // Structs
import ufox_discadelta_core; // Functions
```

### Configuration

```cpp
std::vector<ufox::geometry::discadelta::DiscadeltaSegmentConfig> segmentConfigs{
{"Segment_1", 200.0f, 0.7f, 0.1f, 0.0f, 100.0f, 2},
{"Segment_2", 200.0f, 1.0f, 1.0f, 300.0f, 800.0f, 1},
{"Segment_3", 150.0f, 0.0f, 2.0f, 0.0f, 200.0f, 3},
{"Segment_4", 350.0f, 0.3f, 0.5f, 50.0f, 300.0f, 0}};
```
### Rescaling

```cpp
// In your code...
float rootDistance = 800.0f;
auto [segments, metrics, compressing] = ufox::geometry::discadelta::MakeContext(configs, width);

if (processingCompression) {
   ufox::geometry::discadelta::Compressing(preComputeMetrics);
}
else {
   ufox::geometry::discadelta::Expanding(preComputeMetrics);
}

ufox::geometry::discadelta::Placing(preComputeMetrics);
```
### Reorder & Replacing
```cpp
ufox::geometry::discadelta::SetSegmentOrder(preComputeMetrics, "Segment_1", 3);
ufox::geometry::discadelta::SetSegmentOrder(preComputeMetrics, "Segment_3", 2);

ufox::geometry::discadelta::Placing(preComputeMetrics);
```
