# AI Powered (Vehicle) Surveillance System
A light-weight Vehicle Surveillance System for our FYP, at Khushal Khan Khattak University, Karak, PK. This pile of spaghetti code
was inspired 🤏 this much by solving the issue of paper work done by our university's security gaurds for vehicle entrance and more
by my obssession of perfection and uncomplete features.

## Building
Building from source includes setting up vcpkg and some more configuration. I'm strictly following this approach till I find an easier
way to resolve dependency issue. APSS uses vcpkg for some packages and uses prebuilt binaries for bigger and time-consuming dependencies.

### Prerequisites

* Qt 6.9 (Multimedia, Network, Sql, ShaderTools, ...)
* ByteTrackEigen
* QxOrm
* vcpkg
* OpenCV 4.10.0 (fs, eigen, highgui, jpeg, png)
* tbb 2022.0.0
* ffmpeg 7.1.1 (avcodec, avformat, avdevice, avfilter, swscale, swresample, openssl, zlib, x264, x265, freetype, drawtext)
* eigen3 3.4.0 (needed by ByteTrackEigen)

> [!NOTE] 
> Packages mentioned bellow vcpkg are handled by vcpkg by default (Take a look at vcpkg.json). ByteTrackEigen and QxOrm are added as git
submodules. 
> I'd recommend using Qt's Online Installer to download Qt, Qt Creator, etc. It'll be easier to set all the options and just build and run.

> [!WARNING] 
> This project is tested on intel hardware. TBB however might have restrictions on other hardware. Go to their supported hardware page for more info.

### Build Using Qt Creator

* Open the project in Qt Creator.
* Go to `Projects` tab to configure different options. Some important ones include
    * `APSS_USE_OPENVINO_EP`: If you've Intel hardware.
	* `APSS_USE_CUDA_EP`: If you've an NVIDIA GPU.
	* Leave both off to use CPU. Which is default, if any of the above fails.
* You might get some errors, regarding `libx264-164.dll`. I'll try as much as possible to automate the copy of this dll. But if it doesn't happen, go to the
`<binary_dir>/vcpkg_installed/<triplet_name>/tools/x264/bin` and copy the dll to your executable directory.

> [!NOTE] 
> You'll also have to copy the `QxOrm`'s dlls located at `APSS/Dependencies/QxOrm/lib`. Not my fault, it's because of QxOrm's weird structure.
