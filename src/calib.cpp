#include "css/calib.hpp"

#include <cmath>
#include <numeric>
#include <stdexcept>
#include <Eigen/Dense>

namespace css::calib
{
    Eigen::Vector3f estimateWhiteBalance(const std::vector<Eigen::Vector3f>& measured)
    {
        if (measured.empty())
        {
            return Eigen::Vector3f::Ones();
        }

        Eigen::Vector3f sum = Eigen::Vector3f::Zero();
        for (const auto& v : measured)
        {
            sum += v;
        }

        Eigen::Vector3f mean = sum / static_cast<float>(measured.size());
        Eigen::Vector3f wb = Eigen::Vector3f::Ones();

        for (int i = 0; i < 3; ++i)
        {
            if (mean[i] > 0.0f)
            {
                wb[i] = mean.mean() / mean[i];
            }
        }

        return wb;
    }

    CalibResult solveColorMatrix(const std::vector<Eigen::Vector3f>& measuredIn,
                                 const std::vector<Eigen::Vector3f>& referenceIn,
                                 bool estimateWb,
                                 float regularization)
    {
        if (measuredIn.size() != referenceIn.size() || measuredIn.empty())
        {
            throw std::runtime_error("solveColorMatrix: mismatched or empty inputs");
        }

        const size_t n = measuredIn.size();

        Eigen::Vector3f wb = Eigen::Vector3f::Ones();
        if (estimateWb)
        {
            wb = estimateWhiteBalance(measuredIn);
        }

        // Apply white balance to measured values.
        std::vector<Eigen::Vector3f> measured(n);
        for (size_t i = 0; i < n; ++i)
        {
            measured[i] = measuredIn[i].cwiseProduct(wb);
        }

        Eigen::MatrixXf A(n, 3);
        Eigen::MatrixXf B(n, 3);

        for (size_t i = 0; i < n; ++i)
        {
            A(static_cast<int>(i), 0) = measured[i][0];
            A(static_cast<int>(i), 1) = measured[i][1];
            A(static_cast<int>(i), 2) = measured[i][2];

            B(static_cast<int>(i), 0) = referenceIn[i][0];
            B(static_cast<int>(i), 1) = referenceIn[i][1];
            B(static_cast<int>(i), 2) = referenceIn[i][2];
        }

        // Regularized least squares: (A^T A + λI) M^T = A^T B
        Eigen::Matrix3f AtA = A.transpose() * A;
        AtA += regularization * Eigen::Matrix3f::Identity();

        Eigen::Matrix3f AtB = A.transpose() * B;

        // Use LDLT decomposition for solving (more stable than direct inverse)
        Eigen::LDLT<Eigen::Matrix3f> ldlt(AtA);
        Eigen::Matrix3f M = ldlt.solve(AtB).transpose(); // rows: target channels

        // Compute errors.
        std::vector<float> perPatch;
        perPatch.reserve(n);
        float sumSq = 0.0f;

        for (size_t i = 0; i < n; ++i)
        {
            Eigen::Vector3f pred = M * measured[i];
            Eigen::Vector3f diff = pred - referenceIn[i];
            float e = diff.norm();
            perPatch.push_back(e);
            sumSq += e * e;
        }

        CalibResult res;
        res.colorMatrix = M;
        res.whiteBalance = wb;
        res.perPatchError = std::move(perPatch);
        res.rmsError = std::sqrt(sumSq / static_cast<float>(n));

        return res;
    }
} // namespace css::calib

