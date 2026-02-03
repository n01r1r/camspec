#include "css/pipeline.hpp"

#include "css/calib.hpp"
#include "css/chart.hpp"
#include "css/profile.hpp"
#include "css/refdata.hpp"

#include <algorithm>
#include <stdexcept>

namespace css::pipeline
{
    profile::Profile calibrateFromChart(const cv::Mat& chartImage,
                                        const CalibrateConfig& cfg)
    {
        if (chartImage.empty())
        {
            throw std::runtime_error("calibrateFromChart: empty image");
        }

        CV_Assert(chartImage.type() == CV_32FC3);

        const auto samples = chart::sampleChartPatches(chartImage, cfg.chart);
        if (samples.size() < 24)
        {
            throw std::runtime_error("Expected 24 sampled patches, got " +
                                     std::to_string(samples.size()));
        }

        auto refs = refdata::loadColorChecker24Csv(cfg.refDataCsvPath,
                                                   cfg.illuminant,
                                                   "linear_srgb");

        // Map by index.
        std::vector<Eigen::Vector3f> measured;
        std::vector<Eigen::Vector3f> reference;
        measured.reserve(24);
        reference.reserve(24);

        for (const auto& ref : refs.patches)
        {
            auto it = std::find_if(samples.begin(), samples.end(),
                                   [&](const chart::PatchSample& s) {
                                       return s.index == ref.index;
                                   });
            if (it == samples.end())
                continue;

            const auto& bgr = it->meanBgr;
            measured.emplace_back(bgr[2], bgr[1], bgr[0]); // convert BGR -> RGB
            reference.push_back(ref.linearSrgb);
        }

        if (measured.size() < 6)
        {
            throw std::runtime_error("Too few matching patches for calibration");
        }

        auto calibRes = calib::solveColorMatrix(measured, reference, true, 1e-4f);

        profile::Profile prof;
        prof.cameraName = cfg.cameraName;
        prof.illuminant = cfg.illuminant;
        prof.chartType = "ColorChecker24";
        prof.targetColorSpace = "linear_srgb";
        prof.colorMatrix = calibRes.colorMatrix;
        prof.whiteBalance = calibRes.whiteBalance;

        return prof;
    }

    namespace
    {
        float srgbEncode(float v)
        {
            v = std::clamp(v, 0.0f, 1.0f);
            if (v <= 0.0031308f)
            {
                return 12.92f * v;
            }
            return 1.055f * std::pow(v, 1.0f / 2.4f) - 0.055f;
        }
    } // namespace

    cv::Mat applyProfile(const cv::Mat& linearBgr,
                         const profile::Profile& prof,
                         bool applySrgbGamma)
    {
        CV_Assert(linearBgr.type() == CV_32FC3);

        cv::Mat out(linearBgr.size(), linearBgr.type());

        const Eigen::Matrix3f& M = prof.colorMatrix;
        const Eigen::Vector3f& wb = prof.whiteBalance;

        for (int y = 0; y < linearBgr.rows; ++y)
        {
            const auto* inPtr = linearBgr.ptr<cv::Vec3f>(y);
            auto* outPtr = out.ptr<cv::Vec3f>(y);
            for (int x = 0; x < linearBgr.cols; ++x)
            {
                const cv::Vec3f& bgr = inPtr[x];
                Eigen::Vector3f camRgb(bgr[2], bgr[1], bgr[0]); // BGR -> RGB

                camRgb = camRgb.cwiseProduct(wb);
                Eigen::Vector3f tgt = M * camRgb;

                if (applySrgbGamma)
                {
                    tgt[0] = srgbEncode(tgt[0]);
                    tgt[1] = srgbEncode(tgt[1]);
                    tgt[2] = srgbEncode(tgt[2]);
                }

                // Back to BGR for OpenCV.
                outPtr[x] = cv::Vec3f(tgt[2], tgt[1], tgt[0]);
            }
        }

        return out;
    }
} // namespace css::pipeline

