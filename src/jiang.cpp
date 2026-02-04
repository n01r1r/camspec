#include "css/jiang.hpp"
#include <iostream>
#include <cmath>
#include <limits>
#include <Eigen/Dense>

namespace css::jiang
{
    JiangEstimator::JiangEstimator(const priors::CameraPriors& priors)
        : m_priors(priors)
    {
    }

    JiangResult JiangEstimator::solve(const std::vector<Eigen::Vector3f>& rgbPatches)
    {
        // 1. Validate Input
        if (rgbPatches.size() != 24)
        {
            throw std::runtime_error("JiangEstimator expects exactly 24 patches.");
        }
        if (m_priors.reflectance.cols() != 24)
        {
            throw std::runtime_error("Reflectance prior cols != 24.");
        }

        // Convert input patches to matrix (24 x 3)
        Eigen::MatrixXf observations(24, 3);
        for(size_t i = 0; i < 24; ++i)
        {
            observations.row(i) = rgbPatches[i];
        }

        float deltaLambda = 10.0f;
        
        // 2. Optimization Loop
        float bestError = std::numeric_limits<float>::max();
        float bestCct = 0.0f;
        Eigen::MatrixXf bestCss(33, 3);
        Eigen::VectorXf bestIll(33);

        // Search Range from MATLAB script: 4000 to 27000 step 100
        for (int cct = 4000; cct <= 27000; cct += 100)
        {
            float fCct = static_cast<float>(cct);
            
            // Generate Illuminant SPD (33x1)
            Eigen::VectorXf ill = m_daylight.generate(fCct);

            // Construct 'Radiance' under this illuminant
            // Refl (33x24) * Diag(ill)
            // Equivalent to col-wise multiplication
            Eigen::MatrixXf R_ill = m_priors.reflectance; 
            for (int r = 0; r < R_ill.rows(); ++r)
            {
                R_ill.row(r) *= ill(r);
            }

            // Solve for each channel
            Eigen::MatrixXf currentCss(33, 3);
            float currentSquaredError = 0.0f;

            // Pointers to basis matrices for convenient indexing
            // 0=R, 1=G, 2=B
            const Eigen::MatrixXf* bases[] = { &m_priors.basisR, &m_priors.basisG, &m_priors.basisB };

            for (int ch = 0; ch < 3; ++ch)
            {
                const Eigen::MatrixXf& E = *bases[ch]; // (33 x K)
                
                // System Matrix A = (R_ill^T * E) * deltaLambda
                // R_ill is (33x24), R_ill^T is (24x33)
                // A is (24 x K)
                Eigen::MatrixXf A = (R_ill.transpose() * E) * deltaLambda;

                // b is (24x1) observed values for this channel
                Eigen::VectorXf b = observations.col(ch);

                // Solve Ax = b (Least Squares)
                // Using BDCSVD for robustness
                Eigen::VectorXf x = A.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(b);

                // Reconstruct b_hat to check error
                Eigen::VectorXf b_hat = A * x;
                currentSquaredError += (b - b_hat).squaredNorm();

                // Reconstruct CSS = E * x
                currentCss.col(ch) = E * x;
            }

            float rmsError = std::sqrt(currentSquaredError);

            if (rmsError < bestError)
            {
                bestError = rmsError;
                bestCct = fCct;
                bestCss = currentCss;
                bestIll = ill;
            }
        }

        // 3. Post-Process
        // Clip negatives
        bestCss = bestCss.cwiseMax(0.0f);
        
        // Normalize max value to 1.0
        float maxVal = bestCss.maxCoeff();
        if (maxVal > 1e-6f)
        {
            bestCss /= maxVal;
        }

        JiangResult res;
        res.estimatedCct = bestCct;
        res.rmsError = bestError;
        res.css = bestCss;
        res.illuminant = bestIll;

        return res;
    }
}
