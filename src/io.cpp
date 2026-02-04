#define NOMINMAX
#include "css/io.hpp"
#include <iostream>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#define TINY_DNG_LOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "tiny_dng_loader.h"

// Helper to determine OpenCV Bayer pattern code
// pattern[0][0], [0][1], [1][0], [1][1] are indices into cfa_plane_color
// cfa_plane_color maps index -> Color (0=Red, 1=Green, 2=Blue)
// We assume standard RGB ordering for now.
static int getOpenCVBayerCode(const tinydng::DNGImage& img)
{
    // Check if it's a standard Bayer pattern
    if (img.cfa_pattern_dim != 2) return -1; // Only 2x2 supported for now

    // Map 2x2 pattern indices to RGB
    // 0=Red, 1=Green, 2=Blue (usually)
    // Validate mapping just in case
    int p00 = img.cfa_plane_color[img.cfa_pattern[0][0]];
    int p01 = img.cfa_plane_color[img.cfa_pattern[0][1]];
    int p10 = img.cfa_plane_color[img.cfa_pattern[1][0]];
    int p11 = img.cfa_plane_color[img.cfa_pattern[1][1]];

    // R=0, G=1, B=2
    if (p00 == 0 && p01 == 1 && p10 == 1 && p11 == 2) return cv::COLOR_BayerRG2BGR;
    if (p00 == 2 && p01 == 1 && p10 == 1 && p11 == 0) return cv::COLOR_BayerBG2BGR;
    if (p00 == 1 && p01 == 0 && p10 == 2 && p11 == 1) return cv::COLOR_BayerGR2BGR;
    if (p00 == 1 && p01 == 2 && p10 == 0 && p11 == 1) return cv::COLOR_BayerGB2BGR;

    return -1; // Unknown
}

namespace css::io
{
    namespace
    {
        // Moved saveImage implementation to keep file structure clean, but keeping the helper here if needed.
        // The original toFloat01 and normalizeBlackWhite might not be needed if we do custom processing.
    }

    cv::Mat loadDngAsLinearRgb(const std::string& path)
    {
        std::string warn, err;
        std::vector<tinydng::DNGImage> images;
        std::vector<tinydng::FieldInfo> custom_fields;

        bool ret = tinydng::LoadDNG(path.c_str(), custom_fields, &images, &warn, &err);
        
        if (!warn.empty()) {
            std::cerr << "DNG Warning: " << warn << std::endl;
        }

        if (!ret || images.empty()) {
            throw std::runtime_error("Failed to load DNG: " + path + " (" + err + ")");
        }

        const auto& dng = images[0];

        // Ensure we have data
        if (dng.data.empty()) {
             throw std::runtime_error("DNG has no data");
        }

        // Create raw Mat
        int width = dng.width;
        int height = dng.height;
        cv::Mat raw;
        
        if (dng.bits_per_sample > 8) {
            // Assume 16-bit
             raw = cv::Mat(height, width, CV_16UC1, (void*)dng.data.data()).clone();
        } else {
             raw = cv::Mat(height, width, CV_8UC1, (void*)dng.data.data()).clone();
        }

        // Subtract Black Level (Integer) - simple approx using first value
        // Note: cv::subtract handles saturation (clamping to 0) for unsigned types
        cv::subtract(raw, cv::Scalar(dng.black_level[0]), raw);

        // Demosaic (Must be 8/16U) - raw is modified in-place above
        cv::Mat rgb;
        
        if (dng.samples_per_pixel == 1) {
            // CFA -> Debayer
            int code = getOpenCVBayerCode(dng);
            if (code == -1) {
                std::cerr << "Warning: Unknown Bayer pattern, assuming RGGB" << std::endl;
                code = cv::COLOR_BayerRG2BGR;
            }
            cv::cvtColor(raw, rgb, code);
        } else if (dng.samples_per_pixel == 3) {
            // Already RGB (Linear DNG?)
             if (dng.bits_per_sample > 8) {
                rgb = cv::Mat(height, width, CV_16UC3, (void*)dng.data.data()).clone();
            } else {
                rgb = cv::Mat(height, width, CV_8UC3, (void*)dng.data.data()).clone();
            }
            // Subtract black level again? We did it on 'raw' but if SPP=3 we just re-created 'rgb'?
            // Wait, logic above in original was re-creating 'raw'.
            // Let's stick to using 'raw' which we already subtracted from.
            // But 'raw' was initialized as CV_16UC1 or CV_8UC1 in lines 62-72.
            // If SPP=3, 'raw' (C1) contains all data but treated as 1 channel?
            // Yes, CV_16UC1 with width*3? No, width is width.
            // If SPP=3, tinydng data is 3*width*height.
            // If we created CV_16UC1 with width, height, we missed 2/3 of data?
            // tinydng DNGImage: width, height are image dimensions.
            // data size should be width*height*spp*bps/8.
            // My previous code: raw = cv::Mat(height, width, CV_16UC1, ...).
            // This only views the first plane if SPP=3? Or crashes?
            
            // Fix for SPP=3:
            // We need to reshape or recreate 'raw' correctly BEFORE subtraction if we want to support SPP=3.
            // But let's assume SPP=1 (CFA) for the task at hand.
            // If SPP=3, we fix 'rgb' creation here.
            
            if (dng.planar_configuration == 2) {
                 throw std::runtime_error("Planar RGB DNGs not yet implemented");
            }
             
            if (dng.bits_per_sample > 8) {
                rgb = cv::Mat(height, width, CV_16UC3, (void*)dng.data.data()).clone();
            } else {
                rgb = cv::Mat(height, width, CV_8UC3, (void*)dng.data.data()).clone();
            }
             cv::subtract(rgb, cv::Scalar(dng.black_level[0], dng.black_level[0], dng.black_level[0]), rgb);
             
             // Convert to BGR for OpenCV
             cv::cvtColor(rgb, rgb, cv::COLOR_RGB2BGR);
        } else {
             throw std::runtime_error("Unsupported samples per pixel: " + std::to_string(dng.samples_per_pixel));
        }

        // Linearize (Scale by White)
        cv::Mat floatRgb;
        rgb.convertTo(floatRgb, CV_32F);
        
        float black = static_cast<float>(dng.black_level[0]);
        float white = static_cast<float>(dng.white_level[0]);
        float range = white - black;
        if (range < 1e-6f) range = 1.0f; // Avoid div by zero

        // We already subtracted black from integer, so we just divide by range.
        floatRgb = floatRgb / range;
        
        // Clip to [0,1]
        cv::threshold(floatRgb, floatRgb, 0.0, 0.0, cv::THRESH_TOZERO);
        cv::threshold(floatRgb, floatRgb, 1.0, 1.0, cv::THRESH_TRUNC);

        // Convert to RGB (OpenCV default is BGR)
        cv::cvtColor(floatRgb, floatRgb, cv::COLOR_BGR2RGB);

        return floatRgb;
    }

    void saveImage(const std::string& path,
                   const cv::Mat& image,
                   int bitDepth)
    {
        CV_Assert(image.type() == CV_32FC3 || image.type() == CV_32FC1);

        cv::Mat clamped;
        cv::min(image, 1.0, clamped);
        cv::max(clamped, 0.0, clamped);

        cv::Mat out;
        if (bitDepth == 8)
        {
            clamped.convertTo(out, CV_8U, 255.0);
        }
        else
        {
            clamped.convertTo(out, CV_16U, 65535.0);
        }

        if (!cv::imwrite(path, out))
        {
            throw std::runtime_error("Failed to write image: " + path);
        }
    }
} // namespace css::io

