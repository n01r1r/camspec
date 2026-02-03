#pragma once

#include <vector>
#include <opencv2/core.hpp>

namespace css::chart
{
    struct ChartConfig
    {
        // Outer corners of the chart in image coordinates (pixels)
        cv::Point2f topLeft{};
        cv::Point2f topRight{};
        cv::Point2f bottomRight{};
        cv::Point2f bottomLeft{};

        int rows = 4; // ColorChecker classic: 4 rows
        int cols = 6; // and 6 columns = 24 patches

        // Fraction of the patch area to use (to avoid edges), e.g. 0.7 = central 70% box.
        float innerFraction = 0.7f;
    };

    struct PatchSample
    {
        int index = 0;          // 0..(rows*cols-1), row-major
        cv::Vec3f meanBgr{};    // mean of sampled pixels
        cv::Vec3f medianBgr{};  // median (approximate) of sampled pixels
    };

    /**
     * Interactive corner picker: displays image and lets user click 4 corners.
     *
     * Click order: Patch 1 (Dark Skin, top-left), Patch 6 (Bluish Green, top-right),
     *               Patch 19 (White, bottom-left), Patch 24 (Black, bottom-right).
     *
     * Returns ChartConfig with corners filled in, or throws if user cancels/incomplete.
     */
    ChartConfig pickCornersInteractively(const cv::Mat& image);

    /**
     * Sample all patches of a ColorChecker chart.
     *
     * Assumes:
     * - Input image is linear BGR float in [0,1].
     * - ChartConfig describes the four outer corners of the 4x6 grid.
     */
    std::vector<PatchSample> sampleChartPatches(const cv::Mat& linearBgr,
                                                const ChartConfig& cfg);
} // namespace css::chart

