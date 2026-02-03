#include "css/spectral.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace css::spectral
{
    SpectralSensitivity loadSpectralSensitivityCsv(const std::string& path,
                                                   const std::string& cameraName)
    {
        std::ifstream in(path);
        if (!in)
        {
            throw std::runtime_error("Failed to open spectral CSV: " + path);
        }

        SpectralSensitivity sens;
        sens.cameraName = cameraName;

        std::string line;
        bool firstLine = true;
        while (std::getline(in, line))
        {
            if (line.empty())
                continue;

            if (firstLine)
            {
                firstLine = false;
                // Heuristically skip header if first token is not numeric.
                std::stringstream test(line);
                std::string token;
                if (std::getline(test, token, ','))
                {
                    try
                    {
                        (void)std::stof(token);
                    }
                    catch (...)
                    {
                        continue; // treat as header
                    }
                }
            }

            std::stringstream ss(line);
            std::string token;

            SpectralSample s;

            // wavelength
            if (!std::getline(ss, token, ','))
                continue;
            s.wavelengthNm = std::stof(token);

            float r = 0.0f, g = 0.0f, b = 0.0f;

            if (!std::getline(ss, token, ','))
                continue;
            r = std::stof(token);

            if (!std::getline(ss, token, ','))
                continue;
            g = std::stof(token);

            if (!std::getline(ss, token, ','))
                continue;
            b = std::stof(token);

            s.rgbResponse = Eigen::Vector3f(r, g, b);
            sens.samples.push_back(s);
        }

        if (sens.samples.empty())
        {
            throw std::runtime_error("No samples read from spectral CSV: " + path);
        }

        return sens;
    }
} // namespace css::spectral

