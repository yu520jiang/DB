// ===========================================================================
// tools/detect_video.cpp
// 摄像头 / 视频文件实时检测。
//
//   ./detect_video 0                     # 摄像头 0
//   ./detect_video test.mp4              # 视频文件
//   ./detect_video 0 config/vision_params.yaml
//
// 快捷键： q 退出  s 保存当前帧  m 切换掩码视图
// ---------------------------------------------------------------------------
#include <cstdio>
#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>

#include "param_loader.hpp"
#include "vision_pipeline.hpp"
#include "visualizer.hpp"

int main(int argc, char** argv) {
    const std::string source = (argc > 1) ? argv[1] : "0";
    const std::string config_path =
        (argc > 2) ? argv[2] : "config/vision_params.yaml";

    cv::VideoCapture cap;
    const bool is_index =
        !source.empty() &&
        source.find_first_not_of("0123456789") == std::string::npos;
    if (is_index) {
        cap.open(std::stoi(source));
    } else {
        cap.open(source);
    }
    if (!cap.isOpened()) {
        std::cerr << "无法打开视频源: " << source << "\n";
        return 1;
    }

    const rescue_vision::VisionConfig cfg =
        rescue_vision::ParamLoader::load(config_path);

    rescue_vision::VisionPipeline pipeline;
    pipeline.setDetectorConfig(cfg.detector);
    pipeline.setDepthParams(cfg.depth);

    std::cout << "已打开: " << source
              << "\n快捷键: q 退出 | s 保存当前帧 | m 切换掩码视图\n";

    bool show_mask = false;
    cv::Mat frame;
    while (cap.read(frame) && !frame.empty()) {
        const rescue_vision::PipelineResult result = pipeline.process(frame);

        cv::Mat canvas;
        if (show_mask) {
            canvas = pipeline.maskMosaic(frame);
            if (canvas.empty()) canvas = frame.clone();
        } else {
            canvas = frame.clone();
            rescue_vision::Visualizer::draw(canvas, result.detections);
        }

        char status[128];
        std::snprintf(status, sizeof(status), "targets=%zu  %.2f ms",
                      result.detections.size(), result.elapsed_ms);
        cv::putText(canvas, status, cv::Point(10, 26),
                    cv::FONT_HERSHEY_SIMPLEX, 0.65,
                    cv::Scalar(0, 255, 255), 1, cv::LINE_AA);

        cv::imshow("rescue_vision", canvas);
        const int key = cv::waitKey(1);
        if (key == 'q' || key == 27) break;
        if (key == 's') {
            cv::imwrite("snapshot.jpg", canvas);
            std::cout << "已保存 snapshot.jpg\n";
        }
        if (key == 'm') {
            show_mask = !show_mask;
        }
    }
    return 0;
}
