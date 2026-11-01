# Gym Log

An Android gym app written in C++23. It has no Java or Kotlin code and no third-party libraries.

- Log sets (weight and reps) and get a 90 second rest timer.
- 195 exercises with photos (173 have them). Search by name, and filter by muscle and by equipment.
- Workout plans, and a week schedule: choose a plan for each day. Today's plan shows on the Workout tab.
- A joke exercise, **Smoking**: log how many cigarettes, before or after training, the brand, and pack or self rolled.
- See your workout history.
- See your personal records (estimated 1 rep max).
- Data stays on the phone, in the app's private folder.

## Build

Tests and screenshots on a computer:

    cmake -S . -B build -G Ninja && cmake --build build
    ctest --test-dir build
    ./build/gym_preview out/      # writes PPM images of each screen

APK: every push builds it on GitHub. Download the plain file (no zip) from the Releases page:
`https://github.com/Capmare/android-gym-app/releases/download/latest/gym-log.apk`

Or open the `android/` folder in Android Studio and press Run.
Or use `cd android && gradle assembleDebug`. You need the Android SDK and NDK 27.

## Exercise photos

The photos come from [free-exercise-db](https://github.com/yuhonas/free-exercise-db), which is public domain (Unlicense).
`tools/pack_images.py` downloads them and packs two photos per exercise into
`android/app/src/main/assets/exercises.bin` (13 MB). Run it again if you add exercises.

## Layout

- `src/core` - data, exercise list, statistics, file storage
- `src/ui` - software renderer and screens
- `src/android/main.cpp` - Android entry point
