set (SOURCES screencap.cpp)
set (TARGET screencap)

if (MSVC)
  set_source_files_properties (screencap.cpp PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

add_library (${TARGET}  MODULE ${SOURCES})
target_link_libraries(screencap X11)
set_target_properties (${TARGET} PROPERTIES PREFIX "")

install (TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
