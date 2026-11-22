// ===========================================================================
// tools/detect_image.cpp
// 对单张图片跑一遍检测，打印结果并保存标注图。
//
//   ./detect_image test.jpg out.jpg config/vision_params.yaml
// ---------------------------------------------------------------------------
#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>

#include "param_loader.hpp"
#include "vision_pipeline.hpp"
#include "visualizer.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "用法: detect_image <image_path> [output_path] [config_yaml]\n"
                  << "示例: ./detect_image test.jpg result.jpg config/vision_params.yaml\n";
        return 1;
    }

    const std::string image_path = argv[1];
    const std::string output_path = (argc > 2) ? argv[2] : "detect_result.jpg";
    const std::string config_path =
        (argc > 3) ? argv[3] : "config/vision_params.yaml";

    cv::Mat image = cv::imread(image_path);
    if (image.empty()) {
        std::cerr << "无法读取图像: " << image_path << "\n";
        return 2;
    }

    const rescue_vision::VisionConfig cfg =
        rescue_vision::ParamLoader::load(config_path);

    rescue_vision::VisionPipeline pipeline;
    pipeline.setDetectorConfig(cfg.detector);
    pipeline.setDepthParams(cfg.depth);

    const rescue_vision::PipelineResult result = pipeline.process(image);

    rescue_vision::Visualizer::draw(image, result.detections);

    std::cout << "检测到 " << result.detections.size() << " 个目标，耗时 "
              << result.elapsed_ms << " ms\n";
    for (size_t i = 0; i < result.detections.size(); ++i) {
        const rescue_vision::Detection& d = result.detections[i];
        std::cout << "  [" << i << "] "
                  << rescue_vision::toChineseName(d.target_class)
                  << " | color=" << rescue_vision::toString(d.color_class)
                  << " shape=" << rescue_vision::toString(d.shape_class)
                  << " center=(" << d.center.x << "," << d.center.y << ")"
                  << " bbox=" << d.bbox
                  << " area=" << d.area
                  << " verts=" << d.vertex_count
                  << " aspect=" << d.aspect_ratio
                  << " conf=" << d.confidence << "\n";
    }

    if (!cv::imwrite(output_path, image)) {
        std::cerr << "标注图保存失败: " << output_path << "\n";
        return 3;
    }
    std::cout << "标注图已保存: " << output_path << "\n";
    return 0;
}
