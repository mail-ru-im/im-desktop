# Build instructions

## Requirements

**[Download external libraries](https://hb.bizmrg.com/icq-www/external/external.7z)** and extract it to `./external` folder.
**[Download voip libraries](https://icq-www.hb.bizmrg.com/external/voip.zip)** and extract it to `./core/Voip/libvoip` folder.


## Windows Build Directions

From the root of project directory:
```sh
mkdir build
cd build
```

####  You can build project with Visual Studio 2017 or with NMake:
**Visual Studio 2017**:
```sh
# (also you can set Release instead Debug)
cmake .. -G "Visual Studio 15 2017" -T "v141_xp" -DCMAKE_BUILD_TYPE=Debug
```
Open `build\icq.sln` and build it.

**NMake**:
```sh
# (also you can set Release instead Debug)
cmake .. -G "NMake Makefiles" -T "v141_xp" -DCMAKE_BUILD_TYPE=Debug
nmake
```

## macOS Build Directions
From the root of project directory:
```sh
mkdir build
cd build
```

#### You can build project with Xcode or with make:
**Xcode**:
```sh
# (also you can set Release instead Debug)
cmake .. -G Xcode -DCMAKE_BUILD_TYPE=Debug
```
Open `build\icq.xcodeproj` and build it.

**make**:
```sh
# (also you can set Release instead Debug)
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

## Linux Build Directions
In order to build ICQ execute the following command line:
```sh
# change -DLINUX_ARCH=64 to -DLINUX_ARCH=32 for 32bit binaries
mkdir build && cd build
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DLINUX_ARCH=64 && make
```

