### camspec: C++ Camera Spectral Sensitivity & Color Pipeline

This project provides a C++ pipeline for:

- **Reading custom DNG/RAW files** into a linear RGB representation.
- **Sampling a 24‑patch ColorChecker chart** from the image.
- **Estimating a camera color calibration** (3×3 matrix + white balance).
- **Saving/loading calibration profiles** and **applying** them to other images.

### Folder layout

- **`camspec/`**: C++ source tree (this directory).
  - **`raw/`**: Example/custom DNG/RAW images containing a 24‑patch ColorChecker.
  - **`camSpecSensitivity/`**: Camera spectral sensitivity files (optional, CSV).
  - **`include/`**: Public C++ headers (`css::io`, `css::chart`, `css::calib`, `css::profile`, `css::pipeline`, etc.).
  - **`src/`**: C++ implementation files.
  - **`data/`**: Reference data such as 24‑patch ColorChecker values.
  - **`profiles/`**: Saved calibration profiles created by the tool.
  - **`tests/`**: Simple unit/regression tests.

### DNG / RAW assumptions

- **Input images** are expected to be:
  - **DNG/RAW files** (CFA/Bayer or Linear). The pipeline uses `tinydng` to load raw data.
  - Already-demosaiced 8-bit/16-bit RGB images (e.g. TIFF/PNG) are treated as linear.
- The loader converts images to **32‑bit floating‑point linear RGB** in \[0, 1\].
- **Black/White Levels**: Handled via DNG tags (using `tinydng`) or estimated if missing.
- **Demosaicing**: A simple gradient-based demosaicing (or bilinear depending on OpenCV version) is applied to CFA images.

### 24‑patch ColorChecker assumptions

- The chart is a standard **X‑Rite/Calibrite ColorChecker Classic 24‑patch**.
- The chart is approximately:
  - **Fronto‑parallel** to the camera (small perspective distortion is OK).
  - **Well‑exposed** without heavy clipping in highlights/shadows.
- **Phase 1 (implemented here)**:
  - **Interactive corner picker**: Launch an OpenCV window to click the 4 corners directly on the image.
  - **Manual coordinates**: Provide corner coordinates via `--corners` CLI argument.
  - The tool computes the grid of 4×6 patches via a homography from the selected corners.
- **Phase 2 (future work)**:
  - Automatic detection of the chart using OpenCV (edges/contours, grid fitting, or fiducial markers).

### Target color space

- The internal calibration target is **linear sRGB**:
  - Reference patch values are stored as **linear sRGB triplets** in \[0, 1\].
  - Calibration computes a **3×3 matrix** mapping camera linear RGB → linear sRGB.
- The output pipeline can:
  - Keep images in **linear sRGB** (for further processing), or
  - Convert to **display sRGB** (apply sRGB gamma and quantize to 8‑bit/16‑bit for export).

### High‑level workflow

1. Capture a DNG/RAW image of a 24‑patch ColorChecker under a known illuminant (e.g., D65).
2. Run **calibration**:
   - Provide the chart image, corner coordinates, camera name, and illuminant.
   - The tool samples each patch, compares against reference values, and solves for a 3×3 color matrix + white balance.
   - A **profile** file is saved in `profiles/`.
3. Run **apply**:
   - Supply arbitrary DNG/RAW images and the profile file.
   - The tool applies white balance, the color matrix, and (optionally) sRGB gamma.
   - Outputs corrected images (e.g., TIFF/PNG).

See the CLI usage comments in `src/main.cpp` (once generated) for concrete command‑line examples.

### Example usage with `raw/IMG_0001.DNG`, `IMG_0002.DNG`, `IMG_0003.DNG`

- **Build the project** (from the `camspec` directory):

  - Configure:
    - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
  - Build:
    - `cmake --build build --config Release`

- **Calibrate** from a chart shot (example with `IMG_0001.DNG`):

  **Option A: Interactive corner picker (recommended)**
  
  Run (from the build directory):
  ```bash
  camspec calibrate --input ../raw/IMG_0001.DNG --profile-out ../profiles/IMG_0001_D65.txt
  ```
  
  An interactive window will open. Click the 4 corners in this order:
  1. Patch 1 (Dark Skin) - top-left corner
  2. Patch 6 (Bluish Green) - top-right corner  
  3. Patch 19 (White) - bottom-left corner
  4. Patch 24 (Black) - bottom-right corner
  
  Press any key after selecting all 4 corners to continue.

  **Option B: Manual corner coordinates**
  
  If you know the pixel coordinates, you can provide them directly:
  ```bash
  camspec calibrate --input ../raw/IMG_0001.DNG --profile-out ../profiles/IMG_0001_D65.txt \
    --ref-data ../data/colorchecker_24_D65.csv --camera-name MyCamera --illuminant D65 \
    --corners x0,y0,x1,y1,x2,y2,x3,y3
  ```
  
  Where `(x0,y0)` = top-left, `(x1,y1)` = top-right, `(x2,y2)` = bottom-right, `(x3,y3)` = bottom-left.

- **Apply the profile** to any of the DNG files (for example, `IMG_0002.DNG`):

  - `camspec apply --input ../raw/IMG_0002.DNG --profile ../profiles/IMG_0001_D65.txt --output ../raw/IMG_0002_corrected.tif`

  You can repeat this for `IMG_0003.DNG` or any other DNGs from the same camera/lighting setup.

