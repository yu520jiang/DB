// ===========================================================================
// param_loader.hpp
// 从 YAML 读取检测参数（用 OpenCV FileStorage，无需第三方库）
//
//   VisionConfig cfg = ParamLoader::load("config/vision_params.yaml");
//   ParamLoader::saveTemplate("config/vision_params.yaml"); // 写默认模板
// ---------------------------------------------------------------------------
#pragma once

#include <string>

#include "depth_estimator.hpp"
#include "target_detector.hpp"

namespace rescue_vision {

struct VisionConfig {
    DetectorConfig detector;
    DepthParams    depth;
};

class ParamLoader {
public:
    /// 读取 YAML；文件不存在或解析失败时退回内置默认参数
    static VisionConfig load(const std::string& yaml_path);

    /// 把默认参数写成 YAML 模板（需要目录已存在）
    static bool saveTemplate(const std::string& yaml_path);
};

} // namespace rescue_vision
