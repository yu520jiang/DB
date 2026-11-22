# rescue_vision —— 救援物块识别（C++ / OpenCV / Ubuntu）

2027 中国大学生工程实践与创新能力大赛 · 智能+工程创新赛道
**智能救援现场初赛 —— 救援目标识别模块**

输入一张 RGB（可选 RGB-D）图像，输出 4 类救援物块的位置、类别与置信度：

| 颜色 | 形状 | 类别 | 处理方式 |
|------|------|------|----------|
| 绿色 | 正方体 40³ | **普通物资** `NORMAL` | 可转运（首批门控必须是它） |
| 黑色 | 三棱锥 40 | **核心物资** `CORE` | 可转运 |
| 橘色 | 长方体 80×40×40 | **伤员** `CASUALTY` | 单独转运、单批 1 个 |
| 浅蓝 | 正方体 40³ | **危险目标** `DANGER` | 只识别 / 绕行，禁止抓取转运 |

---

## 1. 目录结构

```
rescue_vision/
├── CMakeLists.txt              # 构建脚本（find_package(OpenCV)）
├── README.md
├── config/
│   └── vision_params.yaml      # HSV 阈值 + 形状/深度参数（可现场标定）
├── includes/                   # 全部头文件（.hpp）
│   ├── target_types.hpp        # 类别枚举 / Detection 数据结构
│   ├── color_segmenter.hpp     # HSV 颜色分割
│   ├── shape_classifier.hpp    # 轮廓几何 -> 形状归类
│   ├── target_detector.hpp     # 颜色 + 形状 联合判别
│   ├── depth_estimator.hpp     # RGB-D 测距（baox内深度中值）
│   ├── vision_pipeline.hpp     # 流水线封装
│   ├── visualizer.hpp          # 绘制结果 / 掩码拼图
│   └── param_loader.hpp        # YAML 参数加载
├── src/                        # 全部实现（.cpp）
│   ├── target_types.cpp
│   ├── color_segmenter.cpp
│   ├── shape_classifier.cpp
│   ├── target_detector.cpp
│   ├── depth_estimator.cpp
│   ├── vision_pipeline.cpp
│   ├── visualizer.cpp
│   └── param_loader.cpp
└── tools/                      # 命令行工具
    ├── detect_image.cpp        # 单张图片检测
    ├── detect_video.cpp        # 摄像头 / 视频实时检测
    ├── tune_hsv.cpp            # 滑动条交互调 HSV
    └── make_config.cpp         # 生成默认参数模板
```

---

## 2. 依赖与编译（Ubuntu）

```bash
# 依赖（Ubuntu 18.04 / 20.04 / 22.04 通用）
sudo apt update
sudo apt install -y build-essential cmake libopencv-dev

# 编译
cd rescue_vision
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

要求 **OpenCV ≥ 3.2**（Ubuntu 18.04 自带 3.2、20.04 自带 4.2、22.04 自带 4.5，均满足）。
如果确实在用 OpenCV 2.4，需要把代码里的 `cv::COLOR_*` / `cv::MORPH_*` / `cv::RETR_*`
换成 2.4 的 `CV_*` 宏。

编译产物在 `build/` 下：`librescue_vision.a` + 四个可执行文件。

---

## 3. 运行

```bash
cd build

# 1) 单张图片检测（输出标注图）
./detect_image ../test.jpg result.jpg

# 2) 摄像头实时检测（0 = 默认摄像头）
./detect_video 0

# 3) 视频文件
./detect_video ../test.mp4

# 4) 交互调参（滑动条调 HSV，按 p 打印数值）
./tune_hsv ../test.jpg

# 5) 重新生成默认参数模板
./make_config config/vision_params.yaml
```

> `detect_image` / `detect_video` 默认读 `config/vision_params.yaml`（相对路径）。
> CMake 会把 `config/` 复制到 `build/config/`，所以从 `build/` 直接运行即可。

`detect_video` 快捷键：`q` 退出、`s` 保存当前帧、`m` 切换 4 类掩码视图。

---

## 4. 算法流程

```
BGR 图像
   │
   ├─ cvtColor → HSV
   │
   ├─ 4 类颜色 inRange（含形态学开/闭运算去噪）
   │     green / light_blue / orange(两段H) / black(低V)
   │
   ├─ findContours (RETR_EXTERNAL)
   │
   ├─ 每个轮廓：approxPolyDP + 凸包 + 包围盒
   │     ├─ solidity = 面积 / 凸包面积        → 过滤破碎轮廓
   │     ├─ bbox_fill = 面积 / 包围盒面积     → 三角形≈0.5，矩形≈1.0
   │     ├─ aspect_ratio = 长边 / 短边        → 正方体 vs 长方体
   │     └─ 形状：bbox_fill<0.62 或 3 顶点 → PYRAMID；否则按长宽比 → CUBE / CUBOID
   │
   ├─ 颜色 + 形状 联合判决 → TargetClass
   │     green + CUBE        → 普通物资
   │     light_blue + CUBE   → 危险目标
   │     orange + 方形/矩形  → 伤员
   │     black + PYRAMID     → 核心物资（斜视矩形轮廓可回退）
   │
   ├─ 置信度 = 0.40·面积 + 0.30·实心度 + 0.30·形状匹配度
   │
   └─ (可选) 深度图：bbox 中心 60% 区域有效深度中值 → 距离(m)
```

**为什么形状不只看“顶点数==4”？**
物块是 3D 的，斜视时正方体轮廓会变成 5~6 边形，所以加了两个更稳的判据：
- `bbox_fill`（面积/包围盒面积）：三角形永远≈0.5，矩形≈1.0；
- `aspect_ratio`：区分正方体与 80×40×40 长方体。

---

## 5. 现场标定流程（重要）

HSV 阈值对光照很敏感，**比赛前务必现场标一次**：

```bash
cd build
./tune_hsv 0          # 或 ./tune_hsv 现场照片.jpg
```

1. 把方块分别放到车前的实际位置上；
2. 拖动 trackbar 的 `color` 依次选 0=绿 / 1=浅蓝 / 2=橘 / 3=黑；
3. 调 H/S/V 上下限，直到掩码只覆盖目标、不粘连地面；
4. 按 `p` 打印出数值，填回 `config/vision_params.yaml` 对应颜色；
5. 重新跑 `./detect_image` 验证。

**常见问题**
- 绿色和浅蓝粘连：把 `light_blue.lower[0]` 往上调（如 90+），`green.upper[0]` 往下调（如 80-）。
- 黑色把阴影也框进来：降低 `black.upper[2]`（V 上限，默认 65），并保证 `min_area` 足够大。
- 橘色漏检：橘色偏红时 H 接近 179，配置文件里 `orange` 已经带了 `lower2/upper2` 第二段。

---

## 6. 参考的开源实现（GitHub / OpenCV 官方）

本工程是自研实现，但算法思路参考了以下公开项目：

- **OpenCV 官方 `samples/cpp/squares.cpp` / `contours2.cpp`**
  用 `approxPolyDP` 顶点数判多边形——本工程 `shape_classifier.cpp` 的根基。
- **[Patny1/hsv_object_detector](https://github.com/Patny1/hsv_object_detector)**
  HSV 阈值 → 轮廓 → 包围盒 + 质心 的检测范式。
- **[twenty-twenty/opencv_basic_color_separator](https://github.com/twenty-twenty/opencv_basic_color_separator)**
  滑动条调 HSV 的交互方式（`tools/tune_hsv.cpp`）。
- **[RobotX-Workshops/dji-tello-playground](https://github.com/RobotX-Workshops/dji-tello-playground)**
  `cv2.inRange + cv2.findContours` 的颜色-轮廓检测流程。
- **[Naveen-Nair/Object-Detection-OpenCV](https://github.com/Naveen-Nair/Object-Detection-OpenCV)**
  C++ 版轮廓检测 + 包围盒计算。
- **[realsenseai/librealsense](https://github.com/realsenseai/librealsense)**
  `rs2::align(RS2_STREAM_COLOR)` 深度对齐到彩色的官方示例（接 RGB-D 时用）。

---

## 7. 可选：接入 Intel RealSense（RGB-D 测距）

本工程默认只吃彩色图；要加距离，只需要在采集层把深度对齐到彩色，再调用
`process(bgr, aligned_depth)`。示意：

```cpp
#include <librealsense2/rs.hpp>
#include <opencv2/opencv.hpp>

rs2::pipeline pipe;
rs2::config cfg;
cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);
cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_BGR8, 30);
pipe.start(cfg);

auto frames   = pipe.wait_for_frames();
auto aligned  = align_to_color.process(frames);      // 深度对齐到彩色
auto color    = aligned.get_color_frame();
auto depth    = aligned.get_depth_frame();

cv::Mat bgr(cv::Size(640, 480), CV_8UC3, (void*)color.get_data(), cv::Mat::AUTO_STEP);
cv::Mat dep(cv::Size(640, 480), CV_16U,  (void*)depth.get_data(), cv::Mat::AUTO_STEP);

auto result = pipeline.process(bgr, dep);   // 每个 Detection.distance_m 自动填好
```

> 深度图必须和彩色同分辨率且已对齐，否则 ROI 会错位。

---

## 8. 与答辩 PPT 的对应

- 第 4 页「感知—定位—决策—执行信息链」：本模块对应 **感知（环境观测）** 环节；
- 第 6 页「RGBD 识别、位姿修正与风险处理」：本模块实现其中的同步采集 / 联合判别 / 风险分支；
- 第 7 页「视觉识别代码实现」：本工程的 `color_segmenter` / `shape_classifier` / `target_detector` 即该页代码片段的产品化版本；
- 输出接口：`Detection.target_class` + `center` + `distance_m` 可接到定位/决策节点，
  危险目标（`DANGER`）由上层直接走绕行分支。

---

## 9. 已知限制

- `cv::putText` 不支持中文，画面标签用 ASCII（`NORMAL/CORE/CASUALTY/DANGER`）。
  若要在画面上显示中文，需要 `opencv_contrib` 的 `cv::freetype`，或者后期用 OpenCV 之外的库绘制。
- 单目 HSV 分割依赖光照；强反光/阴影下建议增大 `min_area`，或加多帧投票（同目标连续 N 帧同类才确认）。
- 黑色目标与深色地面容易混淆，`black.upper[2]` 是最关键的参数。
