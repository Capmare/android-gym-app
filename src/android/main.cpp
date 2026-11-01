// Android entry point. Pure C++: no Java or Kotlin.
// NativeActivity loads this library and calls android_main().
#include <android/configuration.h>
#include <android/input.h>
#include <android/log.h>
#include <android/looper.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>

#include "core/store.hpp"
#include "ui/app.hpp"

namespace {

struct State {
    ANativeWindow* window = nullptr;
    gym::Canvas canvas;
    std::unique_ptr<gym::App> app;
    gym::Input in;
    int slop = 20;          // pixels a finger may move and still count as a tap
    bool need_draw = true;
};

void draw(State& s) {
    ANativeWindow_Buffer buf;
    if (ANativeWindow_lock(s.window, &buf, nullptr) < 0) return;

    if (s.canvas.width() != buf.width || s.canvas.height() != buf.height) s.canvas.resize(buf.width, buf.height);
    s.app->frame(s.canvas, s.in, std::chrono::steady_clock::now());

    // `stride` is the row length of the buffer in pixels. It can be larger than the width.
    auto* dst = static_cast<std::uint32_t*>(buf.bits);
    for (int y = 0; y < buf.height; ++y) {
        const auto row = s.canvas.row(y);
        std::memcpy(dst + std::size_t(y) * buf.stride, row.data(), row.size_bytes());
    }
    ANativeWindow_unlockAndPost(s.window);

    s.in.released = false;
    s.in.drag_dy = 0;
    s.need_draw = false;
}

void on_cmd(android_app* app, int32_t cmd) {
    auto& s = *static_cast<State*>(app->userData);
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            s.window = app->window;
            // 0, 0 = keep the native screen size. One pixel = one screen pixel.
            ANativeWindow_setBuffersGeometry(s.window, 0, 0, WINDOW_FORMAT_RGBA_8888);
            s.need_draw = true;
            break;
        case APP_CMD_TERM_WINDOW:
            s.window = nullptr;
            break;
        case APP_CMD_WINDOW_RESIZED:
        case APP_CMD_CONTENT_RECT_CHANGED:
        case APP_CMD_GAINED_FOCUS:
            s.need_draw = true;
            break;
        default:
            break;
    }
}

int32_t on_input(android_app* app, AInputEvent* ev) {
    if (AInputEvent_getType(ev) != AINPUT_EVENT_TYPE_MOTION) return 0;
    auto& s = *static_cast<State*>(app->userData);
    auto& in = s.in;

    const int x = static_cast<int>(AMotionEvent_getX(ev, 0));
    const int y = static_cast<int>(AMotionEvent_getY(ev, 0));
    switch (AMotionEvent_getAction(ev) & AMOTION_EVENT_ACTION_MASK) {
        case AMOTION_EVENT_ACTION_DOWN:
            in = {};
            in.x = in.press_x = x;
            in.y = in.press_y = y;
            in.down = true;
            break;
        case AMOTION_EVENT_ACTION_MOVE:
            in.drag_dy += y - in.y;
            in.x = x;
            in.y = y;
            if (std::abs(x - in.press_x) > s.slop || std::abs(y - in.press_y) > s.slop) in.moved = true;
            break;
        case AMOTION_EVENT_ACTION_UP:
            in.x = x;
            in.y = y;
            in.down = false;
            in.released = true;
            break;
        case AMOTION_EVENT_ACTION_CANCEL:
            in.down = false;
            in.moved = true;
            break;
        default:
            return 0;
    }
    s.need_draw = true;
    return 1;
}

}  // namespace

void android_main(android_app* app) {
    State state;
    app->userData = &state;
    app->onAppCmd = on_cmd;
    app->onInputEvent = on_input;

    // 160 dpi = 1.0. Some devices report "no density"; use 1.0 then.
    int dpi = AConfiguration_getDensity(app->config);
    if (dpi <= 0 || dpi > 1000) dpi = 160;
    const float unit = static_cast<float>(dpi) / 160.0f;
    state.slop = static_cast<int>(std::lround(8.0f * unit));

    const std::filesystem::path file = std::filesystem::path(app->activity->internalDataPath) / "gym.log";
    state.app = std::make_unique<gym::App>(
        gym::load_file(file), [file](const gym::Log& log) { gym::save_file(file, log); }, unit);

    while (!app->destroyRequested) {
        // Wait for events. While the rest timer runs, wake up 5 times a second.
        const int timeout_ms = state.app->animating() ? 200 : -1;
        int events = 0;
        android_poll_source* source = nullptr;
        int timeout = timeout_ms;
        while (ALooper_pollOnce(timeout, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) return;
            timeout = 0;  // handle all waiting events, then draw
        }
        if (state.window && (state.need_draw || state.app->animating())) draw(state);
    }
}
