#pragma once

#include <vector>
#include <Eigen/Core>

namespace css::calib
{
    struct CalibResult
    {
        Eigen::Matrix3f colorMatrix = Eigen::Matrix3f::Identity(); // camera RGB -> target (linear sRGB)
        Eigen::Vector3f whiteBalance = Eigen::Vector3f::Ones();    // per-channel gains
        float rmsError = 0.0f;
        std::vector<float> perPatchError;                          // same order as inputs
    };

    /**
     * Estimate white-balance gains from measured patch RGBs.
     *
     * Simple heuristic: use the average of all patches as a grey estimate.
     */
    Eigen::Vector3f estimateWhiteBalance(const std::vector<Eigen::Vector3f>& measured);

    /**
     * Solve for a 3x3 color matrix mapping camera RGB to target RGB.
     *
     * measured  - camera-space linear RGB samples (after applying white balance, if desired)
     * reference - target-space linear RGB reference values
     */
    CalibResult solveColorMatrix(const std::vector<Eigen::Vector3f>& measured,
                                 const std::vector<Eigen::Vector3f>& reference,
                                 bool estimateWb = true,
                                 float regularization = 1e-4f);
} // namespace css::calib

