cmake_minimum_required(VERSION 3.20)
project(t3)

set(CMAKE_CXX_STANDARD 17)

add_executable(main
        main.cpp
        HashBasedEventDispatcher2.cpp
        HashBasedEventDispatcher4.cpp
        HashBasedEventDispatcher3.cpp)

add_executable(test
        ut.cpp
        HashBasedEventDispatcher4.cpp)
