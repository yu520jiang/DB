// ===========================================================================
// depth_estimator.hpp
// RGB-D 测距：在检测框内取有效深度中值作为目标距离
//
// 前提：深度图已经“对齐到彩色图”（同分辨率、同视角）。
// 参考（GitHub / 官方示例）：
//   - realsenseai/librealsense  rs2::align(RS2_STREAM_COLOR)
//   - librealsense C++ examples  rs-align / rs-hello-realsense
// ---------------------------------------------------------------------------
#pragma once

#include <opencv2/core.hpp>

#include "target_types.hpp"

namespace rescue_vision {

struct DepthParams {
    float depth_scale = 0.001f;  // 深度原始值 -> 米（RealSense z16 为 1mm）
    int   valid_min   = 1;       // 有效深度下限（原始单位）
    int   valid_max   = 10000;   // 有效深度上限（原始单位，约 10m）
    float roi_shrink  = 0.2f;    // 只取 bbox 中心 (1-roi_shrink) 区域
};

class DepthEstimator {
public:
    explicit DepthEstimator(const DepthParams& params = DepthParams());

    void setParams(const DepthParams& p) { params_ = p; }
    const DepthParams& params() const { return params_; }

    /// 在 bbox 内统计有效深度并取中值。
    /// @param depth      已对齐到彩色的深度图（CV_16U 或 CV_32F）
    /// @param bbox       彩色图坐标系下的检测框
    /// @param valid_ratio 输出：有效深度像素占比（%）
    /// @return 目标距离（米）；无有效深度返回 -1
    float estimate(const cv::Mat& depth, const cv::Rect& bbox, int& valid_ratio) const;

private:
    DepthParams params_;
};

} // namespace rescue_vision
