#include "css/profile.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace css::profile
{
    bool saveProfile(const std::string& path, const Profile& p)
    {
        std::ofstream out(path);
        if (!out)
        {
            return false;
        }

        out << "cameraName=" << p.cameraName << "\n";
        out << "illuminant=" << p.illuminant << "\n";
        out << "chartType=" << p.chartType << "\n";
        out << "targetColorSpace=" << p.targetColorSpace << "\n";

        out << "M=";
        for (int r = 0; r < 3; ++r)
        {
            for (int c = 0; c < 3; ++c)
            {
                out << p.colorMatrix(r, c);
                if (!(r == 2 && c == 2))
                {
                    out << " ";
                }
            }
        }
        out << "\n";

        out << "wb=" << p.whiteBalance[0] << " "
            << p.whiteBalance[1] << " "
            << p.whiteBalance[2] << "\n";

        return true;
    }

    Profile loadProfile(const std::string& path)
    {
        std::ifstream in(path);
        if (!in)
        {
            throw std::runtime_error("Failed to open profile: " + path);
        }

        Profile p;
        p.chartType = "ColorChecker24";
        p.targetColorSpace = "linear_srgb";

        std::string line;
        while (std::getline(in, line))
        {
            if (line.empty())
                continue;

            auto pos = line.find('=');
            if (pos == std::string::npos)
                continue;

            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            if (key == "cameraName")
            {
                p.cameraName = value;
            }
            else if (key == "illuminant")
            {
                p.illuminant = value;
            }
            else if (key == "chartType")
            {
                p.chartType = value;
            }
            else if (key == "targetColorSpace")
            {
                p.targetColorSpace = value;
            }
            else if (key == "M")
            {
                std::stringstream ss(value);
                for (int r = 0; r < 3; ++r)
                {
                    for (int c = 0; c < 3; ++c)
                    {
                        ss >> p.colorMatrix(r, c);
                    }
                }
            }
            else if (key == "wb")
            {
                std::stringstream ss(value);
                ss >> p.whiteBalance[0]
                   >> p.whiteBalance[1]
                   >> p.whiteBalance[2];
            }
        }

        return p;
    }
} // namespace css::profile

