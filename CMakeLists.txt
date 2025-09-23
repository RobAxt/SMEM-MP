# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS components/i2c_drivers/i2c_mgmt_driver 
                         components/i2c_drivers/i2c_ads1115 
                         components/i2c_drivers/i2c_mcp23017 
                         components/i2c_drivers/i2c_pn532
                         components/ao_core
                         components/security_module
                         )
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(SMEM-MP)
