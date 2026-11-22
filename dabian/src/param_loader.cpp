// ===========================================================================
// param_loader.cpp
// ---------------------------------------------------------------------------
#include "param_loader.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <opencv2/core.hpp>

#include "color_segmenter.hpp"

namespace rescue_vision {

namespace {

// ---- 读取小工具 -----------------------------------------------------------
// YAML 里写的是 [ h, s, v ] 序列。用官方支持的 "FileNode >> Mat" 读，
// 比依赖 FileNode >> std::vector<double> 更稳（不同 OpenCV 版本支持不一致）。
bool readScalar3(const cv::FileNode& node, cv::Scalar& out) {
    if (node.empty() || !node.isSeq()) return false;

    cv::Mat m;
    node >> m;
    if (m.empty() || m.total() < 3) return false;

    if (m.type() != CV_64F) {
        m.convertTo(m, CV_64F);
    }
    const double* p = m.ptr<double>();
    if (p == nullptr) return false;
    out = cv::Scalar(p[0], p[1], p[2]);
    return true;
}

void readHSV(const cv::FileNode& node, HSVThreshold& th) {
    if (node.empty()) return;
    readScalar3(node["lower"], th.lower);
    readScalar3(node["upper"], th.upper);
    if (readScalar3(node["lower2"], th.lower2) &&
        readScalar3(node["upper2"], th.upper2)) {
        th.has_second = true;
    }
}

void readInt(const cv::FileNode& node, int& out) {
    if (node.empty()) return;
    node >> out;
}

void readFloat(const cv::FileNode& node, float& out) {
    if (node.empty()) return;
    double v = 0.0;
    node >> v;
    out = static_cast<float>(v);
}

void readDouble(const cv::FileNode& node, double& out) {
    if (node.empty()) return;
    node >> out;
}

// ---- 写出小工具 -----------------------------------------------------------
void writeHSV(cv::FileStorage& fs, const std::string& name,
              const HSVThreshold& th) {
    fs << name << "{";
    fs << "lower" << "[" << th.lower[0] << th.lower[1] << th.lower[2] << "]";
    fs << "upper" << "[" << th.upper[0] << th.upper[1] << th.upper[2] << "]";
    if (th.has_second) {
        fs << "lower2" << "[" << th.lower2[0] << th.lower2[1] << th.lower2[2] << "]";
        fs << "upper2" << "[" << th.upper2[0] << th.upper2[1] << th.upper2[2] << "]";
    }
    fs << "}";
}

} // namespace

VisionConfig ParamLoader::load(const std::string& yaml_path) {
    VisionConfig cfg;
    // 先用内置默认值兜底（尤其是 4 类颜色的 HSV）
    cfg.detector.color = makeDefaultColorParams();

    cv::FileStorage fs;
    try {
        fs.open(yaml_path, cv::FileStorage::READ);
    } catch (const cv::Exception& e) {
        std::cerr << "[ParamLoader] 打开失败: " << e.what() << "\n";
        fs.release();
        return cfg;
    }
    if (!fs.isOpened()) {
        std::cerr << "[ParamLoader] 未找到 " << yaml_path
                  << "，使用内置默认参数\n";
        return cfg;
    }

    const cv::FileNode color = fs["color"];
    if (!color.empty()) {
        readHSV(color["green"],      cfg.detector.color.green);
        readHSV(color["light_blue"], cfg.detector.color.light_blue);
        readHSV(color["orange"],     cfg.detector.color.orange);
        readHSV(color["black"],      cfg.detector.color.black);
        readInt(color["min_area"],     cfg.detector.color.min_area);
        readInt(color["morph_kernel"], cfg.detector.color.morph_kernel);
        readInt(color["open_iter"],    cfg.detector.color.open_iter);
        readInt(color["close_iter"],   cfg.detector.color.close_iter);
    }

    const cv::FileNode shape = fs["shape"];
    if (!shape.empty()) {
        readDouble(shape["approx_eps_ratio"], cfg.detector.shape.approx_eps_ratio);
        readInt(shape["min_area"],            cfg.detector.shape.min_area);
        readFloat(shape["cube_max_aspect"],   cfg.detector.shape.cube_max_aspect);
        readFloat(shape["triangle_fill_max"], cfg.detector.shape.triangle_fill_max);
        readFloat(shape["min_solidity"],      cfg.detector.shape.min_solidity);
    }

    const cv::FileNode depth = fs["depth"];
    if (!depth.empty()) {
        readFloat(depth["depth_scale"], cfg.depth.depth_scale);
        readInt(depth["valid_min"],     cfg.depth.valid_min);
        readInt(depth["valid_max"],     cfg.depth.valid_max);
        readFloat(depth["roi_shrink"],  cfg.depth.roi_shrink);
    }

    fs.release();
    return cfg;
}

bool ParamLoader::saveTemplate(const std::string& yaml_path) {
    const ColorParams cp = makeDefaultColorParams();
    const ShapeParams sp;
    const DepthParams dp;

    cv::FileStorage fs;
    try {
        fs.open(yaml_path, cv::FileStorage::WRITE);
    } catch (const cv::Exception&) {
        return false;
    }
    if (!fs.isOpened()) return false;

    fs << "color" << "{";
    writeHSV(fs, "green",      cp.green);
    writeHSV(fs, "light_blue", cp.light_blue);
    writeHSV(fs, "orange",     cp.orange);
    writeHSV(fs, "black",      cp.black);
    fs << "min_area"     << cp.min_area;
    fs << "morph_kernel" << cp.morph_kernel;
    fs << "open_iter"    << cp.open_iter;
    fs << "close_iter"   << cp.close_iter;
    fs << "}";

    fs << "shape" << "{";
    fs << "approx_eps_ratio"   << sp.approx_eps_ratio;
    fs << "min_area"           << sp.min_area;
    fs << "cube_max_aspect"    << sp.cube_max_aspect;
    fs << "triangle_fill_max"  << sp.triangle_fill_max;
    fs << "min_solidity"       << sp.min_solidity;
    fs << "}";

    fs << "depth" << "{";
    fs << "depth_scale" << dp.depth_scale;
    fs << "valid_min"   << dp.valid_min;
    fs << "valid_max"   << dp.valid_max;
    fs << "roi_shrink"  << dp.roi_shrink;
    fs << "}";

    fs.release();
    return true;
}

} // namespace rescue_vision
