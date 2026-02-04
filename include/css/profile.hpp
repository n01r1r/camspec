#pragma once

#include <string>
#include <Eigen/Core>

namespace css::profile
{
    struct Profile
    {
        std::string cameraName;
        std::string illuminant;       // e.g. "D65"
        std::string chartType;        // e.g. "ColorChecker24"
        std::string targetColorSpace; // e.g. "linear_srgb"

        Eigen::Matrix3f colorMatrix = Eigen::Matrix3f::Identity();
        Eigen::Vector3f whiteBalance = Eigen::Vector3f::Ones();
    };

    /**
     * Save a profile to a simple text file.
     *
     * Format:
     *   cameraName=...
     *   illuminant=...
     *   chartType=ColorChecker24
     *   targetColorSpace=linear_srgb
     *   M=m00 m01 m02 m10 m11 m12 m20 m21 m22
     *   wb=w0 w1 w2
     */
    bool saveProfile(const std::string& path, const Profile& p);

    /**
     * Load a profile from a text file written by saveProfile.
     */
    Profile loadProfile(const std::string& path);
} // namespace css::profile

