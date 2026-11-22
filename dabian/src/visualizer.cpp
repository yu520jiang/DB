// ===========================================================================
// visualizer.cpp
// ---------------------------------------------------------------------------
#include "visualizer.hpp"

#include <algorithm>
#include <cstdio>
#include <string>
#include <opencv2/imgproc.hpp>

namespace rescue_vision {

void Visualizer::draw(cv::Mat& bgr,
                      const std::vector<Detection>& detections,
                      const DrawOptions& options) {
    if (bgr.empty()) return;

    for (size_t i = 0; i < detections.size(); ++i) {
        const Detection& d = detections[i];
        const cv::Scalar color = colorOf(d.target_class);

        if (options.draw_bbox) {
            cv::rectangle(bgr, d.bbox, color, options.thickness);
        }
        if (options.draw_center) {
            cv::circle(bgr, d.center, 4, color, cv::FILLED, cv::LINE_AA);
        }
        if (!options.draw_text) continue;

        char text[192];
        if (options.draw_distance && d.distance_m > 0.f) {
            std::snprintf(text, sizeof(text), "%s %.2f %.2fm",
                          toString(d.target_class), d.confidence, d.distance_m);
        } else {
            std::snprintf(text, sizeof(text), "%s %.2f",
                          toString(d.target_class), d.confidence);
        }

        int baseline = 0;
        const cv::Size ts = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX,
                                            options.font_scale, 1, &baseline);

        int y = d.bbox.y - 6;
        if (y - ts.height < 0) y = d.bbox.y + ts.height + 6;

        int x = d.bbox.x;
        if (x + ts.width + 6 > bgr.cols) x = std::max(0, bgr.cols - ts.width - 6);

        const cv::Rect bg(x, y - ts.height - 4,
                          std::min(ts.width + 6, bgr.cols - x),
                          ts.height + baseline + 6);
        cv::rectangle(bgr, bg, color, cv::FILLED);
        cv::putText(bgr, text, cv::Point(x + 3, y), cv::FONT_HERSHEY_SIMPLEX,
                    options.font_scale, cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
    }
}

cv::Mat Visualizer::makeMaskMosaic(const std::vector<cv::Mat>& masks,
                                   const cv::Size& tile_size) {
    if (masks.empty()) return cv::Mat();

    const int rows = 2;
    const int cols = 2;
    cv::Mat mosaic(tile_size.height * rows, tile_size.width * cols,
                   CV_8UC3, cv::Scalar(25, 25, 25));

    const char* names[4] = {"GREEN", "LIGHT_BLUE", "ORANGE", "BLACK"};
    const cv::Scalar colors[4] = {
        cv::Scalar(0, 200, 0),
        cv::Scalar(255, 170, 60),
        cv::Scalar(0, 140, 255),
        cv::Scalar(200, 200, 200)};

    for (size_t i = 0; i < masks.size() && i < 4; ++i) {
        cv::Mat m;
        if (masks[i].empty()) {
            m = cv::Mat(tile_size, CV_8U, cv::Scalar(0));
        } else {
            cv::resize(masks[i], m, tile_size);
        }

        cv::Mat tile(tile_size, CV_8UC3, cv::Scalar(15, 15, 15));
        tile.setTo(colors[i], m);

        const int r = static_cast<int>(i) / cols;
        const int c = static_cast<int>(i) % cols;
        const cv::Rect roi(c * tile_size.width, r * tile_size.height,
                           tile_size.width, tile_size.height);
        tile.copyTo(mosaic(roi));

        cv::putText(mosaic, names[i], cv::Point(roi.x + 8, roi.y + 22),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, colors[i], 1, cv::LINE_AA);
    }
    return mosaic;
}

} // namespace rescue_vision
