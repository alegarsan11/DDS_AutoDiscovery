cmake_minimum_required(VERSION 3.16)
project(DDS_AutoDiscovery CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Buscar Fast DDS
find_package(fastdds 3 REQUIRED)

# Buscar fastddsgen en el sistema
find_program(FASTDDSGEN_BIN fastddsgen)
if(NOT FASTDDSGEN_BIN)
    message(FATAL_ERROR "fastddsgen no encontrado.")
endif()

set(IDL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/HelloWorld.idl")

# Archivos .cxx que genera fastddsgen
set(GENERATED_SOURCES
    "${CMAKE_CURRENT_BINARY_DIR}/HelloWorldPubSubTypes.cxx"
    "${CMAKE_CURRENT_BINARY_DIR}/HelloWorldTypeObjectSupport.cxx"
)

# Cabeceras generadas (.hpp / .ipp)
set(GENERATED_HEADERS
    "${CMAKE_CURRENT_BINARY_DIR}/HelloWorld.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/HelloWorldCdrAux.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/HelloWorldCdrAux.ipp"
    "${CMAKE_CURRENT_BINARY_DIR}/HelloWorldPubSubTypes.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/HelloWorldTypeObjectSupport.hpp"
)

# Comando para invocar fastddsgen
add_custom_command(
    OUTPUT ${GENERATED_SOURCES} ${GENERATED_HEADERS}
    COMMAND ${FASTDDSGEN_BIN} -replace -d ${CMAKE_CURRENT_BINARY_DIR} ${IDL_FILE}
    DEPENDS ${IDL_FILE}
    COMMENT "Generando fuentes DDS desde ${IDL_FILE}"
)

# --- 1. Ejecutable Publicador ---
add_executable(publisher_node main.cpp ${GENERATED_SOURCES})
target_include_directories(publisher_node PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(publisher_node PRIVATE fastdds)

# --- 2. Ejecutable Suscriptor ---
add_executable(subscriber_node subscriber.cpp ${GENERATED_SOURCES})
target_include_directories(subscriber_node PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(subscriber_node PRIVATE fastdds)