cmake_minimum_required(VERSION 4.2)

project(derkhttpd CXX)

if (NOT DEFINED COMPILE_WARNING_AS_ERROR OR NOT DEFINED CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "COMPILE_WARNING_AS_ERROR or CMAKE_BUILD_TYPE is undefined in the given preset. Available presets include: local-debug-build, local-release-build")
endif ()

add_subdirectory(src)
# enable_testing()
# add_subdirectory(tests)
