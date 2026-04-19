# AI Powered (Vehicle) Surveillance System
A light-weight Vehicle Surveillance System for our BS Computer Science Final Year Project, at [Khushal Khan Khattak University, Karak, PK](https://kkkuk.edu.pk). This project is part obsession, part survival, and a fair amount of spaghetti code.

## Motivation
This project was heavily inspired by the architecture of [Frigate NVR](https://github.com/blakeblackshear/frigate.git) and how it works, as I never had any experience with such heavy video pipeline systems. In other words, we only had 6-7 months to make a light-weight surveillance system and could not acheive something that big in such a short period. So, I really didn't have the time to make mistakes and find out.

## Demo
https://github.com/user-attachments/assets/e5eb7593-71a2-498c-a61f-13891cfdb85c

> [Footage by Sysvideo](https://www.youtube.com/watch?v=eO19UTm93GQ), looped.

> [!NOTE]
> * Tested only on Windows.
> * Report submitted to the university is [available in the v0.1 release](https://github.com/ahsanullah-8bit/APSS/releases/download/v0.1/APSS.Final.Report.pdf). It contains usage guides and everything.
> * We're in a bit of hurry at the moment. So, this might turnout more spaghetti than I thought.

## Building from Source
Building from source includes setting up VCPKG and some more configuration. I'm strictly following this approach till I find an easier way to resolve dependency issues. APSS uses VCPKG for some [packages](vcpkg.json) and uses prebuilt binaries for bigger and time-consuming dependencies through CMake's `FetchContent`.
Prebuilt binaries are hosted in [v0.1 release](https://github.com/ahsanullah-8bit/APSS/releases/tag/v0.1).

### Prerequisites

#### Tools
* MSVC (last built with v18.2.1)
* CMake
* Ninja (or any other generator)
* [VCPKG](https://learn.microsoft.com/en-us/vcpkg/get_started/overview) (optional)
* [ODB 2.5.0](https://github.com/codesynthesis-com/odb.git) Compiler
* [pkgconf 2.4.3](https://github.com/pkgconf/pkgconf.git)

> [!NOTE]
> * I'd recommend using Qt's Online Installer to download Qt, Qt Creator, etc. It'll be easier to set all the options and just build and run.
> * `pkgconf 2.4.3` is managed through VCPKG. If no VCPKG, you must set `-DPKG_CONFIG_EXECUTABLE=<path/to/pkgconf.exe>`
> * You can use VCPKG for Qt, if you prefer that. It's not included in the [vcpkg.json](vcpkg.json) to allow customization.

#### About the Models

The [models.zip](https://github.com/ahsanullah-8bit/APSS/releases/download/v0.1/models.zip) in the release contains a starter set to get the pipeline running. Unzip them into binary directory, the app explicitly looks for these models in a relative `models` directory.

*  **The "Non-Custom" ones:** Standard exports for YOLO and PaddleOCR. I don't own these; they’re just there so you don't have to hunt them down. Feel free to swap them with your own weights.

* **The "Custom" ones:** The `yolo11n-pose-` models are the ones I trained myself for license plate detection. The `1700` and `3k` means the amount of images used during training.

* **Disclaimer:** These were chosen because they were the only thing my laptop could run without melting. If you have the hardware, you should definitely try swapping the Nano models for something heavier.

> [!NOTE]
> The system can't use a dedicated GPU. It only works with ONNXRuntime and OpenVINO Execution Provider.

#### Dependencies
* Qt 6.10 (Qt Quick, Multimedia, Network, ...)
* [ONNXRuntime v5.6](https://github.com/intel/onnxruntime.git) (v1.21)
* [OpenVINO 2025.1](https://github.com/openvinotoolkit/openvino.git)
* [ByteTrackEigen](https://github.com/ahsanullah-8bit/ByteTrackEigen.git)
* [fmr](https://github.com/ahsanullah-8bit/fmr.git)
* [ODB C++ 2.5.0](https://github.com/codesynthesis-com/odb.git) (libodb, qt, sqlite)
* [OpenCV 4.10.0](https://github.com/opencv/opencv/tree/master) (fs, gstreamer, eigen, highgui, jpeg, png)
* [oneTBB 2022.0.0](https://github.com/uxlfoundation/oneTBB.git)
* [FFMpeg 7.1.1](https://github.com/FFmpeg/FFmpeg.git) (avcodec, avformat, avdevice, avfilter, swscale, swresample, openssl, zlib, x264, x265, freetype, drawtext)
* [Eigen 3.4.0](https://github.com/PX4/eigen.git) (needed by ByteTrackEigen)
* [yaml-cpp 0.8.0](https://github.com/jbeder/yaml-cpp.git)
* [reflect-cpp 0.17.0](https://github.com/getml/reflect-cpp.git) (yaml)
* [cppzmq 4.10.0](https://github.com/zeromq/cppzmq)
* [GTest 1.16.0](https://github.com/google/googletest.git) (needed for tests)

> [!NOTE] 
> * ONNXRuntime and OpenVINO are automated through `FetchContent`. If you've a custom build, pass `-Donnxruntime_DIR` or `-Donnxruntime_ROOT` (and so on) to CMake during configuration.
> * Don't get confused in ONNXRuntime v5.6 (intel) and ONNXRuntime 1.21.1 (microsoft). They're basically the same thing. But when I was building it from source, the original (Microsoft) repo had not merged the PR for OpenVINO 2025.1 support yet. I'll be adding ONNXRuntime 1.22 Soon.
> * ByteTrackEigen and fmr are git submodules. So you must use `--recursive` while cloning.
> * The rest are managed by [vcpkg.json](vcpkg.json). Note however, **libodb 2.5.0** is added through the custom-ports. So, you must pass `-DVCPKG_OVERLAY_PORTS=./custom-ports` to CMake.
> * If you don't want to use VCPKG, you must pass `<pkg_name>_DIR` or `<pkg_name>_ROOT` for your custom alternative, replacing `pkg_name` with the package name. Look at the [CMakeLists.txt](CMakeLists.txt), to see the names of each package `find_package(<pkg_name> ...)`.

### Build Using CMake

* Open `x64 Native Tools Command Prompt for VS` and `cd` directory somewhere.

* Clone the project

	```bash
	git clone --recursive https://github.com/ahsanullah-8bit/APSS.git
	cd APSS
	```

* Configure using
	```bash
	cmake -S . -B build # Add more options
	```
	* Some options
		* `-DCMAKE_TOOLCHAIN_FILE=<vcpkg_root>/scripts/buildsystems/vcpkg.cmake` sets up vcpkg toolchain file option **OR** setup Qt Creator's `Edit -> Preferences -> CMake -> General -> Package Manager Auto Setup`.
		* `-DVCPKG_MANIFEST_MODE=ON` to turn on Manifest mode.
		* `-DVCPKG_OVERLAY_PORTS=./custom-ports` for libodb ports.
		* `-DQT_QMAKE_EXECUTABLE=<qt_root>/<version>/msvc2022_64/bin/qmake.exe` for Qt.  
	* Full command (with VCPKG)
	```bash
	cmake -S . -B build -G "Ninja" -DCMAKE_TOOLCHAIN_FILE="<vcpkg_dir>\scripts\buildsystems\vcpkg.cmake" -DVCPKG_MANIFEST_MODE=ON -DVCPKG_OVERLAY_PORTS=./custom-ports -DQT_QMAKE_EXECUTABLE="<qt_root/<version>/msvc2022_64/bin/qmake.exe"

	# Replace the `<,,,>` with proper paths.
	```

* Build using
	```bash
	cmake --build build
	```

> [!TIP]
> If you use VCPKG and do frequent builds. I'd recommend looking into [Binary Caching using a Nuget Feed](https://learn.microsoft.com/en-us/vcpkg/consume/binary-caching-nuget) or alternatives.

## Thoughts & Technical Analysis

### The Setup
This project was built and tested on a **Lenovo V14 G3 IAP** (i5-1235U, 8GB RAM). Resource constraints on this mobile chipset significantly influenced the ability to properly debug and test new things.
* 12th Gen Intel Core i5-1235(U)
* 8GB DDR4 RAM
* I think that's enough unboxing...

### Known Limitations & Challenges
While the core logic is functional, several components faced bottlenecks due to the trade-off between real-time performance and stability. Adding one extra feature would result in a nightmare of debugging and, that is after loading 5 models at once in most cases.

* **Object Detection:** I've only used and tested the [YOLO11n](https://docs.ultralytics.com/models/yolo11) model, which is efficient but the accuracy is lower to that of larger variants. Moving to a Medium or Larg model on proper hardware would likely solve majority of detection misses.

* **Object Tracking:** The custom C++ ByteTrack implementation occasionally has jitters. It's not clear if this a logic bug or a side effect of the lower confidence scores provided by the nano YOLO detection model.
* **Recording:** I've tried 2 ways of recording and both reached certain limits at some point. First try was re-encoding, then just remuxing the packets.
	* **Clip/Event:** Recording per-event footage clips (similar to Frigate NVR). The system couldn't keep up.
	* **Whole Footage + Tagging:** Recording the whole footage and somehow tagging the events into that footage. It seemed pretty much impossible with my kind of expertise. You've to keep track of the frames recorded and associate the predictions occured at each frame during review.
* **Tests:** Tests are "broken" at the moment. Since the project was accepted as-is, I'm stepping away from it for now. The presentation only lasted 2 minutes, and honestly, the project is probably going to stay in 'maintenance mode' indefinitely. It was a massive learning experience, but it’s time to let it rest.