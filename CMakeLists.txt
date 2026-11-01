cmake_minimum_required(VERSION 3.22)
project(gym_app LANGUAGES CXX)

# Host build: runs the tests and draws preview images.
# The Android build is in android/ and uses src/ directly.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src)

add_executable(gym_tests tests/tests.cpp)
target_link_libraries(gym_tests PRIVATE gymcore)
target_compile_definitions(gym_tests PRIVATE GYM_PHOTOS="${CMAKE_SOURCE_DIR}/android/app/src/main/assets/exercises.bin")

add_executable(gym_preview tools/preview.cpp)
target_link_libraries(gym_preview PRIVATE gymcore)

enable_testing()
add_test(NAME gym_tests COMMAND gym_tests)
