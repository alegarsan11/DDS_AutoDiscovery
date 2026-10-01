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

mkdir -p build && cd build

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=~/fastdds_ws/install \
    -DCMAKE_PREFIX_PATH=~/fastdds_ws/install \
    -DBUILD_SHARED_LIBS=ON \
    -DCOMPILE_TOOLS=ON

cmake --build . --target install -j$(nproc)

cd build

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

## Resiliencia de Red y Enlaces Inestables (Radio 2.4 GHz)

###  Retos Encontrados
* **Binding a `127.0.0.1` al arrancar sin red:** Fast DDS escanea las interfaces de red únicamente al llamar a `create_participant()`. Si los nodos arrancan sin enlace activo o con la tarjeta física sin IP, Fast DDS se vincula únicamente a Loopback (`127.0.0.1`) y no detecta cuando la interfaz física sube más tarde.
* **Desconexiones en caliente y microcortes:** En enlaces radio inestables (2.4 GHz), las caídas físicas de la interfaz dejan los sockets UDP de DDS en un estado huérfano sin capacidad de auto-recuperación por sí solos.
* **Confusión de puertos en Unicast:** El puerto `7400` corresponde al canal Multicast PDP por defecto, mientras que el metatráfico Unicast requiere el puerto `7410` (para el Participante 0 en Dominio 0).

### Solución Implementada (Ciclo de Vida Dinámico)
Se implementó un patrón de **gestión dinámica del ciclo de vida de Fast DDS** acoplado al estado de la red mediante inspección del Kernel (POSIX `ifaddrs`):

1. **Comprobación de Interfaz Activa (`is_network_ready()`):** El proceso no inicializa el `DomainParticipant` hasta verificar que la interfaz física de red tiene carrier y una IP asignada válida.
2. **Destrucción Limpia en Caídas:** Si la red se cae durante la ejecución, el software detecta la pérdida de conectividad, destruye ordenadamente las entidades DDS (`delete_contained_entities()`) y entra en un bucle de espera ligero.
3. **Re-inicialización Automática:** Al reconectar la radio o restaurar el enlace, el participante se vuelve a crear desde cero sobre la interfaz restaurada, logrando un autodescubrimiento e interconexión inmediatos.
4. **Discovery Agresivo:** Se ajustó la política QoS de descubrimiento a un periodo de anuncio de 1 segundo y un lease duration de 5 segundos para minimizar la latencia de reconexión.