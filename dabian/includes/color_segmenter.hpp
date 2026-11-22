// ===========================================================================
// color_segmenter.hpp
// HSV 颜色分割
//
// 思路参考（GitHub 开源实现）：
//   - twenty-twenty/opencv_basic_color_separator  滑动条调 HSV 阈值
//   - Patny1/hsv_object_detector                  inRange -> 轮廓 -> 包围盒/质心
//   - RobotX-Workshops/dji-tello-playground       color_contour_detection.py
// ---------------------------------------------------------------------------
#pragma once
#include <vector>
#include <opencv2/core.hpp>
#include "target_types.hpp"
namespace rescue_vision {

/// 单类颜色的 HSV 区间（OpenCV：H∈[0,179], S∈[0,255], V∈[0,255]）
/// 偏红的橘色在 H 两端会断开，用 has_second + lower2/upper2 补第二段。
struct HSVThreshold {
    cv::Scalar lower{35, 80, 40};
    cv::Scalar upper{85, 255, 255};

    bool       has_second = false;
    cv::Scalar lower2{0, 0, 0};
    cv::Scalar upper2{0, 0, 0};
};

struct ColorParams {
    HSVThreshold green;       // 普通物资
    HSVThreshold light_blue;  // 危险目标
    HSVThreshold orange;      // 伤员
    HSVThreshold black;       // 核心物资

    int min_area     = 600;   // 最小连通域面积（像素）
    int morph_kernel = 5;     // 形态学核尺寸，<=0 表示不做
    int open_iter    = 1;     // 开运算次数（去噪）
    int close_iter   = 1;     // 闭运算次数（补孔）
};

/// 现场默认参数；现场光照变了就用 tools/tune_hsv 重新标定
ColorParams makeDefaultColorParams();

class ColorSegmenter {
public:
    static const int kColorCount = 4;

    explicit ColorSegmenter(const ColorParams& params = makeDefaultColorParams());

    void setParams(const ColorParams& p) { params_ = p; }
    const ColorParams& params() const { return params_; }

    /// BGR/BGRA/GRAY -> HSV（统一成 CV_8UC3 再转换）
    static void toHSV(const cv::Mat& bgr, cv::Mat& hsv);

    /// 生成某类颜色的二值掩码（含形态学去噪）
    cv::Mat segment(const cv::Mat& hsv, ColorClass color) const;

    /// 一次性生成全部掩码，顺序固定：
    ///   0=GREEN  1=LIGHT_BLUE  2=ORANGE  3=BLACK
    void segmentAll(const cv::Mat& hsv, std::vector<cv::Mat>& masks) const;

    /// 掩码下标 -> 颜色枚举
    static ColorClass colorAt(int index);

private:
    const HSVThreshold& thresholdOf(ColorClass c) const;

    ColorParams params_;
};

} // namespace rescue_vision
