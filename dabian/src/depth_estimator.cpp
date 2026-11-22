// ===========================================================================
// depth_estimator.cpp
// ---------------------------------------------------------------------------
#include "depth_estimator.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace rescue_vision {

DepthEstimator::DepthEstimator(const DepthParams& params) : params_(params) {}

float DepthEstimator::estimate(const cv::Mat& depth, const cv::Rect& bbox,
                               int& valid_ratio) const {
    valid_ratio = 0;
    if (depth.empty() || bbox.width <= 0 || bbox.height <= 0) return -1.f;
    if (depth.channels() != 1) return -1.f;

    // 统一到 CV_16U 或 CV_32F
    cv::Mat converted;
    const cv::Mat* src = &depth;
    if (depth.type() != CV_16U && depth.type() != CV_32F) {
        depth.convertTo(converted, CV_16U);
        src = &converted;
    }

    // 只取检测框中心区域，避开边缘/背景混合像素
    cv::Rect roi = bbox;
    roi &= cv::Rect(0, 0, depth.cols, depth.rows);
    if (roi.width <= 0 || roi.height <= 0) return -1.f;

    const int sx = static_cast<int>(roi.width  * params_.roi_shrink * 0.5f);
    const int sy = static_cast<int>(roi.height * params_.roi_shrink * 0.5f);
    roi.x += sx; roi.y += sy;
    roi.width  -= 2 * sx;
    roi.height -= 2 * sy;
    if (roi.width <= 0 || roi.height <= 0) return -1.f;

    const bool is_float = (src->type() == CV_32F);
    std::vector<float> values;
    values.reserve(static_cast<size_t>(roi.width) * static_cast<size_t>(roi.height));

    for (int y = roi.y; y < roi.y + roi.height; ++y) {
        for (int x = roi.x; x < roi.x + roi.width; ++x) {
            const float v = is_float
                                ? src->at<float>(y, x)
                                : static_cast<float>(src->at<unsigned short>(y, x));
            if (v >= params_.valid_min && v <= params_.valid_max) {
                values.push_back(v);
            }
        }
    }

    const int total = roi.width * roi.height;
    if (values.empty() || total <= 0) return -1.f;

    valid_ratio = static_cast<int>(100.0f * static_cast<float>(values.size())
                                  / static_cast<float>(total));

    // 取中值：比均值更抗噪，且不受少量飞点影响
    const size_t mid = values.size() / 2;
    std::nth_element(values.begin(),
                     values.begin() + static_cast<std::ptrdiff_t>(mid),
                     values.end());
    return values[mid] * params_.depth_scale;
}

} // namespace rescue_vision
