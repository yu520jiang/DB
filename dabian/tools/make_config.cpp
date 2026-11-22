// ===========================================================================
// tools/make_config.cpp
// 生成默认参数模板 config/vision_params.yaml（覆盖前请备份手调结果）
//
//   ./make_config                          # 写 config/vision_params.yaml
//   ./make_config config/my_params.yaml
// ---------------------------------------------------------------------------
#include <iostream>
#include <string>

#include "param_loader.hpp"

int main(int argc, char** argv) {
    const std::string output =
        (argc > 1) ? argv[1] : "config/vision_params.yaml";

    if (!rescue_vision::ParamLoader::saveTemplate(output)) {
        std::cerr << "写出失败: " << output
                  << "（请确认目录存在；先 mkdir -p config）\n";
        return 1;
    }
    std::cout << "默认参数模板已写出: " << output << "\n";
    return 0;
}
