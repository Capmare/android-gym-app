cmake_minimum_required(VERSION 3.22)
project(gymlog LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Shared code: core logic and UI. Same sources as the host tests.
set(REPO ${CMAKE_CURRENT_LIST_DIR}/../../../../..)
add_subdirectory(${REPO}/src ${CMAKE_BINARY_DIR}/gymcore)

# The NDK supplies the glue that connects NativeActivity to android_main().
set(GLUE ${ANDROID_NDK}/sources/android/native_app_glue)
add_library(gymlog SHARED ${GLUE}/android_native_app_glue.c ${REPO}/src/android/main.cpp)
target_include_directories(gymlog PRIVATE ${GLUE})
target_link_libraries(gymlog PRIVATE gymcore android log)
target_compile_options(gymlog PRIVATE -Wall -Wextra)
# Keep ANativeActivity_onCreate: the glue defines it and NativeActivity looks it up.
# 16 KB page alignment is required by newer Android versions.
target_link_options(gymlog PRIVATE -u ANativeActivity_onCreate -Wl,-z,max-page-size=16384)
