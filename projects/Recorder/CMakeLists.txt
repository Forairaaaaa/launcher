cmake_minimum_required(VERSION 3.16)

project(M5CardputerZeroRecorder LANGUAGES C CXX ASM)

option(RECORDER_USE_SDL "Use LVGL's SDL backend for desktop testing" ON)
option(RECORDER_CONVERT_ASSETS "Convert image assets during CMake configure" ON)
set(RECORDER_BIN_NAME "M5CardputerZero-Recorder" CACHE STRING "Output executable name")

if(NOT DEFINED RECORDER_USE_PULSEAUDIO)
    if(RECORDER_USE_SDL)
        set(RECORDER_USE_PULSEAUDIO OFF)
    else()
        set(RECORDER_USE_PULSEAUDIO ON)
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(RECORDER_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(RECORDER_SRC_DIR "${RECORDER_ROOT}/src")
set(RECORDER_DEPS_DIR "${RECORDER_ROOT}/dependencies")
set(RECORDER_LVGL_DIR "${RECORDER_DEPS_DIR}/lvgl")
set(RECORDER_SPDLOG_DIR "${RECORDER_DEPS_DIR}/spdlog")
set(RECORDER_SMOOTH_UI_DIR "${RECORDER_DEPS_DIR}/smooth_ui_toolkit")
set(RECORDER_MINIAUDIO_DIR "${RECORDER_DEPS_DIR}/miniaudio")

if(NOT DEFINED Python3_EXECUTABLE)
    foreach(candidate IN ITEMS "${RECORDER_ROOT}/.venv/bin/python" "${RECORDER_ROOT}/../../.venv/bin/python")
        if(EXISTS "${candidate}")
            execute_process(
                COMMAND "${candidate}" -m pip --version
                RESULT_VARIABLE RECORDER_PYTHON_PIP_RESULT
                OUTPUT_QUIET
                ERROR_QUIET
            )
            if(RECORDER_PYTHON_PIP_RESULT EQUAL 0)
                set(Python3_EXECUTABLE "${candidate}")
                break()
            endif()
        endif()
    endforeach()
endif()

foreach(required_dir IN LISTS RECORDER_LVGL_DIR RECORDER_SPDLOG_DIR RECORDER_SMOOTH_UI_DIR RECORDER_MINIAUDIO_DIR)
    if(NOT EXISTS "${required_dir}")
        message(FATAL_ERROR "Missing dependency: ${required_dir}. Run ./bootstrap.sh first.")
    endif()
endforeach()

find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(Threads REQUIRED)

if(RECORDER_CONVERT_ASSETS)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${RECORDER_SRC_DIR}/assets/convert_images.py"
        WORKING_DIRECTORY "${RECORDER_ROOT}"
        RESULT_VARIABLE RECORDER_ASSET_RESULT
    )
    if(NOT RECORDER_ASSET_RESULT EQUAL 0)
        message(FATAL_ERROR "Image asset conversion failed")
    endif()
endif()

set(LV_BUILD_CONF_PATH "${RECORDER_ROOT}/lv_conf.h" CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)

set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)

set(SMOOTH_UI_TOOLKIT_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SMOOTH_UI_TOOLKIT_BUILD_TEST OFF CACHE BOOL "" FORCE)

add_subdirectory("${RECORDER_LVGL_DIR}" EXCLUDE_FROM_ALL)
add_subdirectory("${RECORDER_SPDLOG_DIR}" EXCLUDE_FROM_ALL)
add_subdirectory("${RECORDER_SMOOTH_UI_DIR}" EXCLUDE_FROM_ALL)

set(RECORDER_LVGL_POSIX_PATH "./")
if(NOT RECORDER_USE_SDL)
    set(RECORDER_LVGL_POSIX_PATH "/usr/share/Recorder/")
endif()

target_compile_definitions(lvgl PUBLIC
    RECORDER_LVGL_USE_SDL=$<IF:$<BOOL:${RECORDER_USE_SDL}>,1,0>
    RECORDER_LVGL_FS_POSIX_PATH="${RECORDER_LVGL_POSIX_PATH}"
)

if(RECORDER_USE_SDL)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(SDL2 QUIET sdl2)
    endif()

    if(SDL2_FOUND)
        target_include_directories(lvgl SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
        target_compile_options(lvgl PRIVATE ${SDL2_CFLAGS_OTHER})
        target_link_directories(lvgl PUBLIC ${SDL2_LIBRARY_DIRS})
        set(RECORDER_SDL_LIBS ${SDL2_LIBRARIES})
    else()
        find_package(SDL2 REQUIRED)
        target_include_directories(lvgl SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
        set(RECORDER_SDL_LIBS ${SDL2_LIBRARIES})
    endif()
endif()

file(GLOB RECORDER_CPP CONFIGURE_DEPENDS
    "${RECORDER_SRC_DIR}/main.cpp"
    "${RECORDER_SRC_DIR}/core/*.cpp"
    "${RECORDER_SRC_DIR}/hal/*.cpp"
    "${RECORDER_SRC_DIR}/input/*.cpp"
    "${RECORDER_SRC_DIR}/models/*.cpp"
    "${RECORDER_SRC_DIR}/view_models/*.cpp"
    "${RECORDER_SRC_DIR}/views/*.cpp"
)
file(GLOB_RECURSE RECORDER_ASSET_C CONFIGURE_DEPENDS
    "${RECORDER_SRC_DIR}/assets/res_c/*.c"
)

add_executable(${RECORDER_BIN_NAME}
    ${RECORDER_CPP}
    ${RECORDER_ASSET_C}
)

target_include_directories(${RECORDER_BIN_NAME} PRIVATE
    "${RECORDER_SRC_DIR}"
    "${RECORDER_SRC_DIR}/assets"
    "${RECORDER_MINIAUDIO_DIR}"
)

target_compile_definitions(${RECORDER_BIN_NAME} PRIVATE
    RECORDER_USE_PULSEAUDIO=$<IF:$<BOOL:${RECORDER_USE_PULSEAUDIO}>,1,0>
)

target_link_libraries(${RECORDER_BIN_NAME} PRIVATE
    lvgl::lvgl
    smooth_ui_toolkit
    spdlog::spdlog
    Threads::Threads
    ${RECORDER_SDL_LIBS}
    dl
    m
)

set_target_properties(${RECORDER_BIN_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${RECORDER_ROOT}/dist"
)
