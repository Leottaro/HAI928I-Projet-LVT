cmake_minimum_required(VERSION 3.20.0)
set(PROJECT_NAME projet3d)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(LIBS_DIR ${CMAKE_SOURCE_DIR}/libs)
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

set(APP_VERSION_MAJOR 1)
set(APP_VERSION_MINOR 0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_PREFIX_PATH ${QTDIR})
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
# set(OpenGL_GL_PREFERENCE GLVND) # Asked me to set this to GLVND or LEGACY
# set(CMAKE_VERBOSE_MAKEFILE 1) # If you want verbose

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(-fno-omit-frame-pointer)
    message(STATUS "keeping frame pointers")
endif()
if(UNIX AND NOT APPLE)
    add_compile_options(-Wall -Wextra -Wunused-result -pedantic) # -Werror -Wdeprecated-copy
endif()

add_definitions(-DQT_DEPRECATED_WARNINGS)

# ----------------------------------------------------------------------------------------------------
# -------------------------------------------   PROJECT   --------------------------------------------
# ----------------------------------------------------------------------------------------------------

project(${PROJECT_NAME})
file(GLOB_RECURSE APP_SOURCES "${SRC_DIR}/*.cpp" "${SRC_DIR}/*.c" "${INCLUDE_DIR}/*.hpp" "${INCLUDE_DIR}/*.h" "${INCLUDE_DIR}/*.tpp" "${INCLUDE_DIR}/*.t")
include_directories(${PROJECT_NAME} "${INCLUDE_DIR}")
add_executable(${PROJECT_NAME} ${APP_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

# -----------------------------------------------------------------------------------------------------
# --------------------------------------   INSTALLED LIBRARIES   --------------------------------------
# -----------------------------------------------------------------------------------------------------

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)

# -----------------------------------------------------------------------------------------------------
# ---------------------------------------   FETCHED LIBRARIES   ---------------------------------------
# -----------------------------------------------------------------------------------------------------
include(FetchContent)
set(FETCHCONTENT_BASE_DIR ${LIBS_DIR})

# FetchContent_Declare(
#     glm
#     GIT_REPOSITORY https://github.com/g-truc/glm.git
#     GIT_TAG 1.0.3
#     EXCLUDE_FROM_ALL
# )
# FetchContent_MakeAvailable(glm)
# target_link_libraries(${PROJECT_NAME} glm::glm)

# FetchContent_Declare(
#     Eigen
#     GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
#     GIT_TAG 5.0.1
#     EXCLUDE_FROM_ALL
# )
# FetchContent_MakeAvailable(Eigen)
# target_link_libraries(${PROJECT_NAME} Eigen3::Eigen)
