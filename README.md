> [!NOTE]
> Works only on Windows.
> We're in a bit of hurry at the moment. So, this might turnout more spaghetti than I thought.


# AI Powered (Vehicle) Surveillance System
A light-weight Vehicle Surveillance System for our FYP, at Khushal Khan Khattak University, Karak, PK. This pile of spaghetti code
was inspired 🤏 this much by solving the issue of paper work done by our university's security gaurds for vehicle entrance and more
by my obssession of perfection and uncomplete features. Man, I regret throwin' it at the (show your idea) presentation, it could've been much easier!

## Building from Source
Building from source includes setting up vcpkg and some more configuration. I'm strictly following this approach till I find an easier
way to resolve dependency issue. APSS uses vcpkg for some packages and uses prebuilt binaries for bigger and time-consuming dependencies.
Prebuilt binaries are hosted in my [0.1 release](https://github.com/ahsanullah-8bit/APSS/releases/tag/v0.1) as it takes a lot of time building from source the right way.

### Prerequisites

#### Tools
* IDE (i.e Qt Creator, VS 2022, etc.)
* [vcpkg](https://learn.microsoft.com/en-us/vcpkg/get_started/overview)
* [ODB 2.5.0](https://github.com/codesynthesis-com/odb.git) Compiler
* [pkgconf 2.4.3](https://github.com/pkgconf/pkgconf.git)

> [!NOTE]
> I'd recommend using Qt's Online Installer to download Qt, Qt Creator, etc. It'll be easier to set all the options and just build and run.
> `ODB 2.5.0 compiler` is automated through `FetchContent` and `pkgconf 2.4.3` is managed through vcpkg. 
> You can do the same for Qt, if you prefer that. It's not included in the [vcpkg.json](vcpkg.json) to allow customization.

#### Dependencies
* Qt 6.9 (Qt Quick, Multimedia, Network, ...)
* [ONNXRuntime v5.6](https://github.com/intel/onnxruntime.git)
* [OpenVINO 2025.1](https://github.com/openvinotoolkit/openvino.git) (optional, used as an EP by ONNXRuntime)
* [ByteTrackEigen](https://github.com/ahsanullah-8bit/ByteTrackEigen.git)
* [ODB C++ 2.5.0](https://github.com/codesynthesis-com/odb.git) (libodb, qt, sqlite)
* [OpenCV 4.10.0](https://github.com/opencv/opencv/tree/master) (fs, eigen, highgui, jpeg, png)
* [oneTBB 2022.0.0](https://github.com/uxlfoundation/oneTBB.git)
* [FFMpeg 7.1.1](https://github.com/FFmpeg/FFmpeg.git) (avcodec, avformat, avdevice, avfilter, swscale, swresample, openssl, zlib, x264, x265, freetype, drawtext)
* [Eigen 3.4.0](https://github.com/PX4/eigen.git) (needed by ByteTrackEigen)
* [reflect-cpp 0.17.0](https://github.com/getml/reflect-cpp.git) (yaml)
* [GTest 1.16.0](https://github.com/google/googletest.git) (needed for tests)

> [!NOTE] 
> * ONNXRuntime, OpenVINO and ODB (exe) + ODB C++ are automated through `FetchContent`, otherwise mention `-Donnxruntime_DIR` or `-Donnxruntime_ROOT` (and so on) at configure time (see [Build Guide](#build-using-qt-creator)).
> * Don't get confused in ONNXRuntime v5.6 (intel) and ONNXRuntime 1.21.1 (microsoft). They're basically the same thing. But when I was building it from source, the original (Microsoft) repo had not merged the PR for OpenVINO 2025.1 support yet. I'll be adding ONNXRuntime 1.22 Soon.
> * ByteTrackEigen is a submodule. So you must use `--recursive` while cloning.
> * The rest are managed by vcpkg (take a look at vcpkg.json). 

> [!WARNING] 
> This project is tested on intel hardware. TBB however might have restrictions on other hardware. Go to their supported hardware page for more info.

### Build Using Qt Creator

* Clone the project

```
git clone --recursive https://github.com/ahsanullah-8bit/APSS.git
```
* Open the project in Qt Creator.
* (Optional) Set up vcpkg, use `-DCMAKE_TOOLCHAIN_FILE=...` option or setup Qt Creator's `Edit -> Preferences -> CMake -> General -> Package Manager Auto Setup`.
* Go to `Projects` tab to configure different options. Some important ones include
    * `APSS_USE_OPENVINO_EP`: If you've Intel hardware.
	* `APSS_USE_CUDA_EP`: If you've an NVIDIA GPU (not tested).
	* Leave both off to use CPU (not recommended). Which is default, if any of the above fails.
	* `<pkg_name>_DIR` or `<pkg_name>_ROOT`: replacing `pkg_name` with onnxruntime, OpenVINO, OpenCV, TBB, Eigen3, reflectcpp and gtest respectively.
	* `odb_EXECUTABLE`: for odb compiler.
	* `libodb_ROOT`: for libodb, qt profile and its sqlite db in one place.
	* FFMpeg has no other choice, if vcpkg is used. Because it uses pkgconf and pkgconf searches `CMAKE_PREFIX_PATH` to look for modules.
* Build and Run.
* If you use vcpkg and do frequent builds. I'd recommend looking into [Binary Caching using a Nuget Feed](https://learn.microsoft.com/en-us/vcpkg/consume/binary-caching-nuget) or alternatives.

> [!NOTE]
> Copying of dlls to the binary directory is automated for non-vcpkg packages, as vcpkg does copy dlls automatically.
