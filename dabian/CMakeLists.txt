cmake_minimum_required(VERSION 3.10)
project(rescue_vision LANGUAGES CXX)

# ---------------------------------------------------------------------------
# 2027 工创赛 · 智能救援现场初赛 —— 救援物块识别（OpenCV / C++ / Ubuntu）
# 构建：  mkdir -p build && cd build && cmake .. && make -j$(nproc)
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(OpenCV REQUIRED)
message(STATUS "Found OpenCV ${OpenCV_VERSION} : ${OpenCV_DIR}")

# ----- 算法核心库 -----------------------------------------------------------
file(GLOB RV_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(rescue_vision STATIC ${RV_SOURCES})
target_include_directories(rescue_vision PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/includes
  ${OpenCV_INCLUDE_DIRS})
target_link_libraries(rescue_vision PUBLIC ${OpenCV_LIBS})

# ----- 命令行工具 -----------------------------------------------------------
set(RV_TOOLS detect_image detect_video tune_hsv make_config)
foreach(tool ${RV_TOOLS})
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tools/${tool}.cpp)
    add_executable(${tool} ${CMAKE_CURRENT_SOURCE_DIR}/tools/${tool}.cpp)
    target_link_libraries(${tool} PRIVATE rescue_vision)
    target_include_directories(${tool} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/includes)
  endif()
endforeach()

# ----- 把 config/ 复制到 build/，保证相对路径可用 ---------------------------
add_custom_target(copy_config ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/config
          ${CMAKE_CURRENT_BINARY_DIR}/config
  COMMENT "Copy config/ to build directory")
