# aeffects-conf2xml

## Dependencies

__Debian:__
```bash
sudo apt install cmake g++ libtinyxml2-dev
```

__Fedora__
```bash
sudo dnf install cmake g++ tinyxml2-devel
```

## Compiling
```bash
mkdir cmake-build-release
cd cmake-build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

## Usage
```bash
./aeffects-conf2xml /path/to/audio_effects.conf /path/to/audio_effects.xml
```
