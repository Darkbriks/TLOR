# TLOR

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
