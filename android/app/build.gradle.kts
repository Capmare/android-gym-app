plugins {
    id("com.android.application")
}

android {
    namespace = "dev.gymlog"
    compileSdk = 34
    ndkVersion = "27.2.12479018"

    defaultConfig {
        applicationId = "dev.gymlog"
        minSdk = 26
        targetSdk = 34   // 35 forces edge-to-edge drawing. The app does not handle that yet.
        // Android only installs an update if the version number goes up. GitHub counts builds upward.
        val build = (System.getenv("GITHUB_RUN_NUMBER") ?: "1").toInt()
        versionCode = build
        versionName = "0.1.$build"
        ndk { abiFilters += listOf("arm64-v8a", "x86_64") }
    }

    // Keep the photo file uncompressed. Android then maps it into memory instead of copying it.
    androidResources {
        noCompress += "bin"
    }

    // Android only installs an update if it is signed with the same key as the installed app.
    // GitHub makes a new random debug key for every build, so updates failed. This one key is
    // kept in the repo, so every build is signed the same way. It is for personal use only.
    signingConfigs {
        getByName("debug") {
            storeFile = rootProject.file("debug.keystore")
            storePassword = "android"
            keyAlias = "androiddebugkey"
            keyPassword = "android"
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}
