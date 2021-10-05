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
cmake .. -DIM_QT_DYNAMIC=ON -G "Visual Studio 16 2019" -A Win32 -DCMAKE_BUILD_TYPE=Release

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
cmake .. -DIM_QT_DYNAMIC=ON -G Xcode -DCMAKE_BUILD_TYPE=Release
```
Open `build\icq.xcodeproj` and build it.

**make**:
```sh
cmake .. -DIM_QT_DYNAMIC=ON -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Linux Build Directions
In order to build ICQ execute the following command line:
```sh
mkdir build && cd build
cmake .. -DIM_QT_DYNAMIC=ON -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DLINUX_ARCH=64 && make -j$(nproc)
```

