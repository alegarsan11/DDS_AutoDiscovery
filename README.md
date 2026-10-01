# 1. Install FastDDS

sudo pacman -Syu --needed base-devel cmake git python-pip asio tinyxml2 openssl

pip install --break-system-packages vcstool

mkdir -p ~/fastdds_ws/src

cd ~/fastdds_ws/src

### 1. Fast CDR
git clone https://github.com/eProsima/Fast-CDR.git

### 2. Foonathan Memory Vendor
git clone https://github.com/eProsima/foonathan_memory_vendor.git

### 3. Fast DDS
git clone https://github.com/eProsima/Fast-DDS.git

cd ~/fastdds_ws
mkdir -p build 

cd ~/fastdds_ws/src/foonathan_memory_vendor
mkdir -p build && cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=~/fastdds_ws/install \
    -DBUILD_SHARED_LIBS=ON

cmake --build . --target install -j$(nproc)

cd ~/fastdds_ws/src/Fast-CDR
mkdir -p build && cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=~/fastdds_ws/install \
    -DBUILD_SHARED_LIBS=ON

cmake --build . --target install -j$(nproc)

cd ~/fastdds_ws/src/Fast-DDS

cmake ../src \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=~/fastdds_ws/install \
    -DBUILD_SHARED_LIBS=ON \
    -DCOMPILE_TOOLS=ON

make -j$(nproc)



# Rutas para Fast DDS instalado en local
export CMAKE_PREFIX_PATH=~/fastdds_ws/install:$CMAKE_PREFIX_PATH

export LD_LIBRARY_PATH=~/fastdds_ws/install/lib:$LD_LIBRARY_PATH
# Para usar con Arch Linux y consola fish
set -gx CMAKE_PREFIX_PATH ~/fastdds_ws/install $CMAKE_PREFIX_PATH

set -gx LD_LIBRARY_PATH ~/fastdds_ws/install/lib $LD_LIBRARY_PATH

# Necesitas previamente fastddsgen para generar el IDL

 mkdir build && cd build
      cmake ..
      make
