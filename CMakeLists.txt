cmake_minimum_required(VERSION 3.17)
project(cpp_2020_word_counting)

set(CMAKE_CXX_STANDARD 17)

link_libraries(stdc++fs)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED COMPONENTS filesystem locale system)

find_package(LibArchive REQUIRED)


add_executable(cpp_2020_word_counting
        main.cpp
        src/archive_wrapper.cpp
        src/archive_wrapper.h
        src/text_utils.cpp
        src/text_utils.h
        )
target_link_libraries(cpp_2020_word_counting ${Boost_LIBRARIES})
target_link_libraries(cpp_2020_word_counting archive)
