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
target_link_directories(derkhttpd PRIVATE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
target_link_libraries(derkhttpd PRIVATE mynet PRIVATE myhttp PRIVATE myuri PRIVATE myapp)
