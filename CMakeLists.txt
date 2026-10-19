cmake_minimum_required(VERSION 3.20)
project(MakotoExecutor VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(MakotoExecutor SHARED minimal_dll.cpp)

target_compile_definitions(MakotoExecutor PRIVATE
    UNICODE
    _UNICODE
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

if(MSVC)
    target_compile_options(MakotoExecutor PRIVATE /O1 /Os /GL /Gy /Gw /Zc:inline /EHsc /MT /Gd)
    target_link_options(MakotoExecutor PRIVATE /LTCG /OPT:REF /OPT:ICF /MERGE:.rdata=.text /MERGE:.pdata=.text /RELEASE /DEBUG:NONE /NODEFAULTLIB:libcmt.lib /NODEFAULTLIB:libcmtd.lib)
endif()

set_target_properties(MakotoExecutor PROPERTIES
    OUTPUT_NAME "MakotoExecutor"
    PREFIX ""
    SUFFIX ".dll"
)
