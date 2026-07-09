# KUPL

## Release Notes

- [2026/03] The Kunpeng Unified Parallel Library (KUPL) project went live, featuring support for multi-core parallelism, data management, and matrix programming.

## Overview

KUPL provides foundational parallel acceleration functions optimized for the Kunpeng platform, with all APIs implemented in C/C++ and assembly. The library offers core capabilities such as thread management, task scheduling, thread synchronization, memory allocation, shared memory communication, and matrix programming. These features are designed to fully leverage the hardware characteristics of Kunpeng processors to deliver high-performance foundational APIs.

## Version Mapping

- Operating platform
    - Kunpeng 920 series
- System specifications
    - openEuler 20.03 LTS SP3 AArch64
    - openEuler 22.03 LTS SP2 AArch64
    - openEuler 22.03 LTS SP3 AArch64
    - openEuler 22.03 LTS SP4 AArch64
    - openEuler 24.03 LTS SP3 AArch64
    - Kylin Linux Advanced Server V10 (Hydrogen) AArch64
    - Kylin Linux Advanced Server V10 (Sword) AArch64
    - Kylin Linux Advanced Server Industry V10 AArch64
    - Kylin Linux Advanced Server V10 (Jasmine)
    - Kylin Linux Advanced Server V10 (GFB)
    - Kylin Linux Advanced Server V11 (Swan25)
    - Kylinsec OS Linux 3 (Qomolangma) AArch64 (3.5.2)
    - Kylinsec OS Linux 3 (Qomolangma) AArch64 (3.5.3)

## Build and Installation

### 1. [Obtain the HPCKit software package](https://www.hikunpeng.com/document/detail/zh/kunpenghpcs/hpckit/instg/KunpengHPCKit_install_007.html).

> https://www.hikunpeng.com/developer/hpc/hpckit-download

### 2. [Install HPCKit](https://www.hikunpeng.com/document/detail/zh/kunpenghpcs/hpckit/instg/KunpengHPCKit_install_012.html).

#### Extract the HPCKit software package (replace the version number in the example with your actual version).

```shell
tar xvf HPCKit_26.0.RC1_Linux-aarch64.tar.gz
```

#### Install HPCKit.

```shell
sh HPCKit_26.0.RC1_Linux-aarch64/install.sh -y --prefix=[HPCKit_installation_directory]
```

### 3. [Set environment variables](https://www.hikunpeng.com/document/detail/zh/kunpenghpcs/hpckit/instg/KunpengHPCKit_install_014.html).

#### Load the module.

```shell
module use [HPCKit_installation_directory]/HPCKit/latest/modulefiles
```

#### Load the environment variables of the compiler.

Identify your preferred compiler (GCC or BiSheng Compiler) and execute the loading command via the terminal.

- GCC (Replace the version number in the example with your actual version.)

```shell
module load gcc/compiler12.3.1/gccmodule
```

- BiSheng Compiler (Replace the version number in the example with your actual version.)

```shell
module load bisheng/compiler5.1.0.2/bishengmodule
```

### 4. Install the dependencies required for build.

Install CMake.

```shell
yum install cmake
```

### 5. Start the build process.

Identify the build type (GCC or BiSheng Compiler) and execute the build command via the terminal.

- GCC

```shell
sh build.sh --install_path=[KUPL_installation_directory]
```

- BiSheng Compiler

```shell
sh build.sh --compiler=clang --install_path=[KUPL_installation_directory]
```

### 6. Perform other operations.

#### View build options.

```shell
sh build.sh --help
```

#### Build and run the test program (GCC).

```shell
module unload bisheng/hmpi26.0.RC1/release

module load gcc/hmpi26.0.RC1/release

sh build.sh --build_kind=test

sh run_lcov.sh
```

#### Build and run the test program (BiSheng Compiler).

```shell
module unload gcc/hmpi26.0.RC1/release

module load bisheng/hmpi26.0.RC1/release

sh build.sh --build_kind=test --compiler=clang

sh run_lcov.sh test clang
```

## Tutorials

If you are familiar with the build and installation process and would like to gain a deeper understanding of the project, please visit the following detailed tutorials:

### Many-core Parallelism

[Executor-related functions](https://www.hikunpeng.com/document/detail/zh/kunpenghpcs/hpckit/devg/KunpengHPCKit_developer_020.html): Learn about KUPL's executor APIs, covering features like obtaining the current executor ID and total executor count, as well as fine-grained control over multi-threading.

[Multi-threaded programming functions](https://www.hikunpeng.com/document/detail/zh/kunpenghpcs/hpckit/devg/KunpengHPCKit_developer_031.html): Learn about KUPL's multi-threaded programming capabilities, which enable concurrent threads within a single process to execute different tasks in parallel, thereby boosting performance.

[Computational graph programming functions](https://www.hikunpeng.com/document/detail/zh/kunpenghpcs/hpckit/devg/KunpengHPCKit_developer_035.html): Learn about the dynamic and static graph programming models within KUPL.

[Multi-queue and multi-stream programming functions](https://www.hikunpeng.com/document/detail/zh/kunpenghpcs/hpckit/devg/KunpengHPCKit_developer_048.html): Learn about KUPL's multi-queue and multi-stream programming models, as well as core concepts regarding queues and events.

### Data Management

[Memory management functions](https://www.hikunpeng.com/document/detail/zh/kunpenghpcs/hpckit/devg/KunpengHPCKit_developer_064.html): Learn about KUPL's memory operation APIs, including capabilities for memory allocation, copying, and locking.

[Shared Memory Communication Functions](https://www.hikunpeng.com/document/detail/zh/kunpenghpcs/hpckit/devg/KunpengHPCKit_developer_085.html): Learn about KUPL's low-level shared memory communication APIs and the implementation of collective communication functions based on these APIs.

### Matrix Programming

[Matrix Programming API Functions](https://www.hikunpeng.com/document/detail/zh/kunpenghpcs/hpckit/devg/KunpengHPCKit_developer_104.html): Learn about KUPL's matrix programming APIs, including capabilities for accelerating matrix multiplication and copying.

## Directory Structure

```shell
├── cmake                              # Project build directory
├── src                                # Project source directory
│   ├── core                           # Core module of the project
│   ├── dm                             # Data management module
│   ├── executor                       # Executor module
│   ├── memory                         # Memory module
│   ├── mma                            # Matrix programming module
│   ├── mt                             # Multi-threaded programming module
│   ├── tools                          # Tool module
│   ├── utils                          # Public basic class library
│   ├── CMakeLists.txt                 # Source build configuration file
│   ├── kupl_mma.h                     # Matrix programming header file
│   └── kupl.h                         # KUPL header file
├── test                               # Project test directory
├── build.sh                           # Project build script
├── CMakeLists.txt                     # Project build configuration file
├── LICENSE                            # License file
├── llvm-gcov.sh                       # LLVM coverage statistics script
├── README.md                          # Readme
└── run_lcov.sh                        # Script for installing dependencies
```

## Contact Us

Features and documentation are updated regularly. Please follow the latest version for the most up-to-date information.

- **Issue feedback**: Submit queries or report bugs via [Issues](https://atomgit.com/kunpengcompute/kupl/issues).
- **Community interaction**: Join discussions and share ideas via [Discussions](https://atomgit.com/kunpengcompute/kupl/discussions).
- **Technical columns**: Access in-depth technical articles, serialized tutorials, and best practices through the [Kunpeng Community](https://www.hikunpeng.com/developer/techArticles).
