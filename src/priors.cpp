#include "css/priors.hpp"
#include <opencv2/core.hpp>
#include <stdexcept>
#include <iostream>

namespace
{
    // Helper to convert cv::Mat to Eigen::MatrixXf
    Eigen::MatrixXf cvToEigen(const cv::Mat& cvMat)
    {
        if (cvMat.empty()) return Eigen::MatrixXf();
        
        // YAML 'dt: d' means double (CV_64F)
        Eigen::MatrixXf eigMat(cvMat.rows, cvMat.cols);
        
        for(int r = 0; r < cvMat.rows; ++r)
        {
            for(int c = 0; c < cvMat.cols; ++c)
            {
                if (cvMat.type() == CV_64F)
                    eigMat(r, c) = static_cast<float>(cvMat.at<double>(r, c));
                else if (cvMat.type() == CV_32F)
                    eigMat(r, c) = cvMat.at<float>(r, c);
                else
                    throw std::runtime_error("Unsupported cv::Mat type in yaml loader (expected float or double)");
            }
        }
        return eigMat;
    }
}

namespace css::priors
{
    CameraPriors loadPriorsFromYaml(const std::string& path)
    {
        cv::FileStorage fs(path, cv::FileStorage::READ);
        if (!fs.isOpened())
        {
            throw std::runtime_error("Failed to open priors file: " + path);
        }

        CameraPriors result;
        cv::Mat r, g, b, refl;
        
        // Read keys according to assets.yaml structure
        fs["basis_r"] >> r;
        fs["basis_g"] >> g;
        fs["basis_b"] >> b;
        fs["reflectance"] >> refl;

        if (r.empty() || g.empty() || b.empty() || refl.empty())
        {
             throw std::runtime_error("Missing one or more required keys (basis_r, basis_g, basis_b, reflectance) in " + path);
        }

        result.basisR = cvToEigen(r);
        result.basisG = cvToEigen(g);
        result.basisB = cvToEigen(b);
        result.reflectance = cvToEigen(refl);

        return result;
    }
}
