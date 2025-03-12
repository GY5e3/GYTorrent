cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)

project(test)

find_package(Boost COMPONENTS ALL)
find_package(OpenSSL REQUIRED)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/utils)

file(GLOB SOURCES
    "source/*.cpp"
    "source/**/*.cpp")

add_executable(${PROJECT_NAME}
    ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE Boost::boost boost_coroutine OpenSSL::SSL OpenSSL::Crypto)
