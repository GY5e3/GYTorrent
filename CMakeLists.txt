cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

project(test)

find_package(Boost COMPONENTS ALL)
find_package(OpenSSL REQUIRED)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/utils)

add_executable(${PROJECT_NAME}
    source/main.cpp
    source/TorrentMetaData.cpp
    source/TrackerHTTP.cpp
    source/TrackerUDP.cpp
    source/AnnounceResponseHTTP.cpp
    source/AnnounceResponseUDP.cpp
    source/ConnectionManager.cpp
    source/PieceManager.cpp
    source/TorrentClient.cpp
    source/TrackerAnnouncer.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE Boost::boost boost_coroutine OpenSSL::SSL OpenSSL::Crypto)
