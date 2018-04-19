echo "Adding MSYS2 to path..."
SET "PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;%PATH%"
echo %PATH%

echo "Installing MSYS2 packages..."
bash -lc "pacman -S --needed --noconfirm mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-doxygen mingw-w64-x86_64-protobuf"

echo "Generating makefiles"
mkdir build
cd build
cmake .. -LA -G "MSYS Makefiles"

echo "Building"
make VERBOSE=1

echo "Testing"
ctest --output-on-failure

