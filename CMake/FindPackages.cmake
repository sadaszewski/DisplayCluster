# generated by Buildyard, do not edit.

include(System)
list(APPEND FIND_PACKAGES_DEFINES ${SYSTEM})

find_package(MPI )
find_package(Poppler 0.24)
find_package(Boost 1.41.0 REQUIRED date_time serialization)
find_package(LibJpegTurbo 1.2.1 REQUIRED)
find_package(FFMPEG  REQUIRED)
find_package(OpenGL  REQUIRED)
find_package(Qt4 4.6 REQUIRED QtNetwork QtOpenGL QtXml QtXmlPatterns)
find_package(TUIO 1.4 REQUIRED)
find_package(GLUT  REQUIRED)

if(EXISTS ${CMAKE_SOURCE_DIR}/CMake/FindPackagesPost.cmake)
  include(${CMAKE_SOURCE_DIR}/CMake/FindPackagesPost.cmake)
endif()

if(MPI_FOUND)
  set(MPI_name MPI)
endif()
if(MPI_FOUND)
  set(MPI_name MPI)
endif()
if(MPI_name)
  list(APPEND FIND_PACKAGES_DEFINES DISPLAYCLUSTER_USE_MPI)
  set(FIND_PACKAGES_FOUND "${FIND_PACKAGES_FOUND} MPI")
  link_directories(${${MPI_name}_LIBRARY_DIRS})
  if(NOT "${${MPI_name}_INCLUDE_DIRS}" MATCHES "-NOTFOUND")
    include_directories(${${MPI_name}_INCLUDE_DIRS})
  endif()
endif()

if(Poppler_FOUND)
  set(Poppler_name Poppler)
endif()
if(POPPLER_FOUND)
  set(Poppler_name POPPLER)
endif()
if(Poppler_name)
  list(APPEND FIND_PACKAGES_DEFINES DISPLAYCLUSTER_USE_POPPLER)
  set(FIND_PACKAGES_FOUND "${FIND_PACKAGES_FOUND} Poppler")
  link_directories(${${Poppler_name}_LIBRARY_DIRS})
  if(NOT "${${Poppler_name}_INCLUDE_DIRS}" MATCHES "-NOTFOUND")
    include_directories(${${Poppler_name}_INCLUDE_DIRS})
  endif()
endif()

if(Boost_FOUND)
  set(Boost_name Boost)
endif()
if(BOOST_FOUND)
  set(Boost_name BOOST)
endif()
if(Boost_name)
  list(APPEND FIND_PACKAGES_DEFINES DISPLAYCLUSTER_USE_BOOST)
  set(FIND_PACKAGES_FOUND "${FIND_PACKAGES_FOUND} Boost")
  link_directories(${${Boost_name}_LIBRARY_DIRS})
  if(NOT "${${Boost_name}_INCLUDE_DIRS}" MATCHES "-NOTFOUND")
    include_directories(SYSTEM ${${Boost_name}_INCLUDE_DIRS})
  endif()
endif()

if(LibJpegTurbo_FOUND)
  set(LibJpegTurbo_name LibJpegTurbo)
endif()
if(LIBJPEGTURBO_FOUND)
  set(LibJpegTurbo_name LIBJPEGTURBO)
endif()
if(LibJpegTurbo_name)
  list(APPEND FIND_PACKAGES_DEFINES DISPLAYCLUSTER_USE_LIBJPEGTURBO)
  set(FIND_PACKAGES_FOUND "${FIND_PACKAGES_FOUND} LibJpegTurbo")
  link_directories(${${LibJpegTurbo_name}_LIBRARY_DIRS})
  if(NOT "${${LibJpegTurbo_name}_INCLUDE_DIRS}" MATCHES "-NOTFOUND")
    include_directories(${${LibJpegTurbo_name}_INCLUDE_DIRS})
  endif()
endif()

if(FFMPEG_FOUND)
  set(FFMPEG_name FFMPEG)
endif()
if(FFMPEG_FOUND)
  set(FFMPEG_name FFMPEG)
endif()
if(FFMPEG_name)
  list(APPEND FIND_PACKAGES_DEFINES DISPLAYCLUSTER_USE_FFMPEG)
  set(FIND_PACKAGES_FOUND "${FIND_PACKAGES_FOUND} FFMPEG")
  link_directories(${${FFMPEG_name}_LIBRARY_DIRS})
  if(NOT "${${FFMPEG_name}_INCLUDE_DIRS}" MATCHES "-NOTFOUND")
    include_directories(${${FFMPEG_name}_INCLUDE_DIRS})
  endif()
endif()

if(OpenGL_FOUND)
  set(OpenGL_name OpenGL)
endif()
if(OPENGL_FOUND)
  set(OpenGL_name OPENGL)
endif()
if(OpenGL_name)
  list(APPEND FIND_PACKAGES_DEFINES DISPLAYCLUSTER_USE_OPENGL)
  set(FIND_PACKAGES_FOUND "${FIND_PACKAGES_FOUND} OpenGL")
  link_directories(${${OpenGL_name}_LIBRARY_DIRS})
  if(NOT "${${OpenGL_name}_INCLUDE_DIRS}" MATCHES "-NOTFOUND")
    include_directories(${${OpenGL_name}_INCLUDE_DIRS})
  endif()
endif()

if(Qt4_FOUND)
  set(Qt4_name Qt4)
endif()
if(QT4_FOUND)
  set(Qt4_name QT4)
endif()
if(Qt4_name)
  list(APPEND FIND_PACKAGES_DEFINES DISPLAYCLUSTER_USE_QT4)
  set(FIND_PACKAGES_FOUND "${FIND_PACKAGES_FOUND} Qt4")
  link_directories(${${Qt4_name}_LIBRARY_DIRS})
  if(NOT "${${Qt4_name}_INCLUDE_DIRS}" MATCHES "-NOTFOUND")
    include_directories(SYSTEM ${${Qt4_name}_INCLUDE_DIRS})
  endif()
endif()

if(TUIO_FOUND)
  set(TUIO_name TUIO)
endif()
if(TUIO_FOUND)
  set(TUIO_name TUIO)
endif()
if(TUIO_name)
  list(APPEND FIND_PACKAGES_DEFINES DISPLAYCLUSTER_USE_TUIO)
  set(FIND_PACKAGES_FOUND "${FIND_PACKAGES_FOUND} TUIO")
  link_directories(${${TUIO_name}_LIBRARY_DIRS})
  if(NOT "${${TUIO_name}_INCLUDE_DIRS}" MATCHES "-NOTFOUND")
    include_directories(${${TUIO_name}_INCLUDE_DIRS})
  endif()
endif()

if(GLUT_FOUND)
  set(GLUT_name GLUT)
endif()
if(GLUT_FOUND)
  set(GLUT_name GLUT)
endif()
if(GLUT_name)
  list(APPEND FIND_PACKAGES_DEFINES DISPLAYCLUSTER_USE_GLUT)
  set(FIND_PACKAGES_FOUND "${FIND_PACKAGES_FOUND} GLUT")
  link_directories(${${GLUT_name}_LIBRARY_DIRS})
  if(NOT "${${GLUT_name}_INCLUDE_DIRS}" MATCHES "-NOTFOUND")
    include_directories(${${GLUT_name}_INCLUDE_DIRS})
  endif()
endif()

set(DISPLAYCLUSTER_BUILD_DEBS autoconf;automake;cmake;freeglut3-dev;git;git-svn;libavcodec-dev;libavformat-dev;libavutil-dev;libboost-date-time-dev;libboost-serialization-dev;libjpeg-turbo8-dev;libopenmpi-dev;libswscale-dev;libturbojpeg;libxmu-dev;pkg-config;subversion)

set(DISPLAYCLUSTER_DEPENDS MPI;Poppler;Boost;LibJpegTurbo;FFMPEG;OpenGL;Qt4;TUIO;GLUT)

# Write defines.h and options.cmake
if(NOT PROJECT_INCLUDE_NAME)
  set(PROJECT_INCLUDE_NAME ${CMAKE_PROJECT_NAME})
endif()
if(NOT OPTIONS_CMAKE)
  set(OPTIONS_CMAKE ${CMAKE_BINARY_DIR}/options.cmake)
endif()
set(DEFINES_FILE "${CMAKE_BINARY_DIR}/include/${PROJECT_INCLUDE_NAME}/defines${SYSTEM}.h")
set(DEFINES_FILE_IN ${DEFINES_FILE}.in)
file(WRITE ${DEFINES_FILE_IN}
  "// generated by CMake/FindPackages.cmake, do not edit.\n\n"
  "#ifndef ${CMAKE_PROJECT_NAME}_DEFINES_${SYSTEM}_H\n"
  "#define ${CMAKE_PROJECT_NAME}_DEFINES_${SYSTEM}_H\n\n")
file(WRITE ${OPTIONS_CMAKE} "# Optional modules enabled during build\n")
foreach(DEF ${FIND_PACKAGES_DEFINES})
  add_definitions(-D${DEF})
  file(APPEND ${DEFINES_FILE_IN}
  "#ifndef ${DEF}\n"
  "#  define ${DEF}\n"
  "#endif\n")
if(NOT DEF STREQUAL SYSTEM)
  file(APPEND ${OPTIONS_CMAKE} "set(${DEF} ON)\n")
endif()
endforeach()
file(APPEND ${DEFINES_FILE_IN}
  "\n#endif\n")

include(UpdateFile)
update_file(${DEFINES_FILE_IN} ${DEFINES_FILE})
if(Boost_FOUND) # another WAR for broken boost stuff...
  set(Boost_VERSION ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION})
endif()
if(CUDA_FOUND)
  string(REPLACE "-std=c++11" "" CUDA_HOST_FLAGS "${CUDA_HOST_FLAGS}")
  string(REPLACE "-std=c++0x" "" CUDA_HOST_FLAGS "${CUDA_HOST_FLAGS}")
endif()
if(FIND_PACKAGES_FOUND)
  if(MSVC)
    message(STATUS "Configured with ${FIND_PACKAGES_FOUND}")
  else()
    message(STATUS "Configured with ${CMAKE_BUILD_TYPE}${FIND_PACKAGES_FOUND}")
  endif()
endif()
