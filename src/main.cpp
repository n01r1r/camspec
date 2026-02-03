#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

#include <opencv2/core.hpp>

#include "css/chart.hpp"
#include "css/io.hpp"
#include "css/pipeline.hpp"
#include "css/profile.hpp"

namespace fs = std::filesystem;

namespace
{
    void printUsage()
    {
        std::cout << "camspec - DNG + ColorChecker calibration\n\n"
                  << "Usage:\n"
                  << "  camspec calibrate --input chart.dng --profile-out prof.txt \\\n"
                  << "                     [--ref-data data/colorchecker_24_D65.csv] \\\n"
                  << "                     [--camera-name MyCamera] \\\n"
                  << "                     [--illuminant D65] \\\n"
                  << "                     [--corners x0,y0,x1,y1,x2,y2,x3,y3]\n"
                  << "\n"
                  << "  If --corners is omitted, an interactive corner picker will launch.\n"
                  << "  Click corners in order: Patch 1 (top-left), Patch 6 (top-right),\n"
                  << "                          Patch 19 (bottom-left), Patch 24 (bottom-right).\n"
                  << "\n"
                  << "  camspec apply --input img.dng --profile prof.txt --output out.tif\n"
                  << std::endl;
    }

    std::vector<std::string> argsFrom(int argc, char** argv, int start)
    {
        std::vector<std::string> a;
        for (int i = start; i < argc; ++i)
        {
            a.emplace_back(argv[i]);
        }
        return a;
    }

    std::string findDataFile(const std::string& filename)
    {
        // Try multiple possible locations
        std::vector<std::string> searchPaths = {
            filename,  // Current directory
            "../" + filename,
            "../data/" + filename,
            "../../" + filename,
            "../../data/" + filename,

            "../../../data/" + filename,
        };

        for (const auto& path : searchPaths)
        {
            if (fs::exists(path))
            {
                return fs::absolute(path).string();
            }
        }

        // Return original if not found (will cause error later)
        return filename;
    }

    int runCalibrate(const std::vector<std::string>& args)
    {
        std::string inputPath;
        std::string profileOutPath;
        std::string refDataPath = findDataFile("colorchecker_24_D65.csv");
        std::string cameraName = "camera";
        std::string illuminant = "D65";
        css::chart::ChartConfig chartCfg;

        bool haveCorners = false;

        for (size_t i = 0; i < args.size(); ++i)
        {
            const auto& a = args[i];
            auto next = [&](const char* opt) -> std::string {
                if (i + 1 >= args.size())
                {
                    throw std::runtime_error(std::string("Missing value for ") + opt);
                }
                return args[++i];
            };

            if (a == "--input")
            {
                inputPath = next("--input");
            }
            else if (a == "--profile-out")
            {
                profileOutPath = next("--profile-out");
            }
            else if (a == "--ref-data")
            {
                refDataPath = next("--ref-data");
                // Convert to absolute path if relative
                if (!fs::path(refDataPath).is_absolute())
                {
                    if (fs::exists(refDataPath))
                    {
                        refDataPath = fs::absolute(refDataPath).string();
                    }
                }
            }
            else if (a == "--camera-name")
            {
                cameraName = next("--camera-name");
            }
            else if (a == "--illuminant")
            {
                illuminant = next("--illuminant");
            }
            else if (a == "--corners")
            {
                std::string val = next("--corners");
                // Expect x0,y0,x1,y1,x2,y2,x3,y3
                std::vector<float> coords;
                std::string token;
                std::stringstream ss(val);
                while (std::getline(ss, token, ','))
                {
                    coords.push_back(std::stof(token));
                }
                if (coords.size() != 8)
                {
                    throw std::runtime_error("Expected 8 comma-separated values for --corners");
                }
                chartCfg.topLeft = {coords[0], coords[1]};
                chartCfg.topRight = {coords[2], coords[3]};
                chartCfg.bottomRight = {coords[4], coords[5]};
                chartCfg.bottomLeft = {coords[6], coords[7]};
                haveCorners = true;
            }
        }

        if (inputPath.empty() || profileOutPath.empty())
        {
            throw std::runtime_error("calibrate: missing required arguments (--input and --profile-out)");
        }

        std::cout << "Loading DNG image: " << inputPath << std::endl;
        cv::Mat img = css::io::loadDngAsLinearRgb(inputPath);
        std::cout << "Image loaded: " << img.cols << "x" << img.rows << std::endl;

        // If corners not provided via CLI, use interactive picker
        if (!haveCorners)
        {
            std::cout << "No --corners provided, launching interactive corner picker..." << std::endl;
            chartCfg = css::chart::pickCornersInteractively(img);
        }

        std::cout << "Loading reference data: " << refDataPath << std::endl;
        css::pipeline::CalibrateConfig cfg;
        cfg.chart = chartCfg;
        cfg.refDataCsvPath = refDataPath;
        cfg.illuminant = illuminant;
        cfg.cameraName = cameraName;

        std::cout << "Running calibration..." << std::endl;
        auto prof = css::pipeline::calibrateFromChart(img, cfg);

        if (!css::profile::saveProfile(profileOutPath, prof))
        {
            throw std::runtime_error("Failed to save profile to " + profileOutPath);
        }

        std::cout << "Calibration complete.\n"
                  << "  Camera: " << prof.cameraName << "\n"
                  << "  Illuminant: " << prof.illuminant << "\n"
                  << "  Profile: " << profileOutPath << std::endl;

        return 0;
    }

    int runApply(const std::vector<std::string>& args)
    {
        std::string inputPath;
        std::string profilePath;
        std::string outputPath;

        for (size_t i = 0; i < args.size(); ++i)
        {
            const auto& a = args[i];
            auto next = [&](const char* opt) -> std::string {
                if (i + 1 >= args.size())
                {
                    throw std::runtime_error(std::string("Missing value for ") + opt);
                }
                return args[++i];
            };

            if (a == "--input")
            {
                inputPath = next("--input");
            }
            else if (a == "--profile")
            {
                profilePath = next("--profile");
            }
            else if (a == "--output")
            {
                outputPath = next("--output");
            }
        }

        if (inputPath.empty() || profilePath.empty() || outputPath.empty())
        {
            throw std::runtime_error("apply: missing required arguments");
        }

        cv::Mat img = css::io::loadDngAsLinearRgb(inputPath);
        auto prof = css::profile::loadProfile(profilePath);

        cv::Mat corrected = css::pipeline::applyProfile(img, prof, true);
        css::io::saveImage(outputPath, corrected, 16);

        std::cout << "Applied profile and wrote " << outputPath << std::endl;
        return 0;
    }
} // namespace

int main(int argc, char** argv)
{
    // Force output to be visible immediately - use unbuffered mode
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);
    
    std::cout << "camspec starting... (argc=" << argc << ")" << std::endl;
    std::cout.flush();
    
    if (argc < 2)
    {
        std::cout << "No command provided, showing usage..." << std::endl;
        printUsage();
        return 1;
    }

    std::string cmd = argv[1];
    std::cout << "Command: " << cmd << std::endl;
    
    auto args = argsFrom(argc, argv, 2);
    std::cout << "Number of arguments: " << args.size() << std::endl;

    try
    {
        if (cmd == "calibrate")
        {
            std::cout << "Running calibrate command..." << std::endl;
            int result = runCalibrate(args);
            std::cout << "Calibrate command completed with code: " << result << std::endl;
            return result;
        }
        if (cmd == "apply")
        {
            std::cout << "Running apply command..." << std::endl;
            int result = runApply(args);
            std::cout << "Apply command completed with code: " << result << std::endl;
            return result;
        }

        std::cout << "Unknown command: " << cmd << std::endl;
        printUsage();
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        std::cerr.flush();
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown exception occurred!" << std::endl;
        std::cerr.flush();
        return 1;
    }
}

