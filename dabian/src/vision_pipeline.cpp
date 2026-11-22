// ===========================================================================
// vision_pipeline.cpp
// ---------------------------------------------------------------------------
#include "vision_pipeline.hpp"

#include <cstdint>

#include <opencv2/core.hpp>

#include "visualizer.hpp"

namespace rescue_vision {

VisionPipeline::VisionPipeline()
    : detector_(DetectorConfig()), depth_(DepthParams()) {}

PipelineResult VisionPipeline::process(const cv::Mat& bgr) const {
    PipelineResult result;
    const int64_t t0 = static_cast<int64_t>(cv::getTickCount());
    result.detections = detector_.detect(bgr);
    const double elapsed_ticks =
        static_cast<double>(static_cast<int64_t>(cv::getTickCount()) - t0);
    result.elapsed_ms =
        elapsed_ticks * 1000.0 / static_cast<double>(cv::getTickFrequency());
    return result;
}

PipelineResult VisionPipeline::process(const cv::Mat& bgr,
                                      const cv::Mat& aligned_depth) const {
    PipelineResult result = process(bgr);
    if (aligned_depth.empty()) return result;

    for (size_t i = 0; i < result.detections.size(); ++i) {
        Detection& d = result.detections[i];
        int valid_ratio = 0;
        d.distance_m = depth_.estimate(aligned_depth, d.bbox, valid_ratio);
        d.depth_valid_ratio = valid_ratio;
    }
    return result;
}

cv::Mat VisionPipeline::maskMosaic(const cv::Mat& bgr) const {
    if (bgr.empty()) return cv::Mat();

    cv::Mat hsv;
    ColorSegmenter::toHSV(bgr, hsv);
    if (hsv.empty()) return cv::Mat();

    const ColorSegmenter segmenter(detector_.config().color);
    std::vector<cv::Mat> masks;
    segmenter.segmentAll(hsv, masks);
    return Visualizer::makeMaskMosaic(masks);
}

} // namespace rescue_vision
