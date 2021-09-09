#  Copyright (c) Microsoft Corporation. All rights reserved.
#  Licensed under the MIT License.

cmake_minimum_required (VERSION 3.10)

project (MyProj_1.1 C)

azsphere_configure_tools(TOOLS_REVISION "21.04")
azsphere_configure_api(TARGET_API_SET "9")

# Create executable
include_directories ("${PROJECT_NAME}/MyLib") 
include_directories(${CMAKE_SOURCE_DIR} VL53L1X) 
add_subdirectory ("lib" out) 

add_executable (${PROJECT_NAME} 
    main.c 
    VL53L1X/VL53L1X.c
    VL53L1X/i2c.c
    VL53L1X/utils.c
) 
target_link_libraries (${PROJECT_NAME} applibs pthread gcc_s c)
target_link_libraries (${PROJECT_NAME} azureiot pthread gcc_s c)
target_compile_definitions(${PROJECT_NAME} PUBLIC AZURE_IOT_HUB_CONFIGURED)
# target_link_libraries(${PROJECT_NAME} applibs pthread gcc_s c azsphere_libs )

target_include_directories(${PROJECT_NAME} PUBLIC lib )

target_link_libraries(${PROJECT_NAME} MyLib) 

target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unknown-pragmas)
 

azsphere_target_hardware_definition(${PROJECT_NAME} TARGET_DIRECTORY "HardwareDefinitions/mt3620_rdb" 
            TARGET_DEFINITION "mt3620_rdb.json") 

azsphere_target_add_image_package(${PROJECT_NAME})
