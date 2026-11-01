// Draws every screen with sample data and writes PPM images.
// Usage: preview <output dir> [width height unit]
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>

#include "ui/app.hpp"

int main(int argc, char** argv) {
    using namespace gym;
    const std::filesystem::path dir = argc > 1 ? argv[1] : ".";
    std::filesystem::create_directories(dir);

    const std::int64_t now = std::time(nullptr);
    Log log{
        {now - 3 * 86400, {{0, 800, 5}, {0, 800, 5}, {0, 825, 3}, {3, 450, 8}, {3, 450, 7}}},
        {now - 86400, {{1, 1200, 5}, {1, 1200, 5}, {1, 1250, 3}, {9, 2000, 10}}},
        {now - 60, {{0, 825, 5}, {4, 700, 8}}},
    };

    const int w = argc > 3 ? std::atoi(argv[2]) : 720, h = argc > 3 ? std::atoi(argv[3]) : 1440;
    const float unit = argc > 4 ? std::strtof(argv[4], nullptr) : 2.0f;
    App app(std::move(log), nullptr, unit);
    Canvas canvas;
    canvas.resize(w, h);
    const auto t0 = App::Clock::now();
    app.frame(canvas, Input{}, t0);  // gives the app a clock
    app.log_set();                   // starts the rest timer so it shows on screen

    const char* names[] = {"workout", "history", "records"};
    int i = 0;
    for (const auto tab : {Tab::Workout, Tab::History, Tab::Records}) {
        app.set_tab(tab);
        app.frame(canvas, Input{}, t0 + std::chrono::seconds(20));
        std::ofstream f(dir / (std::string(names[i++]) + ".ppm"), std::ios::binary);
        f << "P6\n" << canvas.width() << ' ' << canvas.height() << "\n255\n";
        for (int y = 0; y < canvas.height(); ++y)
            for (const auto px : canvas.row(y)) {
                const char rgb[3] = {char(px & 0xFF), char((px >> 8) & 0xFF), char((px >> 16) & 0xFF)};
                f.write(rgb, 3);
            }
    }
    std::puts("done");
}
