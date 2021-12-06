cmake_minimum_required(VERSION 3.16)
project(game_engine)

add_subdirectory(glfw)
add_subdirectory(glm)
set(PHYSX_PATH ${PROJECT_SOURCE_DIR}/physx)
set(PHYSX_ROOT_DIR ${PHYSX_PATH}/physx)
set(PHYSX_INCLUDE_DIRS ${PHYSX_PATH}/physx/include/ ${PHYSX_PATH}/pxshared/include/)
set(PHYSX_LIBRARIES
        PhysXExtensions
        PhysX
        PhysXPvdSDK
        PhysXVehicle
        PhysXCharacterKinematic
        PhysXCooking
        PhysXCommon
        PhysXFoundation
        )
if (WIN32)
    set(TARGET_BUILD_PLATFORM "windows")
endif ()
set(PX_BUILDSNIPPETS OFF CACHE BOOL "Generate the snippets")
set(PX_BUILDPUBLICSAMPLES OFF CACHE BOOL "Generate the samples projects")
set(PX_GENERATE_STATIC_LIBRARIES ON CACHE BOOL "Generate static libraries")
set(NV_USE_STATIC_WINCRT ON CACHE BOOL "Use the statically linked windows CRT")
set(NV_USE_DEBUG_WINCRT ON CACHE BOOL "Use the debug version of the CRT")
set(PXSHARED_PATH ${PHYSX_PATH}/pxshared)
set(PXSHARED_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CMAKEMODULES_PATH ${PHYSX_PATH}/externals/cmakemodules)
set(PX_OUTPUT_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/externals/physx)
set(PX_OUTPUT_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/externals/physx)
add_subdirectory(${PHYSX_PATH}/physx/compiler/public ${PHYSX_PATH}/externals/physx)

set(CMAKE_CXX_STANDARD 20)

add_executable(game_engine main.cpp)
target_link_libraries(game_engine glfw ${PHYSX_LIBRARIES})
