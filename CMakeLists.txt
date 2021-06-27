cmake_minimum_required(VERSION 2.8)

project(cpp-psvr-encoder)

find_library(AVFORMAT_LIBRARY avformat)
find_library(AVCODEC_LIBRARY avcodec)
find_package(Boost REQUIRED COMPONENTS filesystem)

if (NOT AVFORMAT_LIBRARY)
    message(FATAL_ERROR "Couldn't find avformat")
endif()

add_executable(${PROJECT_NAME} "main.cpp")

target_link_libraries(${PROJECT_NAME} PUBLIC ${AVFORMAT_LIBRARY} ${AVCODEC_LIBRARY} ${Boost_LIBRARIES})
