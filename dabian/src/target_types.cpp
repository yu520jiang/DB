// ===========================================================================
// target_types.cpp
// ---------------------------------------------------------------------------
#include "target_types.hpp"

namespace rescue_vision {

const char* toString(TargetClass c) {
    switch (c) {
        case TargetClass::NORMAL_MATERIAL: return "NORMAL";
        case TargetClass::CORE_MATERIAL:   return "CORE";
        case TargetClass::CASUALTY:        return "CASUALTY";
        case TargetClass::DANGER:          return "DANGER";
        default:                           return "UNKNOWN";
    }
}

const char* toChineseName(TargetClass c) {
    switch (c) {
        case TargetClass::NORMAL_MATERIAL: return "普通物资(绿色正方体)";
        case TargetClass::CORE_MATERIAL:   return "核心物资(黑色三棱锥)";
        case TargetClass::CASUALTY:        return "伤员(橘色长方体)";
        case TargetClass::DANGER:          return "危险目标(浅蓝正方体·只识别/绕行)";
        default:                           return "未知";
    }
}

const char* toString(ColorClass c) {
    switch (c) {
        case ColorClass::GREEN:      return "GREEN";
        case ColorClass::LIGHT_BLUE: return "LIGHT_BLUE";
        case ColorClass::ORANGE:     return "ORANGE";
        case ColorClass::BLACK:      return "BLACK";
        default:                     return "UNKNOWN";
    }
}

const char* toString(ShapeClass c) {
    switch (c) {
        case ShapeClass::CUBE:    return "CUBE";
        case ShapeClass::CUBOID:  return "CUBOID";
        case ShapeClass::PYRAMID: return "PYRAMID";
        default:                  return "OTHER";
    }
}

cv::Scalar colorOf(TargetClass c) {
    switch (c) {
        case TargetClass::NORMAL_MATERIAL: return cv::Scalar(0, 200, 0);      // 绿
        case TargetClass::CORE_MATERIAL:   return cv::Scalar(80, 80, 80);     // 深灰（近黑）
        case TargetClass::CASUALTY:        return cv::Scalar(0, 140, 255);    // 橘
        case TargetClass::DANGER:          return cv::Scalar(255, 170, 60);   // 蓝
        default:                           return cv::Scalar(200, 200, 200);
    }
}

} // namespace rescue_vision
