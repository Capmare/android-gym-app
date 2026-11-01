// Android entry point. Pure C++: no Java or Kotlin.
// NativeActivity loads this library and calls android_main().
#include <jni.h>

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

struct Insets {
    int top = 0, bottom = 0;
};

struct State {
    ANativeWindow* window = nullptr;
    JNIEnv* env = nullptr;      // attached to this thread in android_main
    int settle_frames = 0;      // extra redraws after start-up, while Android finishes the layout
    gym::Canvas canvas;
    std::unique_ptr<gym::App> app;
    gym::Input in;
    int slop = 20;          // pixels a finger may move and still count as a tap
    bool need_draw = true;
};

// Asks Android how tall the status bar and the navigation bar are.
// This is the same as `decorView.getRootWindowInsets().getSystemWindowInsetTop()` in Java.
// It uses JNI, but the app still has no Java or Kotlin code of its own.
Insets query_insets(State& s, android_app* app) {
    JNIEnv* env = s.env;
    Insets result;
    if (!env || env->PushLocalFrame(16) != 0) return result;  // frees all local references at the end

    jobject activity = app->activity->clazz;
    const auto call_object = [&](jobject obj, const char* method, const char* sig) -> jobject {
        if (!obj) return nullptr;
        jclass cls = env->GetObjectClass(obj);
        jmethodID id = env->GetMethodID(cls, method, sig);
        if (!id) {
            env->ExceptionClear();  // "no such method": carry on without insets
            return nullptr;
        }
        return env->CallObjectMethod(obj, id);
    };
    jobject window = call_object(activity, "getWindow", "()Landroid/view/Window;");
    jobject decor = call_object(window, "getDecorView", "()Landroid/view/View;");
    jobject insets = call_object(decor, "getRootWindowInsets", "()Landroid/view/WindowInsets;");
    if (insets) {
        jclass cls = env->GetObjectClass(insets);
        const auto get = [&](const char* method) {
            jmethodID id = env->GetMethodID(cls, method, "()I");
            if (!id) {
                env->ExceptionClear();
                return 0;
            }
            return env->CallIntMethod(insets, id);
        };
        result.top = get("getSystemWindowInsetTop");
        result.bottom = get("getSystemWindowInsetBottom");
    }
    if (env->ExceptionCheck()) env->ExceptionClear();
    env->PopLocalFrame(nullptr);
    return result;
}

void draw(State& s, android_app* app) {
    ANativeWindow_Buffer buf;
    if (ANativeWindow_lock(s.window, &buf, nullptr) < 0) return;

    const Insets insets = query_insets(s, app);
    s.app->set_insets(insets.top, insets.bottom);

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
    if (s.settle_frames > 0) --s.settle_frames;
}

void on_cmd(android_app* app, int32_t cmd) {
    auto& s = *static_cast<State*>(app->userData);
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            s.window = app->window;
            s.settle_frames = 15;
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
    auto& s = *static_cast<State*>(app->userData);

    // Back button or back gesture: close the search page or the keyboard first.
    if (AInputEvent_getType(ev) == AINPUT_EVENT_TYPE_KEY) {
        if (AKeyEvent_getKeyCode(ev) != AKEYCODE_BACK) return 0;
        // On the first screen there is nothing to close: return 0 so Android closes the app.
        if (!s.app->can_go_back()) return 0;
        if (AKeyEvent_getAction(ev) == AKEY_EVENT_ACTION_UP && s.app->back()) s.need_draw = true;
        return 1;
    }
    if (AInputEvent_getType(ev) != AINPUT_EVENT_TYPE_MOTION) return 0;
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
        gym::load_file(file), [file](const gym::Data& data) { gym::save_file(file, data); }, unit);
    app->activity->vm->AttachCurrentThread(&state.env, nullptr);

    while (!app->destroyRequested) {
        // Wait for events. While the rest timer runs, wake up 5 times a second.
        const bool ticking = state.app->animating() || state.settle_frames > 0;
        const int timeout_ms = ticking ? 200 : -1;
        int events = 0;
        android_poll_source* source = nullptr;
        int timeout = timeout_ms;
        while (ALooper_pollOnce(timeout, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) {
                app->activity->vm->DetachCurrentThread();
                return;
            }
            timeout = 0;  // handle all waiting events, then draw
        }
        if (state.window && (state.need_draw || ticking)) draw(state, app);
    }
}
