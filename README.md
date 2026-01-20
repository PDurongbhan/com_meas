# COM_MEAS with AimIO (Windows, CMake, conda)

This repository is a release build of a small application
to perform joint centre of mass calculation on **Windows** using:

* MSVC (Visual Studio, x64)
* CMake (>= 3.20)
* Ninja
* conda-forge / numerics88 packages (Boost, n88util, AimIO)

The goal of this repo is **reproducibility**: once this works, you should not
have to rediscover the same toolchain and dependency issues again.

This project was developed and tested on Windows 10 using a conda environment.

---

## What this repo contains

* `main.cpp`
  Minimal program that includes AimIO headers and links successfully.

* `CMakeLists.txt`
  Consumer-side CMake that finds AimIO, Boost, and n88util via `find_package`
  and builds an executable.

This repo **does not** vendor Boost, n88util, or AimIO itself. Boost and n88util is built separately and
installed into a conda environment. AimIO is built within this project.

---

## Prerequisites

### 1. Visual Studio (MSVC x64)

Install **Visual Studio Community** with:

* Desktop development with C++
* MSVC v143 toolset (or newer)

You must use the **x64 toolchain**.
Do NOT use x86.

### 2. conda (Anaconda or Miniconda)

Ensure `conda` is available in your shell.

### 3. Git

Git for Windows (with Git Credential Manager enabled).

---

## Create the conda environment

Example environment (name used here: `aimio-rel`):

```
conda create -n aimio-rel -y -c conda-forge cmake ninja boost boost-cpp
conda activate aimio-rel
```

Install `n88util` and `aimio` dependencies from numerics88:

```
conda install -y -c numerics88 n88util
```

Headers:
<env>\include and <env>\Library\include

CMake package configs:
<env>\CMake and <env>\Library\CMake

---

## Build and install AimIO (from source)

AimIO must be built from source and installed into the same conda environment.

Example directory layout:

```
C:\COMMS\
  com_meas\
  external\
    AimIO\
```

### Open the correct shell

Use **Developer PowerShell for VS** (NOT PowerShell x86).

Confirm x64 compiler:

```
where.exe cl
```

You should see a path containing:
Hostx64\x64\cl.exe

### Clone AimIO:

```
cd C:\COMMS\com_meas\external
git clone https://github.com/Numerics88/AimIO.git
```

### Configure AimIO (release)

From the AimIO source directory:

```
conda activate aimio-rel

cmake -S . -B build-rel -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -Dn88util_DIR="$env:CONDA_PREFIX\Library\CMake" `
  -DBoost_DIR="$env:CONDA_PREFIX\Library\lib\cmake\Boost-1.78.0" `
  -DCMAKE_INSTALL_PREFIX="$env:CONDA_PREFIX" `
  -DN88_BUILD_AIX=OFF `
  -DN88_BUILD_CTHEADER=OFF
```

### Build and install (release)

```
conda activate aimio-rel

cmake --build build-rel
cmake --install build-rel
```

Verify install:

```
<env>\include\AimIO\AimIO.h
<env>\CMake\AimIOConfig.cmake
```

---

## Build com_meas

From the root of this repository:

```
conda activate aimio-rel

cmake -S . -B build-rel -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -Dn88util_DIR="$env:CONDA_PREFIX\Library\CMake" `
  -DBoost_DIR="$env:CONDA_PREFIX\Library\lib\cmake\Boost-1.78.0" `
  -DAimIO_DIR="$env:CONDA_PREFIX\CMake" `
  -DCMAKE_PREFIX_PATH="$env:CONDA_PREFIX;$env:CONDA_PREFIX\Library"

cmake --build build-rel
```

##Run com_meas
```
build-rel\com_meas.exe distal_bone.aim proximal_bone.aim
```

Expected output:

```
Read Aim 1 image data: completed

Read Aim 1 image header: completed

Read Aim 2 image data: completed

Read Aim 2 image header: completed

The distance between the center of masses is : 4.65537
The inclination with respect to x: 134.91 y: 73.6584 z: 49.4602
File opened successfully.
File closed successfully.
```

---

## Important Windows notes

1. **x64 vs x86 matters**
   Mixing x86 and x64 will cause Boost and AimIO link failures.

2. **Boost headers are NOT in `<env>/include`**
   They are in: <env>/Library/include

   This is why the CMakeLists explicitly adds both include paths.

3. **AimIO does not automatically pull transitive dependencies**
   Consumer projects must explicitly call:

   ```
   find_package(n88util CONFIG REQUIRED)
   find_package(Boost CONFIG REQUIRED COMPONENTS filesystem system)
   ```

   before `find_package(AimIO)`.

4. **Debug vs Release**
   Be consistent. If AimIO is installed as Debug, your app must also be Debug.

---

## CMakeLists.txt (consumer)

The current working CMakeLists.txt for this project is:

```
cmake_minimum_required(VERSION 3.20)
project(com_meas LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(n88util CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS filesystem system)
find_package(AimIO CONFIG REQUIRED)

add_executable(com_meas src/main.cpp)
target_link_libraries(com_meas PRIVATE AimIO::AimIO)

target_include_directories(com_meas PRIVATE
    $ENV{CONDA_PREFIX}/include
    $ENV{CONDA_PREFIX}/Library/include
)
```

---

## Why this repo exists

Rebuilding existing C++ code from OpenVMS on Windows using:

MSVC

CMake

Boost

conda

third-party scientific libraries

is far harder than it should be.

This repository exists as a known-good reference for:

Release-first builds

AimIO integration

Windows + conda + MSVC interoperability

avoiding Debug/Release and x86/x64 traps

If it prevents even one future rebuild from turning into a week-long ordeal, it has done its job.
