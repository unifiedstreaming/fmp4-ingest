cmake_minimum_required (VERSION 2.8)
project (fmp4ingest)
include_directories("${PROJECT_SOURCE_DIR}")
link_directories("${PROJECT_SOURCE_DIR}")

if(UNIX)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)

    if(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
endif()

#add_library (fmp4stream fmp4stream.cpp fmp4stream.h)
add_executable(fmp4ingest fmp4ingest.cpp fmp4stream.cpp fmp4stream.h base64.cpp base64.h)
target_link_libraries(fmp4ingest ${CURL_LIBRARIES})

if($ENV{CURL_LIBRARY_DIR})
link_directories(fmp4ingest $ENV{CURL_LIBRARY_DIR})
endif()

if(UNIX)
if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(fmp4ingest PUBLIC "-pthread")
endif()
endif()

if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(fmp4ingest "${CMAKE_THREAD_LIBS_INIT}")
endif()

add_executable(fmp4dump fmp4dump.cpp fmp4stream.cpp fmp4stream.h base64.cpp base64.h)
add_executable(fmp4sparse fmp4sparse.cpp fmp4stream.cpp fmp4stream.h base64.cpp base64.h)
add_executable(fmp4Init fmp4init.cpp fmp4stream.cpp fmp4stream.h base64.cpp base64.h)
add_executable(fmp4DashEvent fmp4DashEvent.cpp fmp4stream.cpp fmp4stream.h base64.cpp base64.h)
