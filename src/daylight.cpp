#include "css/daylight.hpp"
#include <cmath>

namespace css::daylight
{
    DaylightGenerator::DaylightGenerator()
    {
        // Initialize basis vectors S0, S1, S2 for 400-720nm @ 10nm
        // Source: daylightScalars.txt from reference/MATLAB code
        m_basis.resize(33, 3);
        
        // Data format: S0, S1, S2 (ignoring wavelength column which is implicit 400+10*i)
        // 400
        m_basis.row(0) << 94.8f, 43.4f, -1.1f;
        // 410
        m_basis.row(1) << 104.8f, 46.3f, -0.5f;
        // 420
        m_basis.row(2) << 105.9f, 43.9f, -0.7f;
        // 430
        m_basis.row(3) << 96.8f, 37.1f, -1.2f;
        // 440
        m_basis.row(4) << 113.9f, 36.7f, -2.6f;
        // 450
        m_basis.row(5) << 125.6f, 35.9f, -2.9f;
        // 460
        m_basis.row(6) << 125.5f, 32.6f, -2.8f;
        // 470
        m_basis.row(7) << 121.3f, 27.9f, -2.6f;
        // 480
        m_basis.row(8) << 121.3f, 24.3f, -2.6f;
        // 490
        m_basis.row(9) << 113.5f, 20.1f, -1.8f;
        // 500
        m_basis.row(10) << 113.1f, 16.2f, -1.5f;
        // 510
        m_basis.row(11) << 110.8f, 13.2f, -1.3f;
        // 520
        m_basis.row(12) << 106.5f, 8.6f, -1.2f;
        // 530
        m_basis.row(13) << 108.8f, 6.1f, -1.0f;
        // 540
        m_basis.row(14) << 105.3f, 4.2f, -0.5f;
        // 550
        m_basis.row(15) << 104.4f, 1.9f, -0.3f;
        // 560
        m_basis.row(16) << 100.0f, 0.0f, 0.0f;
        // 570
        m_basis.row(17) << 96.0f, -1.6f, 0.2f;
        // 580
        m_basis.row(18) << 95.1f, -3.5f, 0.5f;
        // 590
        m_basis.row(19) << 89.1f, -3.5f, 2.1f;
        // 600
        m_basis.row(20) << 90.5f, -5.8f, 3.2f;
        // 610
        m_basis.row(21) << 90.3f, -7.2f, 4.1f;
        // 620
        m_basis.row(22) << 88.4f, -8.6f, 4.7f;
        // 630
        m_basis.row(23) << 84.0f, -9.5f, 5.1f;
        // 640
        m_basis.row(24) << 85.1f, -10.9f, 6.7f;
        // 650
        m_basis.row(25) << 81.9f, -10.7f, 7.3f;
        // 660
        m_basis.row(26) << 82.6f, -12.0f, 8.6f;
        // 670
        m_basis.row(27) << 84.9f, -14.0f, 9.8f;
        // 680
        m_basis.row(28) << 81.3f, -13.6f, 10.2f;
        // 690
        m_basis.row(29) << 71.9f, -12.0f, 8.3f;
        // 700
        m_basis.row(30) << 74.3f, -13.3f, 9.6f;
        // 710
        m_basis.row(31) << 76.4f, -12.9f, 8.5f;
        // 720
        m_basis.row(32) << 63.3f, -10.6f, 7.0f;
    }

    Eigen::VectorXf DaylightGenerator::generate(float cct) const
    {
        // Logic from getDaylightScalars.m
        
        // 1. Calculate xD (CIE chromaticity x coordinate)
        double xD = 0.0;
        if (cct >= 4000.0f && cct <= 7000.0f)
        {
            xD = -4.607e9 / std::pow(cct, 3) 
                 + 2.9678e6 / std::pow(cct, 2) 
                 + 0.09911e3 / cct 
                 + 0.244063;
        }
        else
        {
            xD = -2.0064e9 / std::pow(cct, 3) 
                 + 1.9018e6 / std::pow(cct, 2) 
                 + 0.24748e3 / cct 
                 + 0.23704;
        }

        // 2. Calculate yD
        double yD = -3.0 * xD * xD + 2.87 * xD - 0.275;

        // 3. Calculate Scalars M1, M2
        // Denom common: 0.0241 + 0.2562*xD - 0.7341*yD
        double denom = 0.0241 + 0.2562 * xD - 0.7341 * yD;
        
        double M1 = (-1.3515 - 1.7703 * xD + 5.9114 * yD) / denom;
        double M2 = (0.03 - 31.4424 * xD + 30.0717 * yD) / denom;

        // 4. Combine Basis
        // SD = S0 + M1*S1 + M2*S2
        Eigen::VectorXf S0 = m_basis.col(0);
        Eigen::VectorXf S1 = m_basis.col(1);
        Eigen::VectorXf S2 = m_basis.col(2);

        Eigen::VectorXf SD = S0 + (float)M1 * S1 + (float)M2 * S2;

        return SD;
    }
}
