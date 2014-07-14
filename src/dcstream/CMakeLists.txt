include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS} ${Qt5Concurrent_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=shadow")

set(DCSTREAM_LIBRARY_LIBS ${Qt5Core_LIBRARIES}
                          ${Qt5Network_LIBRARIES}
                          ${Qt5Concurrent_LIBRARIES}
                          ${LibJpegTurbo_LIBRARIES}
                          ${Boost_THREAD_LIBRARY}
                          ${Boost_SYSTEM_LIBRARY})

set(DCSTREAM_LIBRARY_SRCS
    ../Event.cpp
    ../log.cpp
    ../MessageHeader.cpp
    Socket.cpp
    Stream.cpp
    StreamPrivate.cpp
    StreamSendWorker.cpp
    ImageWrapper.cpp
    ImageSegmenter.cpp
    ImageJpegCompressor.cpp
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Version.in.h
               ${CMAKE_BINARY_DIR}/Version.h)

set(DCSTREAM_LIBRARY_PUBLIC_HEADERS
    ImageWrapper.h
    Stream.h
    types.h
    ../Event.h
    ${CMAKE_BINARY_DIR}/Version.h
)

set(MOC_HEADERS Socket.h)
qt5_wrap_cpp(MOC_OUTFILES ${MOC_HEADERS})

add_library(dcstream SHARED ${DCSTREAM_LIBRARY_SRCS} ${MOC_OUTFILES} ${DISPLAYCLUSTER_LIBRARY_SHARED_HEADERS})
target_link_libraries(dcstream ${DCSTREAM_LIBRARY_LIBS})
set_target_properties(dcstream PROPERTIES
  VERSION ${VERSION} SOVERSION ${VERSION_ABI})

# install library
install(TARGETS dcstream
    ARCHIVE DESTINATION lib COMPONENT dev       # .a, .lib
    RUNTIME DESTINATION bin COMPONENT dcstream  # .exe, .dll
    LIBRARY DESTINATION lib COMPONENT dcstream) # .so, mod.dll

# install headers
install(FILES ${DCSTREAM_LIBRARY_PUBLIC_HEADERS} DESTINATION include/dc COMPONENT dev)
