# Gym Log

An Android gym app written in C++23. It has no Java or Kotlin code and no third-party libraries.

- Log sets (weight and reps) and get a 90 second rest timer.
- See your workout history.
- See your personal records (estimated 1 rep max).
- Data stays on the phone, in the app's private folder.

## Build

Tests and screenshots on a computer:

    cmake -S . -B build -G Ninja && cmake --build build
    ctest --test-dir build
    ./build/gym_preview out/      # writes PPM images of each screen

APK: open the `android/` folder in Android Studio and press Run.
Or use `cd android && gradle assembleDebug`. You need the Android SDK and NDK 27.

## Layout

- `src/core` - data, statistics, file storage
- `src/ui` - software renderer and screens
- `src/android/main.cpp` - Android entry point
