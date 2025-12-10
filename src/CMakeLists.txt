add_library(mynet mynet/make_srvsock.cpp mynet/handles.cpp mynet/io_funcs.cpp)
target_include_directories(mynet PUBLIC ${MY_HEADER_DIR})

add_library(myhttp myhttp/enums.cpp myhttp/intake.cpp myhttp/outtake.cpp)
target_include_directories(myhttp PUBLIC ${MY_HEADER_DIR})

add_library(myuri myuri/uri.cpp myuri/parse.cpp)
target_include_directories(myuri PUBLIC ${MY_HEADER_DIR})

add_library(myapp myapp/contents.cpp myapp/routes.cpp)
target_include_directories(myapp PUBLIC ${MY_HEADER_DIR})

add_executable(derkhttpd main.cpp)
target_include_directories(derkhttpd PUBLIC ${MY_HEADER_DIR})

# For working around a regression within Homebrew LLVM 21.1.7 installs under Clang: (issue 235411)
if (APPLE AND LLVM_LIBRARY_DIR)
    target_link_directories(derkhttpd PUBLIC "${LLVM_LIBRARY_DIR}/c++" PUBLIC ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
else ()
    target_link_directories(derkhttpd PUBLIC "${LLVM_LIBRARY_DIR}/c++" PUBLIC ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    message(WARNING "Failed to find LLVM 21 libc++ by LLVM_LIBRARY_DIR!")
endif ()

target_link_libraries(derkhttpd PRIVATE mynet PRIVATE myhttp PRIVATE myuri PRIVATE myapp)
