#include "css/refdata.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace css::refdata
{
    RefSet loadColorChecker24Csv(const std::string& path,
                                 const std::string& illuminant,
                                 const std::string& colorSpace)
    {
        std::ifstream in(path);
        if (!in)
        {
            throw std::runtime_error("Failed to open reference CSV: " + path);
        }

        RefSet set;
        set.illuminant = illuminant;
        set.colorSpace = colorSpace;

        std::string line;
        bool firstLine = true;
        while (std::getline(in, line))
        {
            if (line.empty())
                continue;

            // Skip header if present (contains non-numeric chars in first token).
            if (firstLine)
            {
                firstLine = false;
                std::stringstream test(line);
                std::string token;
                if (std::getline(test, token, ','))
                {
                    try
                    {
                        (void)std::stoi(token);
                    }
                    catch (...)
                    {
                        continue; // treat as header
                    }
                }
            }

            std::stringstream ss(line);
            std::string token;

            PatchRef p;

            // index
            if (!std::getline(ss, token, ','))
                continue;
            p.index = std::stoi(token);

            // name
            if (!std::getline(ss, p.name, ','))
                continue;

            // R, G, B
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

            p.linearSrgb = Eigen::Vector3f(r, g, b);
            set.patches.push_back(p);
        }

        if (set.patches.size() != 24)
        {
            throw std::runtime_error("Expected 24 patches in reference CSV, got " +
                                     std::to_string(set.patches.size()));
        }

        return set;
    }
} // namespace css::refdata

