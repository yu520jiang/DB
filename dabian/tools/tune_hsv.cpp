// ===========================================================================
// tools/tune_hsv.cpp
// 交互式 HSV 调参：滑动条调 4 类目标的阈值，按 p 打印可直接填入 YAML 的数值。
//
//   ./tune_hsv test.jpg            # 用图片调
//   ./tune_hsv 0                   # 用摄像头调
//
// 快捷键： q 退出 | p 打印当前阈值
// 参考：twenty-twenty/opencv_basic_color_separator（滑动条调 HSV 的思路）
// ---------------------------------------------------------------------------
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>

#include "color_segmenter.hpp"
#include "param_loader.hpp"

namespace {

const char* kWinView = "tune_hsv - view";
const char* kWinMask = "tune_hsv - mask";

struct TunerState {
    cv::Mat bgr;
    cv::Mat hsv;
    rescue_vision::ColorParams params;
    int color_index = 0;   // 0绿 1浅蓝 2橘 3黑
};

TunerState g;

rescue_vision::HSVThreshold& currentThreshold() {
    switch (g.color_index) {
        case 0: return g.params.green;
        case 1: return g.params.light_blue;
        case 2: return g.params.orange;
        default: return g.params.black;
    }
}

/// 把 6 个滑动条读回当前颜色区间
void applySliders() {
    rescue_vision::HSVThreshold& th = currentThreshold();
    const int h1 = cv::getTrackbarPos("H lo", kWinView);
    const int h2 = cv::getTrackbarPos("H hi", kWinView);
    const int s1 = cv::getTrackbarPos("S lo", kWinView);
    const int s2 = cv::getTrackbarPos("S hi", kWinView);
    const int v1 = cv::getTrackbarPos("V lo", kWinView);
    const int v2 = cv::getTrackbarPos("V hi", kWinView);

    th.lower = cv::Scalar(std::min(h1, h2), std::min(s1, s2), std::min(v1, v2));
    th.upper = cv::Scalar(std::max(h1, h2), std::max(s1, s2), std::max(v1, v2));
    th.has_second = false;
}

void onTrackbar(int, void*) { applySliders(); }

/// 切换颜色时，把该颜色已有阈值同步到滑动条
void onColorChanged(int, void*) {
    const rescue_vision::HSVThreshold& th = currentThreshold();
    cv::setTrackbarPos("H lo", kWinView, static_cast<int>(th.lower[0]));
    cv::setTrackbarPos("H hi", kWinView, static_cast<int>(th.upper[0]));
    cv::setTrackbarPos("S lo", kWinView, static_cast<int>(th.lower[1]));
    cv::setTrackbarPos("S hi", kWinView, static_cast<int>(th.upper[1]));
    cv::setTrackbarPos("V lo", kWinView, static_cast<int>(th.lower[2]));
    cv::setTrackbarPos("V hi", kWinView, static_cast<int>(th.upper[2]));
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "用法: tune_hsv <图片路径 | 摄像头索引> [config_yaml]\n";
        return 1;
    }

    const std::string source = argv[1];
    const std::string config_path =
        (argc > 2) ? argv[2] : "config/vision_params.yaml";

    const rescue_vision::VisionConfig cfg =
        rescue_vision::ParamLoader::load(config_path);
    g.params = cfg.detector.color;

    // 读入图像
    const bool is_index =
        !source.empty() &&
        source.find_first_not_of("0123456789") == std::string::npos;
    if (is_index) {
        cv::VideoCapture cap(std::stoi(source));
        if (!cap.isOpened()) {
            std::cerr << "无法打开摄像头\n";
            return 2;
        }
        cap >> g.bgr;
    } else {
        g.bgr = cv::imread(source);
    }
    if (g.bgr.empty()) {
        std::cerr << "无法读取输入: " << source << "\n";
        return 3;
    }

    rescue_vision::ColorSegmenter::toHSV(g.bgr, g.hsv);

    cv::namedWindow(kWinView, cv::WINDOW_NORMAL);
    // 注意：highgui 的 trackbar 名称用 ASCII，避免中文在 GTK/Qt 下显示为方块
    cv::createTrackbar("color 0G 1LB 2O 3B", kWinView, nullptr, 3, onColorChanged);
    cv::createTrackbar("H lo", kWinView, nullptr, 179, onTrackbar);
    cv::createTrackbar("H hi", kWinView, nullptr, 179, onTrackbar);
    cv::createTrackbar("S lo", kWinView, nullptr, 255, onTrackbar);
    cv::createTrackbar("S hi", kWinView, nullptr, 255, onTrackbar);
    cv::createTrackbar("V lo", kWinView, nullptr, 255, onTrackbar);
    cv::createTrackbar("V hi", kWinView, nullptr, 255, onTrackbar);
    onColorChanged(0, nullptr);

    while (true) {
        const rescue_vision::HSVThreshold& th = currentThreshold();

        cv::Mat mask;
        cv::inRange(g.hsv, th.lower, th.upper, mask);

        // 半透明着色叠加到原图（addWeighted 没有 mask 重载，先 setTo 再融合）
        cv::Mat overlay = g.bgr.clone();
        overlay.setTo(cv::Scalar(0, 255, 0), mask);
        cv::Mat view;
        cv::addWeighted(g.bgr, 0.60, overlay, 0.40, 0.0, view);

        char info[128];
        std::snprintf(info, sizeof(info),
                      "color=%d  H[%d,%d] S[%d,%d] V[%d,%d]",
                      g.color_index,
                      static_cast<int>(th.lower[0]), static_cast<int>(th.upper[0]),
                      static_cast<int>(th.lower[1]), static_cast<int>(th.upper[1]),
                      static_cast<int>(th.lower[2]), static_cast<int>(th.upper[2]));
        cv::putText(view, info, cv::Point(10, 26), cv::FONT_HERSHEY_SIMPLEX,
                    0.6, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);

        cv::imshow(kWinView, view);
        cv::imshow(kWinMask, mask);

        const int key = cv::waitKey(30);
        if (key == 'q' || key == 27) break;
        if (key == 'p') {
            std::cout << "\n--- 把下面数值填回 config/vision_params.yaml ---\n"
                      << "color_index = " << g.color_index
                      << "   (0=green 1=light_blue 2=orange 3=black)\n"
                      << "      lower: [ " << th.lower[0] << ", " << th.lower[1]
                      << ", " << th.lower[2] << " ]\n"
                      << "      upper: [ " << th.upper[0] << ", " << th.upper[1]
                      << ", " << th.upper[2] << " ]\n";
        }
    }
    return 0;
}
