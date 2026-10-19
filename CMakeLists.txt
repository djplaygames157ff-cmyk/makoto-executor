cmake_minimum_required(VERSION 3.20)
project(ZeusExecutor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build as DLL
add_library(ZeusExecutor SHARED
    executor.cpp
    injector.cpp
    evasion.cpp
    ui.cpp
    lua_core.cpp
    dllmain.cpp
)

target_include_directories(ZeusExecutor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lua
)

target_compile_definitions(ZeusExecutor PRIVATE
    -DUNICODE
    -D_UNICODE
    -DNOMINMAX
    -D_CRT_SECURE_NO_WARNINGS
)

# Lua 5.4 static
add_subdirectory(lua)

target_link_libraries(ZeusExecutor PRIVATE
    lua54
    d3d11
    dxgi
    psapi
    ws2_32
)

# Set DLL properties
set_target_properties(ZeusExecutor PROPERTIES
    OUTPUT_NAME "ZeusExecutor"
    PREFIX ""
    SUFFIX ".dll"
    LINK_FLAGS "/NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcmtd.lib"
    STATIC_RUNTIME ON
)

# Hide exports by default
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(ZeusExecutor PRIVATE /O2 /GL /Gy)
    target_link_options(ZeusExecutor PRIVATE /LTCG /OPT:REF /OPT:ICF)
endif()
