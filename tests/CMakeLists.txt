
# Copyright (c) 2013-2014, EPFL/Blue Brain Project
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>
#                     Raphael Dumusc <raphael.dumusc@epfl.ch>
#

include_directories(${CMAKE_SOURCE_DIR}/src/core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mock)
include_directories(${CMAKE_BINARY_DIR}) ### for config.h ###
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -include config.h")

# Libraries to link the tests executables with
set(DC_LIBRARIES
  dcstream
)

if(BUILD_CORE_LIBRARY)
  list(APPEND DC_LIBRARIES
    dccore
    dcwebservice
  )
endif()

add_subdirectory(cpp)
add_subdirectory(mock)
add_subdirectory(perf)
add_subdirectory(resources)
