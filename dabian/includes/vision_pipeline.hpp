// ===========================================================================
// vision_pipeline.hpp
// 视觉流水线：一次调用完成 “分割 -> 形状 -> 类别 -> （可选）测距”
//
//   VisionPipeline pipeline;
//   auto r = pipeline.process(bgr);              // 仅彩色
//   auto r2 = pipeline.process(bgr, aligned_depth); // 彩色 + 对齐深度
// ---------------------------------------------------------------------------
#pragma once

#include <vector>
#include <opencv2/core.hpp>

#include "depth_estimator.hpp"
#include "target_detector.hpp"
#include "target_types.hpp"

namespace rescue_vision {

struct PipelineResult {
    std::vector<Detection> detections;
    double elapsed_ms = 0.0;   // 本次检测耗时（毫秒）
};

class VisionPipeline {
public:
    VisionPipeline();

    void setDetectorConfig(const DetectorConfig& cfg) { detector_.setConfig(cfg); }
    void setDepthParams(const DepthParams& p) { depth_.setParams(p); }

    const DetectorConfig& detectorConfig() const { return detector_.config(); }

    /// 仅彩色图
    PipelineResult process(const cv::Mat& bgr) const;

    /// 彩色图 + 已对齐深度图（同一分辨率）
    PipelineResult process(const cv::Mat& bgr, const cv::Mat& aligned_depth) const;

    /// 生成 4 类颜色掩码调试图（调参时用）
    cv::Mat maskMosaic(const cv::Mat& bgr) const;

private:
    TargetDetector detector_;
    DepthEstimator depth_;
};

} // namespace rescue_vision
