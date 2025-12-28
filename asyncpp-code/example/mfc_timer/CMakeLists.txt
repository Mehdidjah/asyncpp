cmake_minimum_required(VERSION 3.5)

project(mfc_timer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MFC_FLAG 1)

set(PROJECT_SOURCES
    Resource.h
    framework.h
    mfc_timer.cpp
    mfc_timer.h
    mfc_timerDlg.cpp
    mfc_timerDlg.h
    mfctimer.rc
    pch.cpp
    pch.h
    targetver.h
    res/mfc_timer.ico
    res/mfctimer.rc2

    ../../include/async-promise/promise.hpp
    ../../include/async-promise/promise_implementation.hpp
    ../../include/async-promise/any_type.hpp
    ../../include/async-promise/extensions.hpp
    ../../include/async-promise/call_traits.hpp
    ../../extensions/windows/promise_windows.hpp
)

add_executable(mfc_timer WIN32 ${PROJECT_SOURCES})
target_compile_definitions(mfc_timer PRIVATE _AFXDLL)
target_link_libraries(mfc_timer PRIVATE promise)

