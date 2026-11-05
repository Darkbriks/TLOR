---
title: About TLOR project
description: About TLOR project
sidebar:
  order: 0
---

# TLOR

## Dependencies by platform

### Linux

**C++ application:**

- CMake 3.29 or newer
- Ninja
- Clang (`clang` and `clang++`) with C++23 support

**Python map tools:**

- Python 3.9 or newer
- `pip` and `venv`
- `python3-tk` for the offline map
- Install the Python packages from `Maps/offline map/requirements_offline.txt`
  and/or `Maps/online map/requirements_online.txt`

### macOS

**C++ application:**

- Xcode Command Line Tools
- CMake 3.29 or newer
- Ninja
- LLVM/Clang with C++23 support

**Python map tools:**

- Python 3.9 or newer
- `pip` and `venv`
- Tk support for the offline map
- Install the Python packages from `Maps/offline map/requirements_offline.txt`
  and/or `Maps/online map/requirements_online.txt`

### Windows

**C++ application:**

- CMake 3.29 or newer
- Visual Studio 2022 with the **Desktop development with C++** workload
  (MSVC and C++23 support)
- PowerShell to run the commands below

**Python map tools:**

- Python 3.9 or newer
- `pip` and `venv`
- Install the Python packages from `Maps\offline map\requirements_offline.txt`
  and/or `Maps\online map\requirements_online.txt`

The Python dependencies can be installed from the repository root with:

```bash
python -m pip install -r "Maps/offline map/requirements_offline.txt"
python -m pip install -r "Maps/online map/requirements_online.txt"
```

## Build and run

### Linux
```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
./build/linux-debug/bin/TLOR
```

### macOS
```bash
cmake --preset macos-debug
cmake --build --preset macos-debug
./build/macos-debug/bin/TLOR
```

### Windows (PowerShell)
```powershell
cmake --preset windows
cmake --build --preset windows-debug
.\build\windows\bin\Debug\TLOR.exe
```

## Basic Git commands (CLI)

```bash
# Clone the repository (ssh, recommended)
git clone git@github.com:Darkbriks/TLOR.git
cd TLOR

# Clone the repository (https, if you can, prefer ssh)
git clone https://github.com/Darkbriks/TLOR.git
cd TLOR

# Check current status
git status

# Get latest changes from remote
git pull

# Create and switch to a new branch
git checkout -b <branch-name>

# Stage and commit changes
git add .
git commit -m "Describe your change"

# Push main branch to remote
git push origin main

# Push branch to remote
git push -u origin <branch-name>
```
