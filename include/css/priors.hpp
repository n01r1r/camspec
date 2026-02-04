#pragma once

#include <string>
#include <Eigen/Core>

namespace css::priors
{
    struct CameraPriors
    {
        // PCA Basis vectors for each channel
        // Rows = Wavelengths (e.g. 33), Cols = Components (e.g. 3 or 6)
        Eigen::MatrixXf basisR;
        Eigen::MatrixXf basisG;
        Eigen::MatrixXf basisB;
        
        // Spectral Reflectance of the chart patches
        // Rows = Wavelengths (33), Cols = Num Patches (24)
        Eigen::MatrixXf reflectance;
    };

    /**
     * Load priors from an OpenCV YAML file (e.g. assets.yaml).
     * Expected keys: 'basis_r', 'basis_g', 'basis_b', 'reflectance'.
     * Throws std::runtime_error on failure.
     */
    CameraPriors loadPriorsFromYaml(const std::string& path);
}
