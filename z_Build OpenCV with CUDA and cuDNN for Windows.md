
# Build OpenCV with CUDA and cuDNN for Windows

## Pre-requisites

1. Visual Studio (with C/C++ development toolkit)
2. Check your NVIDIA GPU model and then look it up on the [CUDA Wikipedia page](https://en.wikipedia.org/wiki/CUDA#GPUs_supported).
  - Download and install [CUDA](https://docs.nvidia.com/cuda/cuda-installation-guide-microsoft-windows/index.html#installing-cuda-development-tools)
  - Download and install [cuDNN](https://docs.nvidia.com/deeplearning/cudnn/install-guide/index.html#install-windows)
  - Download [NVIDIA Video Codec SDK](https://developer.nvidia.com/nvidia-video-codec-sdk). Mind that **the encoder is not supported** due to library deprecation. We are going to install the **decoding library only**. To do so, copy:
      - `$(download_dir)/Lib/x64/nvcuvid.lib` in `$(NVIDIA_GPU_DIR)/CUDA/vxx.x/lib/x64/`
      - `$(download_dir)/Interface/*.h` in `$(NVIDIA_GPU_DIR)/CUDA/vxx.x/include/`
3. OpenCV and OpenCV-contrib modules that can be downloaded from github
4. CMake


## Installation

- Uninstall any opencv version linked with pip
- `pip install numpy`
- Unzip both OpenCV and OpenCV-contrib
- Create a `build` folder (it works better if completely out of opencv path)

### CMake

1. Paths
    - where the source code: path-to-opencv-x-x-x
    - where the binaries: your `build` directory
    - click on `Configure`

2. Variables to check

| VARIABLE                   | VALUE                            |
| -------------------------- | -------------------------------- |
| `WITH_CUDA`                | `TRUE`                           |
| `OPENCV_DNN_CUDA`          | `TRUE`                           |
| `ENABLE_FAST_MATH`         | `TRUE`                           |
| `CMAKE_CONFIGURATION_TYPE` | `Release`                        |
| `OPENCV_EXTRA_MODULES`     | `path-to-opencv-contrib/modules` |
| `CMAKE_INSTALL_PREFIX`     | `path-to-build/install`          |

Check [these variables](https://docs.opencv.org/master/d5/de5/tutorial_py_setup_in_windows.html) to be checked or unchecked, with particular attention to python paths.

Now modify the config again:

| VARIABLE                     | VALUE                                                        |
| ---------------------------- | ------------------------------------------------------------ |
| `CUDA_ARCH_BIN`              | select the one found on the [CUDA Wikipedia page](https://en.wikipedia.org/wiki/CUDA#GPUs_supported) |
| `CUDA_FAST_MATH`             | `TRUE`                                                       |
| `WITH_OPENGL`                | `TRUE`                                                       |
| `WITH_NVCUVID`               | `TRUE`                                                       |
| `ENABLE_PRECOMPILED_HEADERS` | `FALSE`, see [troubleshoot](https://github.com/opencv/opencv_contrib/issues/1786) |

After running `Configure`, check that `NVIDIA_nvcuvenc_LIBRARY` is not found, whereas `NVIDIA_nvcuvid_LIBRARY` is present.

Click on `Generate`.


### Visual Studio

- Go to your `opencv/build` folder. There you will find `OpenCV.sln` file. Open it with Visual Studio
  - If an error shows up: Tools → Options → Projects and Solutions → Web Projects: set last field _Automatically show data collections blabla.._ to `FALSE`.
- Close and open the same file again and:
  - Change `Debug` to `Release`
  - In solution explorer:
    - CmakeTargets → right click on `ALL_BUILDS` → `build`
    - CmakeTargets → right click on `INSTALL` → `build`


## Verify

Import cv2 in a python env and then run: `cv2.getCudaEnabledDeviceCound()`



# Other References

- https://www.youtube.com/watch?v=tjXkW0-4gME
- https://www.youtube.com/watch?v=YsmhKar8oOc
- https://docs.opencv.org/master/d2/de6/tutorial_py_setup_in_ubuntu.html
- https://docs.opencv.org/master/d7/d9f/tutorial_linux_install.html
- https://towardsdatascience.com/how-to-install-opencv-and-extra-modules-from-source-using-cmake-and-then-set-it-up-in-your-pycharm-7e6ae25dbac5



# Troubleshooting
- https://answers.opencv.org/question/188525/enable-cudacodecvideoreader
- https://github.com/opencv/opencv_contrib/issues/1786
