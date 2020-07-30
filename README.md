# Build instructions


## Windows Build Directions

From the root of project directory:
```sh
mkdir build
cd build
```

####  You can build project with Visual Studio 2019:
**Visual Studio 2019**:
```sh
# (also you can set Release instead Debug)
cmake .. -G "Visual Studio 16 2019" -A Win32 -DCMAKE_BUILD_TYPE=Release

MSBuild.exe icq.sln /m /t:Rebuild /p:Configuration=Release /p:Platform=Win32 /nodeReuse:false
```
Or open `build\icq.sln` and build it.

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
make -j$(nproc)
```

## Linux Build Directions
In order to build ICQ execute the following command line:
```sh
# change -DLINUX_ARCH=64 to -DLINUX_ARCH=32 for 32bit binaries
mkdir build && cd build
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DLINUX_ARCH=64 && make -j$(nproc)
```

