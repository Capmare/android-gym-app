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
        versionCode = 1
        versionName = "0.1"
        ndk { abiFilters += listOf("arm64-v8a", "x86_64") }
    }

    // Keep the photo file uncompressed. Android then maps it into memory instead of copying it.
    androidResources {
        noCompress += "bin"
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}
