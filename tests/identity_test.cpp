#include <iostream>
#include <vector>

#include <Eigen/Core>

#include "css/calib.hpp"

int main()
{
    using Vec3 = Eigen::Vector3f;

    std::vector<Vec3> measured{
        Vec3(0.1f, 0.2f, 0.3f),
        Vec3(0.4f, 0.5f, 0.6f),
        Vec3(0.7f, 0.2f, 0.9f),
        Vec3(0.9f, 0.1f, 0.2f),
    };

    std::vector<Vec3> reference = measured; // identity mapping

    auto res = css::calib::solveColorMatrix(measured, reference, false, 1e-6f);

    Eigen::Matrix3f I = Eigen::Matrix3f::Identity();
    float diff = (res.colorMatrix - I).norm();

    if (diff > 1e-3f)
    {
        std::cerr << "Expected identity matrix, got diff=" << diff << std::endl;
        return 1;
    }

    if (res.rmsError > 1e-4f)
    {
        std::cerr << "Expected near-zero RMS error, got " << res.rmsError << std::endl;
        return 1;
    }

    std::cout << "identity_test passed\n";
    return 0;
}

