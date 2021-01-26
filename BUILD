# pip install conan

mkdir build
cd build
conan profile new default --detect
conan profile update settings.compiler.libcxx=libstdc++11 default
conan install .. --build
cmake ..
cmake --build .
ls bin/
