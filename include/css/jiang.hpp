#pragma once

#include <vector>
#include <Eigen/Core>
#include "css/priors.hpp"
#include "css/daylight.hpp"

namespace css::jiang
{
    struct JiangResult
    {
        float estimatedCct = 0.0f;
        float rmsError = 0.0f;
        Eigen::MatrixXf css;          // Recovered CSS (33 rows x 3 cols), normalized to max 1.0
        Eigen::VectorXf illuminant;   // Recovered Illuminant SPD (33x1)
    };

    class JiangEstimator
    {
    public:
        explicit JiangEstimator(const priors::CameraPriors& priors);
        
        /**
         * Solve for CSS using Jiang et al. method.
         * 
         * @param rgbPatches Observed linear RGB values (vector of size 24)
         *                   Order must match the reflectance data (Dark Skin -> Black)
         * @return Optimization result
         */
        JiangResult solve(const std::vector<Eigen::Vector3f>& rgbPatches);

    private:
        priors::CameraPriors m_priors;
        daylight::DaylightGenerator m_daylight;
    };
}
