#pragma once

#include <string>
#include <vector>
#include <Eigen/Core>

namespace css::refdata
{
    struct PatchRef
    {
        int index = 0;                 // 0..23
        std::string name;              // e.g. "Dark Skin"
        Eigen::Vector3f linearSrgb{};  // linear sRGB in [0,1]
    };

    struct RefSet
    {
        std::string illuminant;        // e.g. "D65"
        std::string colorSpace;        // e.g. "linear_srgb"
        std::vector<PatchRef> patches; // size 24 for classic chart
    };

    /**
     * Load ColorChecker 24-patch reference data from a CSV file.
     *
     * Expected CSV format (no header required, but allowed):
     *   index,name,R,G,B
     * where R,G,B are linear sRGB values in [0,1].
     */
    RefSet loadColorChecker24Csv(const std::string& path,
                                 const std::string& illuminant,
                                 const std::string& colorSpace = "linear_srgb");

} // namespace css::refdata

