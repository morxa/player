INCLUDE_DIRECTORIES (${PLAYERCORE_EXTRA_INCLUDE_DIRS})
LINK_DIRECTORIES (${PLAYERCORE_EXTRA_LINK_DIRS})
ADD_LIBRARY (opaquedriver SHARED opaquedriver.cc)
TARGET_LINK_LIBRARIES (opaquedriver playercore)

INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs)
ADD_EXECUTABLE (opaque_client opaque.c)
TARGET_LINK_LIBRARIES (opaque_client playerc playerinterface playercommon ${PLAYERC_EXTRA_LINK_LIBRARIES})

SET (exampleCMakeLists_in ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt.example.in)
SET (exampleCMakeLists ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt.example)
SET (exampleDestDir share/${PROJECT_NAME_LOWER}/examples/plugins/opaquedriver)
CONFIGURE_FILE (${exampleCMakeLists_in} ${exampleCMakeLists} @ONLY)
INSTALL (FILES ${exampleCMakeLists} DESTINATION ${exampleDestDir} RENAME CMakeLists.txt COMPONENT examples)
INSTALL (FILES opaquedriver.cc opaque.cfg opaque.c sharedstruct.h README DESTINATION ${exampleDestDir} COMPONENT examples)

