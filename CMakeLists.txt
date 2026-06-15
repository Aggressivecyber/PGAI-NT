cmake_minimum_required(VERSION 3.4.3)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(NT)

# 多线程优化
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -pthread")

find_package(Geant4 REQUIRED ui_all vis_all multithreading)

include(${Geant4_USE_FILE})
include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB header ${PROJECT_SOURCE_DIR}/include/*.hh)

add_executable(NT Practice.cc ${sources} ${header})
target_link_libraries(NT ${Geant4_LIBRARIES})

set(NT_SCRIPTS
    vis.mac
    run.mac
    test3.stl
    ZHUNZHI.stl
)

foreach(_script ${NT_SCRIPTS})
    configure_file(${PROJECT_SOURCE_DIR}/${_script}
                   ${PROJECT_BINARY_DIR}/${_script}
                   COPYONLY)
 endforeach()
