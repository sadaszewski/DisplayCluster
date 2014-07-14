
# Copyright (c) 2013-2014, EPFL/Blue Brain Project
#                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>
#                     Raphael Dumusc <raphael.dumusc@epfl.ch>
#
# Generates a mock library used by the cpp unit tests.

include_directories(${Qt5Network_INCLUDE_DIRS} ${Qt5Core_INCLUDE_DIRS})

set(MOCK_LIBRARY_FILES
  glxDisplay.h
  GlobalQtApp.h
  MinimalGlobalQtApp.h
)

list(APPEND MOC_HEADERS MockNetworkListener.h)
list(APPEND MOCK_LIBRARY_FILES MockNetworkListener.cpp)

# Core Library Tests
if(BUILD_CORE_LIBRARY)
  list(APPEND MOC_HEADERS MockTextInputDispatcher.h)
  list(APPEND MOCK_LIBRARY_FILES MockTextInputDispatcher.cpp)
endif()

qt5_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS})

# Create the mock library
add_library(mock SHARED ${MOC_OUTFILES} ${MOCK_LIBRARY_FILES})
target_link_libraries(mock ${DC_LIBRARIES})
