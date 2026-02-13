cmake_minimum_required(VERSION 3.13.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(EdgeAI_nRF)

zephyr_compile_options(-fdiagnostics-color=always)

add_definitions(
    -DEI_CLASSIFIER_TFLITE_ENABLE_CMSIS_NN=1
    -DARM_MATH_LOOPUNROLL
    -DEI_NORDIC
)

add_subdirectory(third_party/edge_impulse)

target_sources(app PRIVATE
    src/main.cpp
    src/mpu6050/mpu6050.cpp
    src/mqtt/mqtt.c
)

target_include_directories(app PRIVATE
    src
    src/mpu6050
    src/mqtt
    third_party/edge_impulse
)

add_definitions(
    -DSB_CONFIG_WIFI_NRF70=1
    -DSB_CONFIG_MQTT_HELPER=1
    -DCONFIG_WIFI=1
    -DCONFIG_WIFI_NM=1
    -DCONFIG_MQTT_HELPER=1
    -DCONFIG_NET_CONNECTION_MANAGER=1
)

