// ===========================================================================
// target_detector.cpp
// ---------------------------------------------------------------------------
#include "target_detector.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <opencv2/imgproc.hpp>

namespace rescue_vision {

TargetDetector::TargetDetector(const DetectorConfig& cfg) : cfg_(cfg) {}

TargetClass TargetDetector::decideTarget(ColorClass color,
                                         const ShapeInfo& s,
                                         bool allow_black_fallback) {
    const bool rect_like =
        (s.shape == ShapeClass::CUBE || s.shape == ShapeClass::CUBOID);

    switch (color) {
        case ColorClass::GREEN:
            // 普通物资 = 绿色正方体
            if (s.shape == ShapeClass::CUBE && s.aspect_ratio <= 1.45f) {
                return TargetClass::NORMAL_MATERIAL;
            }
            return TargetClass::UNKNOWN;

        case ColorClass::LIGHT_BLUE:
            // 危险目标 = 浅蓝正方体（与绿色同形，只能靠颜色区分）
            if (s.shape == ShapeClass::CUBE && s.aspect_ratio <= 1.45f) {
                return TargetClass::DANGER;
            }
            return TargetClass::UNKNOWN;

        case ColorClass::ORANGE:
            // 伤员 = 橘色长方体（80x40x40）；正/侧视是矩形，
            // 端面 40x40 会近似正方形，所以方形/矩形都接受。
            if (rect_like) return TargetClass::CASUALTY;
            return TargetClass::UNKNOWN;

        case ColorClass::BLACK:
            // 核心物资 = 黑色三棱锥；斜视时轮廓可能接近矩形，允许回退
            if (s.shape == ShapeClass::PYRAMID) return TargetClass::CORE_MATERIAL;
            if (allow_black_fallback && rect_like) return TargetClass::CORE_MATERIAL;
            return TargetClass::UNKNOWN;

        default:
            return TargetClass::UNKNOWN;
    }
}

float TargetDetector::computeConfidence(const Detection& d) {
    // 1) 面积项：600 ~ 6000 像素线性
    const float f_area = std::min(1.f, d.area / 6000.f);

    // 2) 实心度项：0.72 ~ 1.0 线性
    const float f_solid =
        std::min(1.f, std::max(0.f, (d.solidity - 0.72f) / 0.28f));

    // 3) 形状匹配项
    float f_shape = 0.5f;
    if (d.shape_class == ShapeClass::PYRAMID) {
        // 越像三角形（bbox_fill 越接近 0.5）越好
        f_shape = std::min(1.f, std::max(0.f, (0.62f - d.bbox_fill) / 0.20f + 0.60f));
    } else if (d.shape_class == ShapeClass::CUBE) {
        f_shape = (d.aspect_ratio <= 1.15f)
                      ? 1.f
                      : std::max(0.f, 1.f - (d.aspect_ratio - 1.15f) / 0.60f);
    } else if (d.shape_class == ShapeClass::CUBOID) {
        f_shape = std::min(1.f, std::max(0.f, (d.aspect_ratio - 1.20f) / 0.80f));
    }

    return std::min(1.f, 0.40f * f_area + 0.30f * f_solid + 0.30f * f_shape);
}

std::vector<Detection> TargetDetector::detect(const cv::Mat& bgr) const {
    std::vector<Detection> detections;
    if (bgr.empty()) return detections;

    cv::Mat hsv;
    ColorSegmenter::toHSV(bgr, hsv);
    if (hsv.empty()) return detections;

    const ColorSegmenter segmenter(cfg_.color);
    const ShapeClassifier shaper(cfg_.shape);

    for (int ci = 0; ci < ColorSegmenter::kColorCount; ++ci) {
        const ColorClass color = ColorSegmenter::colorAt(ci);
        const cv::Mat mask = segmenter.segment(hsv, color);

        // 注意：OpenCV 4 的 findContours 只保留带 hierarchy 的重载，
        // 所以这里统一传 hierarchy，兼容 OpenCV 3.x / 4.x。
        std::vector<std::vector<cv::Point> > contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(mask, contours, hierarchy,
                         cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        for (size_t k = 0; k < contours.size(); ++k) {
            const ShapeInfo si = shaper.classify(contours[k]);
            if (!si.valid) continue;

            const TargetClass tc = decideTarget(color, si, cfg_.allow_black_fallback);
            if (tc == TargetClass::UNKNOWN) continue;
            if (cfg_.require_shape_match && si.shape == ShapeClass::OTHER) continue;

            Detection d;
            d.target_class = tc;
            d.color_class  = color;
            d.shape_class  = si.shape;
            d.bbox         = si.bbox;
            d.vertex_count = si.vertex_count;
            d.aspect_ratio = si.aspect_ratio;
            d.bbox_fill    = si.bbox_fill;
            d.solidity     = si.solidity;
            d.area         = static_cast<float>(cv::contourArea(contours[k]));

            // 质心（一阶矩）；退化时退回包围盒中心
            const cv::Moments m = cv::moments(contours[k]);
            if (std::fabs(m.m00) > 1e-6) {
                d.center = cv::Point2f(static_cast<float>(m.m10 / m.m00),
                                       static_cast<float>(m.m01 / m.m00));
            } else {
                d.center = cv::Point2f(si.bbox.x + si.bbox.width * 0.5f,
                                       si.bbox.y + si.bbox.height * 0.5f);
            }

            d.confidence = computeConfidence(d);
            if (d.confidence < cfg_.min_confidence) continue;

            d.label = std::string(toString(d.target_class));
            detections.push_back(d);
        }
    }

    // 置信度降序，方便上层优先处理高分目标
    std::sort(detections.begin(), detections.end(),
              [](const Detection& a, const Detection& b) {
                  return a.confidence > b.confidence;
              });
    return detections;
}

} // namespace rescue_vision
