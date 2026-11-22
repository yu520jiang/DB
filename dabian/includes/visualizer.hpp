// ===========================================================================
// visualizer.hpp
// 检测结果可视化：包围盒 / 质心 / 类别标签 / 距离 / 掩码拼图
// ---------------------------------------------------------------------------
#pragma once

#include <vector>
#include <opencv2/core.hpp>

#include "target_types.hpp"

namespace rescue_vision {

struct DrawOptions {
    bool draw_bbox     = true;
    bool draw_center   = true;
    bool draw_text     = true;
    bool draw_distance = true;
    double font_scale  = 0.55;
    int    thickness   = 2;
};

class Visualizer {
public:
    /// 在 bgr 上就地绘制全部检测结果
    static void draw(cv::Mat& bgr,
                     const std::vector<Detection>& detections,
                     const DrawOptions& options = DrawOptions());

    /// 4 类颜色掩码拼成 2x2 调试图
    static cv::Mat makeMaskMosaic(const std::vector<cv::Mat>& masks,
                                  const cv::Size& tile_size = cv::Size(320, 240));
};

} // namespace rescue_vision
