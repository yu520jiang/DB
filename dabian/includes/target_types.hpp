// ===========================================================================
// target_types.hpp
// 救援目标类型 / 检测结果 数据结构
//
// 赛题（2027 工创赛 智能+工程创新赛道 · 智能救援现场初赛）：
//   颜色   形状     类别        处理方式
//   绿色   正方体   普通物资    可转运（首批门控必须是它）
//   黑色   三棱锥   核心物资    可转运
//   橘色   长方体   伤员        单独转运，单批 1 个
//   浅蓝   正方体   危险目标    只识别 / 绕行，禁止抓取转运
// ---------------------------------------------------------------------------
#pragma once

#include <string>
#include <opencv2/core.hpp>

namespace rescue_vision {

enum class TargetClass {
    UNKNOWN = 0,
    NORMAL_MATERIAL,   // 普通物资：绿色正方体
    CORE_MATERIAL,     // 核心物资：黑色三棱锥
    CASUALTY,          // 伤员：橘色长方体
    DANGER             // 危险目标：浅蓝正方体
};

enum class ColorClass {
    UNKNOWN = 0,
    GREEN,
    LIGHT_BLUE,
    ORANGE,
    BLACK
};

enum class ShapeClass {
    UNKNOWN = 0,
    CUBE,      // 正方体：近正方形，4~6 边形（斜视时轮廓可能到 6 边形）
    CUBOID,    // 长方体：4~6 边形，长宽比明显 > 1
    PYRAMID,   // 三棱锥：三角形轮廓（面积≈包围盒一半）
    OTHER
};

struct Detection {
    TargetClass target_class = TargetClass::UNKNOWN;
    ColorClass  color_class  = ColorClass::UNKNOWN;
    ShapeClass  shape_class  = ShapeClass::UNKNOWN;

    cv::Rect    bbox;                 // 图像像素包围盒
    cv::Point2f center{0.f, 0.f};     // 轮廓质心（像素）
    float area       = 0.f;           // 轮廓像素面积
    float bbox_fill  = 0.f;           // 面积 / 包围盒面积：三角形≈0.5，矩形≈1
    float aspect_ratio = 0.f;         // 包围盒长宽比 max/min
    int   vertex_count = 0;           // approxPolyDP 顶点数
    float solidity   = 0.f;           // 轮廓面积 / 凸包面积
    float confidence = 0.f;           // 综合置信度 0~1

    // 若同时输入了“已与彩色对齐”的深度图
    float distance_m = -1.f;          // 目标距离（米），-1 表示无深度
    int   depth_valid_ratio = 0;      // 有效深度像素占比（%）

    std::string label;                // 可视化用 ASCII 标签
};

/// ASCII 短标签，用于 cv::putText 与日志
const char* toString(TargetClass c);
/// 中文名称，用于控制台输出（UTF-8 终端）
const char* toChineseName(TargetClass c);

const char* toString(ColorClass c);
const char* toString(ShapeClass c);

/// 目标类别对应的 BGR 绘制颜色
cv::Scalar colorOf(TargetClass c);

} // namespace rescue_vision
