set(SOURCES delay_map.cpp)
set(TARGET delay_map)

if (MSVC)
  set_source_files_properties(delay_map.cpp PROPERTIES LANGUAGE CXX)
  set (SOURCES ${SOURCES} ${FREI0R_DEF})
endif (MSVC)

# find_package(OpenCV REQUIRED)

add_library(${TARGET} MODULE ${SOURCES})
# include_directories(${OpenCV_INCLUDE_DIRECTORIES})
# target_link_libraries(${TARGET} ${OPENV_LIBRARIES})
set_target_properties(${TARGET} PROPERTIES PREFIX "")

install(TARGETS ${TARGET} LIBRARY DESTINATION ${LIBDIR})
