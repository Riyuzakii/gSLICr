cmake_minimum_required(VERSION 2.8)
project(gSLICr)

IF(MSVC_IDE)
  set(OpenCV_STATIC OFF)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-DUSING_CMAKE=1)
ELSE(MSVC_IDE)
  set(CFLAGS_WARN "-Wall -Wextra -Wno-unused-parameter -Wno-strict-aliasing")
  set(CMAKE_CXX_FLAGS "-fPIC -O3 -march=native ${CFLAGS_WARN} ${CMAKE_CXX_FLAGS}")
ENDIF(MSVC_IDE)

if(APPLE)
  set(CUDA_HOST_COMPILER /usr/bin/clang)
endif(APPLE)

find_package(CUDA REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(${CUDA_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS})
add_subdirectory(ORUtils)

set(GSLICR_LIB
gSLICr_Lib/engines/gSLICr_core_engine.h
gSLICr_Lib/engines/gSLICr_seg_engine.h
gSLICr_Lib/engines/gSLICr_seg_engine_GPU.h
gSLICr_Lib/engines/gSLICr_seg_engine_shared.h
gSLICr_Lib/engines/gSLICr_core_engine.cpp
gSLICr_Lib/engines/gSLICr_seg_engine.cpp
gSLICr_Lib/engines/gSLICr_seg_engine_GPU.cu
gSLICr_Lib/objects/gSLICr_settings.h
gSLICr_Lib/objects/gSLICr_spixel_info.h
gSLICr_Lib/gSLICr_defines.h
gSLICr_Lib/gSLICr.h
)

list(APPEND "-std=c++11 -ftree-vectorize")
SOURCE_GROUP(engines FILES ${GSLICR_LIB})

cuda_add_library(gSLICr_lib
			${GSLICR_LIB}
			NVTimer.h
			OPTIONS -gencode arch=compute_30,code=compute_30)
target_link_libraries(gSLICr_lib ${CUDA_LIBRARY})

add_executable(demo demo.cpp)
target_link_libraries(demo gSLICr_lib ${OpenCV_LIBS})
add_executable(hysteresis hysteresis.cpp)
target_link_libraries(hysteresis gSLICr_lib ${OpenCV_LIBS})
add_executable(masker masker.cpp)
target_link_libraries(masker gSLICr_lib ${OpenCV_LIBS})
add_executable(rgb-gen rgb-gen.cpp)
target_link_libraries(rgb-gen gSLICr_lib ${OpenCV_LIBS})
add_executable(demo2 demo2.cpp)
target_link_libraries(demo2 gSLICr_lib ${OpenCV_LIBS})
add_executable(demo-for-imgs demo-for-imgs.cpp)
target_link_libraries(demo-for-imgs gSLICr_lib ${OpenCV_LIBS})
add_executable(splfitter splfitter.cpp)
target_link_libraries(splfitter gSLICr_lib ${OpenCV_LIBS})
add_executable(ultTrainer ultTrainer.cpp)
target_link_libraries(ultTrainer gSLICr_lib ${OpenCV_LIBS})
add_executable(explorer explorer.cpp)
target_link_libraries(explorer gSLICr_lib ${OpenCV_LIBS})
add_executable(blu-chan blu-chan.cpp)
target_link_libraries(blu-chan gSLICr_lib ${OpenCV_LIBS})
