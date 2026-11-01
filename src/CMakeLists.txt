add_library(gymcore STATIC
    core/exercises.cpp
    core/stats.cpp
    core/store.cpp
    ui/app.cpp
    ui/picker.cpp
    ui/plan.cpp
    ui/canvas.cpp)
target_include_directories(gymcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(gymcore PUBLIC cxx_std_23)
target_compile_options(gymcore PRIVATE -Wall -Wextra -Wpedantic)
