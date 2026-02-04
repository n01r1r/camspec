#pragma once

#include <Eigen/Core>

namespace css::daylight
{
    /**
     * Generates standard CIE Daylight SPD for a given CCT.
     * Uses the Judd et al. basis vectors (S0, S1, S2).
     * Output range is 400nm - 720nm @ 10nm steps (33 values),
     * matching the project's standard spectral range.
     */
    class DaylightGenerator
    {
    public:
        DaylightGenerator();

        /**
         * Generate Relative Spectral Power Distribution for a given CCT.
         * The result is normalized such that the value at 560nm is 100 (or similar relative scaling).
         * 
         * @param cct Correlated Color Temperature in Kelvin (e.g. 6500)
         * @return 33x1 vector of relative power
         */
        Eigen::VectorXf generate(float cct) const;

        const Eigen::MatrixXf& getBasis() const { return m_basis; }

    private:
        // 33 rows (wavelengths 400..720), 3 cols (S0, S1, S2)
        Eigen::MatrixXf m_basis;
    };
}
