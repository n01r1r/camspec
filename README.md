# CamSpec

![C++17](https://img.shields.io/badge/standard-C%2B%2B17-blue.svg)
![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)

> **A C++17 Library for Camera Spectral Sensitivity Recovery**

CamSpec is a C++ library designed for spectral sensitivity recovery and camera calibration. It implements a flexible pipeline allowing users to switch between different optimization algorithms—from physics-based recovery to statistical estimation—within a unified framework. It currently supports constrained optimization (Jiang et al.) and provides a foundation for Off-Planckian illuminant estimation.

## Key Features

- **C++17 Implementation**: Built with standard C++17, utilizing `std::filesystem` for cross-platform I/O and strict type safety. No legacy dependencies.
- **Pipeline Architecture**: Decouples the solver (`Estimator`) from data processing (`Pipeline`). This structure allows researchers to plug in custom algorithms (e.g., Gray World, Off-Planckian) without modifying the core engine.
- **Constrained Optimization**: Implements regularized least-squares optimization with physical constraints (non-negativity, smoothness) based on Jiang et al.
- **Production Ready**: Designed as a lightweight, header-only compatible library suitable for integration into ISP tuning tools or offline calibration utilities.

## Supported Algorithms

CamSpec employs a flexible factory pattern via `EstimatorType` to switch between optimization strategies.

| Estimator Type | Status | Description | Use Case |
| :--- | :--- | :--- | :--- |
| **`JIANG_PCA`** | *In Progress* | Physics-based constrained optimization using PCA basis vectors (Jiang et al., 2013). | High-precision spectral recovery & scientific calibration. |
| **`GRAY_WORLD`** | *Planned* | Statistical estimation assuming the average scene color is neutral. | Real-time AWB, computationally lightweight. |
| **`GRAY_EDGE`** | *Planned* | Derivative-based estimation assuming average edge differences are neutral. | Scenes with uniform colored backgrounds. |
| **`OFF_PLANCKIAN`** | *Planned* | Extended optimization for non-standard light sources (LED, Fluorescent) deviating from the daylight locus. | Modern indoor lighting environments. |

## Getting Started

### System Prerequisites
- CMake 3.15+
- C++17 compliant compiler (GCC, Clang, MSVC)
- OpenCV (Image processing)

### External Modules
The following libraries are included in the `external/` directory (added via `git submodule`):
- **Eigen** for linear algebra & optimization
- **tinyDNG** for DNG file loading

*Note: Please clone this repository with `--recursive` to fetch submodules.*

### Build Instructions

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Usage Example

```cpp
#include "camspec/pipeline.hpp"
// #include "camspec/estimators/jiang.hpp" // (Planned API Structure)

int main() {
    // 1. Configure the pipeline with a specific estimator
    // auto estimator = std::make_unique<camspec::JiangEstimator>();
    // camspec::Pipeline pipeline(std::move(estimator));

    camspec::Pipeline pipeline; // Current default initialization

    // 2. Process input raw image
    auto result = pipeline.process("path/to/raw_image.dng");

    if (result) {
        result->save("profile.json");
        std::cout << "Profile recovered successfully.\n";
    }

    return 0;
}
```

## Roadmap

* **Algorithm Expansion**: Finalize the base `JIANG_PCA` estimator and implement statistical estimators (`GRAY_WORLD`, `GRAY_EDGE`).
* **Off-Planckian Support**: Research and develop specialized estimators for non-standard light sources (LED, Fluorescent).
* **CLI Tool**: Provide a standalone command-line interface for batch processing and chart extraction.
* **ISP Integration**: Optimize for integration into real-time image signal processing pipelines.
