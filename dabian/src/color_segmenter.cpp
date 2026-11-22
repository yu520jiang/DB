#include "color_segmenter.hpp"
#include <opencv2/imgproc.hpp>

namespace rescue_vision {

ColorParams makeDefaultColorParams() {
    ColorParams p;

    // 绿色正方体（普通物资）
    p.green.lower = cv::Scalar(35, 80, 40);
    p.green.upper = cv::Scalar(85, 255, 255);

    // 浅蓝正方体（危险目标）—— H 与绿色分开，避免“同形”混淆
    p.light_blue.lower = cv::Scalar(88, 60, 90);
    p.light_blue.upper = cv::Scalar(118, 255, 255);

    // 橘色长方体（伤员）—— 橘色偏红，H 两端各补一段
    p.orange.lower      = cv::Scalar(0, 110, 90);
    p.orange.upper      = cv::Scalar(22, 255, 255);
    p.orange.has_second = true;
    p.orange.lower2     = cv::Scalar(168, 110, 90);
    p.orange.upper2     = cv::Scalar(179, 255, 255);

    // 黑色三棱锥（核心物资）—— 黑色饱和度低，直接用低 V 判定
    p.black.lower = cv::Scalar(0, 0, 0);
    p.black.upper = cv::Scalar(179, 255, 65);

    p.min_area     = 600;
    p.morph_kernel = 5;
    p.open_iter    = 1;
    p.close_iter   = 1;
    return p;
}

ColorSegmenter::ColorSegmenter(const ColorParams& params) : params_(params) {}

ColorClass ColorSegmenter::colorAt(int index) {
    switch (index) {
        case 0: return ColorClass::GREEN;
        case 1: return ColorClass::LIGHT_BLUE;
        case 2: return ColorClass::ORANGE;
        case 3: return ColorClass::BLACK;
        default: return ColorClass::UNKNOWN;
    }
}

const HSVThreshold& ColorSegmenter::thresholdOf(ColorClass c) const {
    switch (c) {
        case ColorClass::GREEN:      return params_.green;
        case ColorClass::LIGHT_BLUE: return params_.light_blue;
        case ColorClass::ORANGE:     return params_.orange;
        case ColorClass::BLACK:      return params_.black;
        default:                     return params_.green;
    }
}

void ColorSegmenter::toHSV(const cv::Mat& bgr, cv::Mat& hsv) {
    hsv.release();
    if (bgr.empty()) return;

    cv::Mat bgr8;
    if (bgr.channels() == 3) {
        bgr8 = bgr;
    } else if (bgr.channels() == 4) {
        cv::cvtColor(bgr, bgr8, cv::COLOR_BGRA2BGR);
    } else if (bgr.channels() == 1) {
        cv::cvtColor(bgr, bgr8, cv::COLOR_GRAY2BGR);
    } else {
        return;
    }
    cv::cvtColor(bgr8, hsv, cv::COLOR_BGR2HSV);
}

cv::Mat ColorSegmenter::segment(const cv::Mat& hsv, ColorClass color) const {
    cv::Mat mask;
    if (hsv.empty()) return mask;

    const HSVThreshold& th = thresholdOf(color);
    cv::inRange(hsv, th.lower, th.upper, mask);

    // 红色在 H 两端断开时的第二段掩码
    if (th.has_second) {
        cv::Mat mask2;
        cv::inRange(hsv, th.lower2, th.upper2, mask2);
        cv::bitwise_or(mask, mask2, mask);
    }

    // 形态学去噪：开运算去小点，闭运算补孔
    if (params_.morph_kernel > 0) {
        const cv::Mat kernel = cv::getStructuringElement(
            cv::MORPH_ELLIPSE, cv::Size(params_.morph_kernel, params_.morph_kernel));
        if (params_.open_iter > 0) {
            cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel,
                             cv::Point(-1, -1), params_.open_iter);
        }
        if (params_.close_iter > 0) {
            cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel,
                             cv::Point(-1, -1), params_.close_iter);
        }
    }
    return mask;
}

void ColorSegmenter::segmentAll(const cv::Mat& hsv,
                                std::vector<cv::Mat>& masks) const {
    masks.clear();
    masks.reserve(kColorCount);
    for (int i = 0; i < kColorCount; ++i) {
        masks.push_back(segment(hsv, colorAt(i)));
    }
}

} // namespace rescue_vision
