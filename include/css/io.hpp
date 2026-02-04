#pragma once

#include <string>
#include <opencv2/core.hpp>

namespace css::io
{
    /**
     * Load a DNG/RAW (or any OpenCV-readable) image as linear RGB in [0,1].
     *
     * - Uses cv::imread with IMREAD_UNCHANGED.
     * - Converts to 32-bit float.
     * - Normalizes by an estimated black/white level if metadata is unavailable.
     *
     * The returned image uses OpenCV's default channel order (BGR).
     */
    cv::Mat loadDngAsLinearRgb(const std::string& path);

    /**
     * Save a linear RGB/BGR float image in [0,1] to disk.
     *
     * - Optionally converts to 8-bit or 16-bit before writing.
     * - Clamps values to [0,1].
     */
    void saveImage(const std::string& path,
                   const cv::Mat& image,
                   int bitDepth = 16);
} // namespace css::io

