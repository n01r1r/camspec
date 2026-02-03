# camspec: C++ Camera Spectral Sensitivity & Color Pipeline

A C++ pipeline for camera color calibration and spectral sensitivity estimation, based on the research below:

> **Original Research**: [What is the Space of Spectral Sensitivity Functions for Digital Color Cameras?](https://www.gujinwei.org/research/camspec/) (WACV 2013)

## Features

- **Raw Processing**: Uses `tinydng` to load custom 16-bit CFA/Bayer and Linear DNGs.
- **Color Calibration**: Extracts 24-patch ColorChecker values to estimate a 3x3 Color Correction Matrix (CCM) and White Balance.
- **Profile Management**: Saves and applies calibration profiles to raw images.
- **Interactive Tool**: built-in GUI for easy corner selection.

## Build & Usage

### 1. Build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### 2. Calibrate
Calculate a profile from a DNG containing a ColorChecker:
```bash
# Launches interactive corner picker
camspec calibrate --input ../raw/IMG_0001.DNG --profile-out ../profiles/IMG_0001_D65.txt
```

### 3. Apply
Apply the profile to correct other images from the same shoot:
```bash
camspec apply --input ../raw/IMG_0002.DNG --profile ../profiles/IMG_0001_D65.txt --output ../raw/IMG_0002_corrected.tif
```

## TODO

The goal is to fully port the authors' original Matlab implementation to C++, with additional features if possible.

1.  **Spectral Sensitivity Recovery**: Implement PCA-based estimation of Camera Spectral Sensitivity (CSS) from a single image.
2.  **Illuminant Estimation**: Add CCT optimization to estimate unknown daylight spectra.
3.  **Database Integration**: Incorporate the PCA basis from the [Camera Spectral Sensitivity Database](https://www.gujinwei.org/research/camspec/).
