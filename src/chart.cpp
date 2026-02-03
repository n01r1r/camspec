#include "css/chart.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

namespace css::chart
{
    namespace
    {
        cv::Mat homographyFromCorners(const ChartConfig& cfg)
        {
            std::vector<cv::Point2f> src{
                {0.0f, 0.0f},
                {1.0f, 0.0f},
                {1.0f, 1.0f},
                {0.0f, 1.0f}};

            std::vector<cv::Point2f> dst{
                cfg.topLeft,
                cfg.topRight,
                cfg.bottomRight,
                cfg.bottomLeft};

            return cv::getPerspectiveTransform(src, dst);
        }

        std::vector<cv::Point2f> patchQuadCanonical(int row, int col,
                                                    int rows, int cols,
                                                    float innerFraction)
        {
            const float patchWidth = 1.0f / static_cast<float>(cols);
            const float patchHeight = 1.0f / static_cast<float>(rows);

            const float cx = (static_cast<float>(col) + 0.5f) * patchWidth;
            const float cy = (static_cast<float>(row) + 0.5f) * patchHeight;

            const float wInner = patchWidth * innerFraction;
            const float hInner = patchHeight * innerFraction;

            const float x0 = cx - 0.5f * wInner;
            const float x1 = cx + 0.5f * wInner;
            const float y0 = cy - 0.5f * hInner;
            const float y1 = cy + 0.5f * hInner;

            return {
                {x0, y0},
                {x1, y0},
                {x1, y1},
                {x0, y1},
            };
        }
    } // namespace

    ChartConfig pickCornersInteractively(const cv::Mat& image)
    {
        if (image.empty())
        {
            throw std::runtime_error("pickCornersInteractively: empty image");
        }

        // Convert to displayable format (8-bit, normalized)
        cv::Mat display;
        if (image.type() == CV_32FC3 || image.type() == CV_32FC1)
        {
            cv::Mat normalized;
            cv::normalize(image, normalized, 0, 255, cv::NORM_MINMAX, CV_8U);
            if (normalized.channels() == 1)
            {
                cv::cvtColor(normalized, display, cv::COLOR_GRAY2BGR);
            }
            else
            {
                display = normalized;
            }
        }
        else
        {
            display = image.clone();
            if (display.channels() == 1)
            {
                cv::cvtColor(display, display, cv::COLOR_GRAY2BGR);
            }
        }

        // Create window and set up mouse callback
        const std::string winName = "Select Corners";
        cv::namedWindow(winName, cv::WINDOW_NORMAL);

        // Resize window to reasonable size while maintaining aspect ratio
        float aspect = static_cast<float>(display.cols) / static_cast<float>(display.rows);
        int winW = 800;
        cv::resizeWindow(winName, winW, static_cast<int>(winW / aspect));

        // Data structure to hold points and image
        struct PickerData
        {
            std::vector<cv::Point2f> pts;
            cv::Mat img;
            std::string windowName;
        } data;
        data.img = display.clone();
        data.windowName = winName;

        // Mouse callback: capture left clicks and draw markers
        cv::setMouseCallback(winName, [](int event, int x, int y, int /*flags*/, void* userdata) {
            PickerData* d = static_cast<PickerData*>(userdata);
            const std::string labels[] = {"1", "6", "19", "24"};
            const std::string descriptions[] = {"Dark Skin (top-left)", "Bluish Green (top-right)",
                                               "White (bottom-left)", "Black (bottom-right)"};

            if (event == cv::EVENT_LBUTTONDOWN && d->pts.size() < 4)
            {
                d->pts.emplace_back(static_cast<float>(x), static_cast<float>(y));

                // Draw circle and label
                cv::circle(d->img, cv::Point(x, y), 15, cv::Scalar(0, 255, 0), -1);
                cv::circle(d->img, cv::Point(x, y), 15, cv::Scalar(0, 0, 0), 2);
                cv::putText(d->img, labels[d->pts.size() - 1],
                           cv::Point(x + 20, y),
                           cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(0, 255, 0), 2);

                std::cout << "Selected corner " << d->pts.size() << ": "
                          << descriptions[d->pts.size() - 1]
                          << " at (" << x << ", " << y << ")" << std::endl;

                cv::imshow(d->windowName, d->img);
            }
        }, &data);

        std::cout << "\n=== Interactive Corner Picker ===" << std::endl;
        std::cout << "Click the 4 corners of the ColorChecker chart in this order:\n"
                  << "  1. Patch 1 (Dark Skin) - top-left corner\n"
                  << "  2. Patch 6 (Bluish Green) - top-right corner\n"
                  << "  3. Patch 19 (White) - bottom-left corner\n"
                  << "  4. Patch 24 (Black) - bottom-right corner\n"
                  << "\nPress any key after selecting all 4 corners to continue..."
                  << std::endl;

        cv::imshow(winName, data.img);
        cv::waitKey(0);
        cv::destroyWindow(winName);

        if (data.pts.size() != 4)
        {
            throw std::runtime_error("Interactive picker: Expected 4 corners, got " +
                                     std::to_string(data.pts.size()));
        }

        ChartConfig cfg;
        cfg.topLeft = data.pts[0];      // Patch 1
        cfg.topRight = data.pts[1];     // Patch 6
        cfg.bottomLeft = data.pts[2];    // Patch 19
        cfg.bottomRight = data.pts[3];   // Patch 24

        std::cout << "Corners selected successfully!" << std::endl;
        return cfg;
    }

    std::vector<PatchSample> sampleChartPatches(const cv::Mat& linearBgr,
                                                const ChartConfig& cfg)
    {
        CV_Assert(linearBgr.type() == CV_32FC3);

        const cv::Mat H = homographyFromCorners(cfg);

        std::vector<PatchSample> samples;
        samples.reserve(cfg.rows * cfg.cols);

        for (int r = 0; r < cfg.rows; ++r)
        {
            for (int c = 0; c < cfg.cols; ++c)
            {
                const int idx = r * cfg.cols + c;

                std::vector<cv::Point2f> canonicalQuad =
                    patchQuadCanonical(r, c, cfg.rows, cfg.cols, cfg.innerFraction);
                std::vector<cv::Point2f> imgQuad;
                cv::perspectiveTransform(canonicalQuad, imgQuad, H);

                cv::Rect bbox = cv::boundingRect(imgQuad);
                bbox &= cv::Rect(0, 0, linearBgr.cols, linearBgr.rows);
                if (bbox.empty())
                {
                    continue;
                }

                cv::Mat mask = cv::Mat::zeros(bbox.size(), CV_8U);
                std::vector<cv::Point> quadInt;
                quadInt.reserve(4);
                for (const auto& p : imgQuad)
                {
                    quadInt.emplace_back(
                        static_cast<int>(std::round(p.x) - bbox.x),
                        static_cast<int>(std::round(p.y) - bbox.y));
                }
                cv::fillConvexPoly(mask, quadInt, cv::Scalar(255));

                cv::Mat roi = linearBgr(bbox);

                cv::Scalar mean = cv::mean(roi, mask);

                // Approximate median by computing the median over all masked pixels per channel.
                std::vector<float> valsB;
                std::vector<float> valsG;
                std::vector<float> valsR;
                valsB.reserve(roi.total());
                valsG.reserve(roi.total());
                valsR.reserve(roi.total());

                for (int y = 0; y < roi.rows; ++y)
                {
                    const auto* ptr = roi.ptr<cv::Vec3f>(y);
                    const auto* mptr = mask.ptr<uint8_t>(y);
                    for (int x = 0; x < roi.cols; ++x)
                    {
                        if (mptr[x])
                        {
                            valsB.push_back(ptr[x][0]);
                            valsG.push_back(ptr[x][1]);
                            valsR.push_back(ptr[x][2]);
                        }
                    }
                }

                auto medianOf = [](std::vector<float>& v) -> float {
                    if (v.empty())
                        return 0.0f;
                    const size_t mid = v.size() / 2;
                    std::nth_element(v.begin(), v.begin() + mid, v.end());
                    return v[mid];
                };

                PatchSample s;
                s.index = idx;
                s.meanBgr = cv::Vec3f(
                    static_cast<float>(mean[0]),
                    static_cast<float>(mean[1]),
                    static_cast<float>(mean[2]));

                s.medianBgr = cv::Vec3f(
                    medianOf(valsB),
                    medianOf(valsG),
                    medianOf(valsR));

                samples.push_back(s);
            }
        }

        return samples;
    }
} // namespace css::chart

