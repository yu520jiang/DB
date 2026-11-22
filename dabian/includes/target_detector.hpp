// ===========================================================================
// target_detector.hpp
// 颜色 + 形状 联合判别 -> 救援目标
//
// 赛题原话：“绿色与浅蓝均为正方体：形状先排除异形，颜色仍用于区分同形目标。”
// 因此本判决器：
//   1) 形状先做门槛（异形直接丢）
//   2) 颜色决定类别（绿/浅蓝同形，靠颜色区分）
//   3) 橘色长方体、黑色三棱锥按各自形状规则放宽
// ---------------------------------------------------------------------------
#pragma once

#include <vector>
#include <opencv2/core.hpp>

#include "color_segmenter.hpp"
#include "shape_classifier.hpp"
#include "target_types.hpp"

namespace rescue_vision {

struct DetectorConfig {
    // 默认就用现场的 4 类颜色阈值，避免忘记初始化时全变成绿色区间
    ColorParams color = makeDefaultColorParams();
    ShapeParams shape;

    /// 形状不合格（OTHER）时是否直接丢弃
    bool require_shape_match = true;

    /// 置信度低于该值不输出
    float min_confidence = 0.10f;

    /// 黑色目标斜视时轮廓可能接近矩形，仍按核心物资处理
    bool allow_black_fallback = true;
};

class TargetDetector {
public:
    explicit TargetDetector(const DetectorConfig& cfg = DetectorConfig());

    void setConfig(const DetectorConfig& cfg) { cfg_ = cfg; }
    const DetectorConfig& config() const { return cfg_; }

    /// 输入 BGR 图，输出按置信度降序排列的检测结果
    std::vector<Detection> detect(const cv::Mat& bgr) const;

private:
    /// 颜色 + 形状 -> 目标类别（UNKNOWN 表示判不了）
    static TargetClass decideTarget(ColorClass color,
                                    const ShapeInfo& shape,
                                    bool allow_black_fallback);

    /// 综合置信度：面积 / 实心度 / 形状匹配度
    static float computeConfidence(const Detection& d);

    DetectorConfig cfg_;
};

} // namespace rescue_vision
