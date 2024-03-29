cmake_minimum_required(VERSION 2.8.11)

project(BabelClient)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find the QtWidgets library
find_package(Qt5Widgets)
find_package(Qt5Core)

file(GLOB SOURCES src/*.cpp includes/*.cpp)

include_directories(Client/includes)
include_directories(CommonIncludes)


qt5_wrap_cpp(moc_sources Client/includes/UserInterfaceController.h)
# Tell CMake to create the helloworld executable
add_executable(BabelClient WIN32 ${SOURCES} ${moc_sources})

# Use the Widgets module from Qt 5.
target_link_libraries(BabelClient Qt5::Widgets)

set_target_properties(BabelClient PROPERTIES LINKER_LANGUAGE CXX)

ADD_DEFINITIONS(-std=c++11)
