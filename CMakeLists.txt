# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
# CMakeLists.txt (raíz)
cmake_minimum_required(VERSION 3.16)

# Solo los “anidados” debajo de components/network_drivers
set(EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/components/network_drivers/mqtt_driver
    ${CMAKE_CURRENT_LIST_DIR}/components/network_drivers/net_driver
    ${CMAKE_CURRENT_LIST_DIR}/components/network_drivers/sntp_driver
    ${CMAKE_CURRENT_LIST_DIR}/components/communication_module
    ${CMAKE_CURRENT_LIST_DIR}/components/i2c_drivers/i2c_mgmt_driver 
    ${CMAKE_CURRENT_LIST_DIR}/components/i2c_drivers/i2c_ads1115 
    ${CMAKE_CURRENT_LIST_DIR}/components/i2c_drivers/i2c_mcp23017 
    ${CMAKE_CURRENT_LIST_DIR}/components/i2c_drivers/i2c_pn532
    ${CMAKE_CURRENT_LIST_DIR}/components/ao_core
    ${CMAKE_CURRENT_LIST_DIR}/components/security_module
    )
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(SMEM-MP)
