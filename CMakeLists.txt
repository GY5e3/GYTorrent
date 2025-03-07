cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)

project(MyProject)

find_package(Boost COMPONENTS ALL)
find_package(LibtorrentRasterbar CONFIG REQUIRED)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/Abstraction
    ${PROJECT_SOURCE_DIR}/utils)

add_executable(MyProject
    source/main.cpp
    source/TorrentMetaData.cpp
    source/TrackerHTTP.cpp
    source/TrackerUDP.cpp
    source/AnnounceResponseHTTP.cpp
    source/AnnounceResponseUDP.cpp)

target_link_libraries(MyProject PRIVATE Boost::boost boost_coroutine LibtorrentRasterbar::torrent-rasterbar)
