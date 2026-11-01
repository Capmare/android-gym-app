// Draws every screen with sample data and writes PPM images.
// Usage: preview <output dir> [width height unit inset_top inset_bottom photos.bin]
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>

#include "core/images.hpp"
#include "ui/app.hpp"

int main(int argc, char** argv) {
    using namespace gym;
    const std::filesystem::path dir = argc > 1 ? argv[1] : ".";
    std::filesystem::create_directories(dir);

    const std::int64_t now = std::time(nullptr);
    Data data;
    data.log = {
        {now - 3 * 86400, {{0, 800, 5}, {0, 800, 5}, {0, 825, 3}, {3, 450, 8}, {3, 450, 7}}},
        {now - 86400, {{1, 1200, 5}, {1, 1200, 5}, {1, 1250, 3}, {9, 2000, 10}}},
        {now - 60, {{0, 825, 5}, {4, 700, 8}}},
    };
    data.plans = {{"PUSH DAY", {{0, 4, 8}, {3, 3, 10}, {8, 3, 12}}}, {"LEG DAY", {{1, 5, 5}, {9, 3, 10}}}};
    for (const int d : {0, 3}) data.week[static_cast<std::size_t>(d)] = 0;
    for (const int d : {1, 4}) data.week[static_cast<std::size_t>(d)] = 1;
    // Make today a push day so the workout screen shows the plan.
    const std::time_t tt = std::time(nullptr);
    std::tm tm{};
    localtime_r(&tt, &tm);
    data.week[static_cast<std::size_t>((tm.tm_wday + 6) % 7)] = 0;

    const int w = argc > 3 ? std::atoi(argv[2]) : 720, h = argc > 3 ? std::atoi(argv[3]) : 1440;
    const float unit = argc > 4 ? std::strtof(argv[4], nullptr) : 2.0f;
    const int inset_top = argc > 5 ? std::atoi(argv[5]) : 0, inset_bottom = argc > 6 ? std::atoi(argv[6]) : 0;
    const auto photo_bytes = read_binary_file(argc > 7 ? argv[7] : "android/app/src/main/assets/exercises.bin");
    const ImagePack photos(photo_bytes);
    App app(std::move(data), nullptr, unit);
    app.set_images(&photos);
    app.set_insets(inset_top, inset_bottom);
    Canvas canvas;
    canvas.resize(w, h);
    const auto t0 = App::Clock::now();
    app.frame(canvas, Input{}, t0);  // gives the app a clock
    app.log_set();                   // starts the rest timer so it shows on screen

    const auto save = [&](const char* file) {
        app.frame(canvas, Input{}, t0 + std::chrono::seconds(20));
        std::ofstream f(dir / (std::string(file) + ".ppm"), std::ios::binary);
        f << "P6\n" << canvas.width() << ' ' << canvas.height() << "\n255\n";
        for (int y = 0; y < canvas.height(); ++y)
            for (const auto px : canvas.row(y)) {
                const char rgb[3] = {char(px & 0xFF), char((px >> 8) & 0xFF), char((px >> 16) & 0xFF)};
                f.write(rgb, 3);
            }
    };

    app.set_tab(Tab::Workout);
    save("1_workout");
    app.set_tab(Tab::Plan);
    save("2_plan");
    app.set_tab(Tab::History);
    save("3_history");
    app.set_tab(Tab::Records);
    save("4_records");
    app.open_picker(false);
    app.set_search("press");
    save("5_search");
    app.show_keyboard();
    save("6_search_keyboard");
    app.back();
    app.back();
    app.edit_plan(0);
    save("7_plan_edit");
    std::puts("done");
}
