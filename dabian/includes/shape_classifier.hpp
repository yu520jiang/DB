// ===========================================================================
// shape_classifier.hpp
// 轮廓几何分析 -> 形状归类（正方体 / 长方体 / 三棱锥）
//
// 思路参考（OpenCV 官方 samples + GitHub 实现）：
//   - OpenCV samples/cpp/squares.cpp     按 approxPolyDP 顶点数判多边形
//   - Naveen-Nair/Object-Detection-OpenCV  C++ 轮廓检测 + 包围盒
//
// 关键点：物块是 3D 的，斜视时正方体轮廓可能是 5~6 边形，
//         所以不能只看顶点数==4，还要看：
//           1) 面积/包围盒面积（bbox_fill）：三角形≈0.5，矩形≈1.0
//           2) 长宽比（aspect_ratio）区分正方体 / 长方体
// ---------------------------------------------------------------------------
#pragma once

#include <vector>
#include <opencv2/core.hpp>

#include "target_types.hpp"

namespace rescue_vision {

struct ShapeParams {
    double approx_eps_ratio   = 0.02;  // approxPolyDP 精度 = 该值 × 轮廓周长
    int    min_area           = 600;   // 最小轮廓面积
    int    min_vertex         = 3;     // 有效多边形最少顶点
    int    max_vertex         = 8;     // 超过该值视为异形（OTHER）
    float  cube_max_aspect    = 1.35f; // 长宽比 <= 该值 -> 正方体
    float  triangle_fill_max  = 0.62f; // bbox_fill < 该值 -> 三棱锥
    float  min_solidity       = 0.72f; // 面积/凸包面积，过滤破碎轮廓
};

struct ShapeInfo {
    bool       valid        = false;
    ShapeClass shape        = ShapeClass::UNKNOWN;
    int        vertex_count = 0;
    float      aspect_ratio = 0.f;
    float      bbox_fill    = 0.f;
    float      solidity     = 0.f;
    cv::Rect   bbox;
};

class ShapeClassifier {
public:
    explicit ShapeClassifier(const ShapeParams& params = ShapeParams());

    void setParams(const ShapeParams& p) { params_ = p; }
    const ShapeParams& params() const { return params_; }

    /// 对单个轮廓做几何分析 + 形状归类
    ShapeInfo classify(const std::vector<cv::Point>& contour) const;

private:
    ShapeParams params_;
};

} // namespace rescue_vision
