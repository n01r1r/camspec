#pragma once

#include <string>
#include <vector>
#include <Eigen/Core>

namespace css::spectral
{
    struct SpectralSample
    {
        float wavelengthNm = 0.0f;
        Eigen::Vector3f rgbResponse = Eigen::Vector3f::Zero();
    };

    struct SpectralSensitivity
    {
        std::string cameraName;
        std::vector<SpectralSample> samples;
    };

    /**
     * Load camera spectral sensitivity from a CSV file.
     *
     * Expected CSV format:
     *   wavelength_nm,R,G,B
     */
    SpectralSensitivity loadSpectralSensitivityCsv(const std::string& path,
                                                   const std::string& cameraName = "");
} // namespace css::spectral

