# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/components/uart_drivers/uart_pzem004t
    ${CMAKE_CURRENT_LIST_DIR}/components/energy_module
    ${CMAKE_CURRENT_LIST_DIR}/components/i2c_drivers/i2c_mgmt_driver 
    ${CMAKE_CURRENT_LIST_DIR}/components/i2c_drivers/i2c_ads1115 
    )
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(SMEM-MP)
