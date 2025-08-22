cmake_minimum_required(VERSION 2.6)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(PGAI-NT)

find_package(Geant4 REQUIRED ui_all vis_all)

include(${Geant4_USE_FILE})
include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB header ${PROJECT_SOURCE_DIR}/include/*.hh)

add_executable(PGAI-NT Practice.cc ${sources} ${header})
target_link_libraries(PGAI-NT ${Geant4_LIBRARIES})

set(PGAINT_SCRIPTS
    vis.mac
    run.mac
  test3.stl
  ZHUNZHI.stl
)

foreach(_script ${PGAINT_SCRIPTS})
    configure_file(${PROJECT_SOURCE_DIR}/${_script}
                   ${PROJECT_BINARY_DIR}/${_script}
                   COPYONLY)
 endforeach()