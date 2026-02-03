#pragma once

#include <string>
#include <opencv2/core.hpp>

#include "css/chart.hpp"
#include "css/profile.hpp"
#include "css/refdata.hpp"

namespace css::pipeline
{
    struct CalibrateConfig
    {
        chart::ChartConfig chart;
        std::string refDataCsvPath;   // e.g. data/colorchecker_24_D65.csv
        std::string illuminant = "D65";
        std::string cameraName = "camera";
    };

    /**
     * High-level calibration: from chart image to Profile.
     *
     * - Assumes input image is linear BGR float in [0,1].
     * - Uses ColorChecker 24 reference data from a CSV file.
     */
    profile::Profile calibrateFromChart(const cv::Mat& chartImage,
                                        const CalibrateConfig& cfg);

    /**
     * Apply a profile to a linear BGR image in [0,1].
     *
     * - Applies white balance and 3x3 color matrix.
     * - Optionally applies sRGB gamma (for display/export).
     */
    cv::Mat applyProfile(const cv::Mat& linearBgr,
                         const profile::Profile& prof,
                         bool applySrgbGamma = true);
} // namespace css::pipeline

