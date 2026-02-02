cmake_minimum_required(VERSION 3.20)
project(t3)

set(CMAKE_CXX_STANDARD 17)

add_executable(t3
        main.cpp
        HashBasedEventDispatcher.h
        FunctionTraits.h
        TypeHash.h
        CollectBaseHashes.h
        EventDispatcher.h
        EventDispatcher2.h
        ArrayView.h)
