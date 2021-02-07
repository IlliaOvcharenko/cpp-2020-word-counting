cmake_minimum_required(VERSION 3.17)
project(cpp_2020_word_counting)

set(CMAKE_CXX_STANDARD 17)

link_libraries(stdc++fs)
set(THREADS_PREFER_PTHREAD_FLAG ON)

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
        src/thread_safe_queue.h
        src/time_measurement.h
        src/logger.h
        src/stages.cpp
        src/stages.h
        src/output_utils.cpp
        src/output_utils.h
)
target_link_libraries(cpp_2020_word_counting ${Boost_LIBRARIES})
target_link_libraries(cpp_2020_word_counting archive)


add_executable(
        dev_queue
        dev_queue.cpp

        src/thread_safe_queue.h
)
target_link_libraries(dev_queue PRIVATE Threads::Threads)


add_executable(
        sandbox
        sandbox.cpp
)

