// ===========================================================================
// shape_classifier.cpp
// ---------------------------------------------------------------------------
#include "shape_classifier.hpp"

#include <algorithm>
#include <opencv2/imgproc.hpp>

namespace rescue_vision {

ShapeClassifier::ShapeClassifier(const ShapeParams& params) : params_(params) {}

ShapeInfo ShapeClassifier::classify(const std::vector<cv::Point>& contour) const {
    ShapeInfo info;
    if (static_cast<int>(contour.size()) < params_.min_vertex) return info;

    const double area = cv::contourArea(contour);
    if (area < params_.min_area) return info;

    // ---- 实心度：面积 / 凸包面积，过滤破碎或凹陷的轮廓 ----
    std::vector<cv::Point> hull;
    cv::convexHull(contour, hull);
    const double hull_area = cv::contourArea(hull);
    if (hull_area < 1.0) return info;

    info.solidity = static_cast<float>(area / hull_area);
    if (info.solidity < params_.min_solidity) return info;

    // ---- 多边形逼近（OpenCV samples/cpp/squares.cpp 同款做法）----
    const double peri = cv::arcLength(contour, true);
    if (peri < 3.0) return info;

    std::vector<cv::Point> approx;
    cv::approxPolyDP(contour, approx, params_.approx_eps_ratio * peri, true);

    info.vertex_count = static_cast<int>(approx.size());
    if (info.vertex_count < params_.min_vertex ||
        info.vertex_count > params_.max_vertex) {
        return info;
    }

    info.bbox = cv::boundingRect(contour);
    if (info.bbox.width <= 0 || info.bbox.height <= 0) return info;

    const int lo = std::min(info.bbox.width, info.bbox.height);
    const int hi = std::max(info.bbox.width, info.bbox.height);
    info.aspect_ratio = static_cast<float>(hi) / static_cast<float>(lo);

    const double bbox_area =
        static_cast<double>(info.bbox.width) * static_cast<double>(info.bbox.height);
    info.bbox_fill = static_cast<float>(area / bbox_area);

    // ---- 形状判定 ----
    // 先用“填充比”抓三角形（三棱锥），再用长宽比细分正方体 / 长方体。
    // 这样做的好处：3D 物块斜视时正方体轮廓会变成 5~6 边形，
    // 但填充比仍然接近 1，不会被误判成三角形。
    if (info.vertex_count == 3 || info.bbox_fill < params_.triangle_fill_max) {
        info.shape = ShapeClass::PYRAMID;
    } else if (info.aspect_ratio <= params_.cube_max_aspect) {
        info.shape = ShapeClass::CUBE;
    } else {
        info.shape = ShapeClass::CUBOID;
    }

    info.valid = true;
    return info;
}

} // namespace rescue_vision
