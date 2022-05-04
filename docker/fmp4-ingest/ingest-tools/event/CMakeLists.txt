cmake_minimum_required (VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
project (event_track)
include_directories("${PROJECT_SOURCE_DIR}")

if(CMAKE_COMPILER_IS_GNUCXX)
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)

    if(COMPILER_SUPPORTS_CXX11)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
endif()

add_executable(unittest unittest.cpp event_track.cpp fmp4stream.cpp catch.hpp base64.cpp event_track.h fmp4stream.h)
add_executable(generate_example generate_example.cpp event_track.cpp fmp4stream.cpp base64.cpp event_track.h fmp4stream.h)
add_executable(fmp4_dash_event fmp4_dash_event.cpp event_track.cpp fmp4stream.cpp base64.cpp event_track.h fmp4stream.h)
add_executable(dash_event_fmp4 dash_event_fmp4.cpp event_track.cpp fmp4stream.cpp base64.cpp tinyxml2.cpp event_track.h fmp4stream.h)
add_executable(gen_avail_track gen_avails.cpp event_track.cpp fmp4stream.cpp base64.cpp tinyxml2.cpp event_track.h fmp4stream.h)
add_executable(print_event_samples print_event_samples.cpp event_track.cpp fmp4stream.cpp base64.cpp tinyxml2.cpp event_track.h fmp4stream.h)
