# cmake -DBOOST_ROOT=path_to_boost_include -DBOOST_LIBRARYDIR=path_to_boost_lib -DCMAKE_BUILD_TYPE=Release|Debug .


cmake_minimum_required(VERSION 3.12)

# make Release as default
if (NOT EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt)
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
    endif()
endif()

# C++20 standard for 2025
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(async-promise
    VERSION 3.0.0
    DESCRIPTION "Modern C++20 Promise/A+ library"
    LANGUAGES CXX
)

# build shared option
option(PROMISE_BUILD_SHARED "Build shared library" OFF)
option(PROMISE_BUILD_EXAMPLES "Build examples" ON)

set(my_headers
    include/async-promise/promise.hpp
    include/async-promise/promise_implementation.hpp
    include/async-promise/any_type.hpp
    include/async-promise/extensions.hpp
    include/async-promise/call_traits.hpp
)

set(my_sources
    src/promise.cpp
)

if(PROMISE_BUILD_SHARED OR BUILD_SHARED_LIBS)
    add_definitions(-DPROMISE_BUILD_SHARED)
    add_library(async-promise SHARED ${my_sources} ${my_headers})
else()
    add_library(async-promise STATIC ${my_sources} ${my_headers})
endif()

target_include_directories(async-promise PUBLIC include .)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets)
if(NOT QT_FOUND)
    message(WARNING "QT not found, so project qt_timer will not be compiled")
else()
    if(PROMISE_BUILD_SHARED OR BUILD_SHARED_LIBS)
        add_library(promise_qt SHARED ./extensions/qt/promise_qt.cpp ${my_headers})
    else()
        add_library(promise_qt STATIC ./extensions/qt/promise_qt.cpp ${my_headers})
    endif()
    target_link_libraries(promise_qt PRIVATE async-promise Qt${QT_VERSION_MAJOR}::Widgets)
endif()

if(PROMISE_BUILD_EXAMPLES) 
    add_executable(test0 ${my_headers} example/test0.cpp)
    target_link_libraries(test0 PRIVATE async-promise)


    find_package(Threads)
    if(Threads_FOUND)
        add_executable(simple_timer ${my_headers} example/simple_timer.cpp)
        target_link_libraries(simple_timer PRIVATE async-promise Threads::Threads)

        add_executable(simple_benchmark_test ${my_headers} example/simple_benchmark_test.cpp)
        target_link_libraries(simple_benchmark_test PRIVATE async-promise Threads::Threads)
    
        add_executable(multithread_test ${my_headers} example/multithread_test.cpp)
        target_link_libraries(multithread_test PRIVATE async-promise Threads::Threads)
    endif()

    add_executable(chain_defer_test ${my_headers} example/chain_defer_test.cpp)
    target_link_libraries(chain_defer_test PRIVATE async-promise)


    if(QT_FOUND)
        add_subdirectory(./example/qt_timer)
    endif()

    find_package(MFC)
    if(NOT MFC_FOUND)
        message(WARNING "MFC not found, so project mfc_timer will not be compiled")
    else()
        add_subdirectory(./example/mfc_timer)
    endif()
endif()
