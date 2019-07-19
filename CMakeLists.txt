cmake_minimum_required(VERSION 2.6.2)
project(Voreen)

SET(USERSPECIFICPATH /mnt/dscvr/apps)
SET(DEPENDS_PATH /mnt/dscvr/apps/Voreen/SDK/linux)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${USERSPECIFICPATH}/FionaLib)
INCLUDE_DIRECTORIES(${USERSPECIFICPATH}/FionaLib/include)
INCLUDE_DIRECTORIES(${DEPENDS_PATH}/include)
INCLUDE_DIRECTORIES(${DEPENDS_PATH}/include/ext)


LINK_DIRECTORIES(${USERSPECIFICPATH}/Voreen)
LINK_DIRECTORIES(${DEPENDS_PATH}/lib)

if(WIN32)
else()
	add_definitions(-DLINUX_BUILD)
#	add_definitions(-DROSS_TEST)
endif()

ADD_EXECUTABLE(Voreen main.cpp FionaVoreen.cpp VRVolumeAction.cpp )
TARGET_LINK_LIBRARIES(Voreen ${DEPENDS_PATH}/lib/libtgt.so ${DEPENDS_PATH}/lib/libvoreen_core.so ${USERSPECIFICPATH}/FionaLib/libFionaLib.a libGLEW.so libGL.so libGLU.so  ${USERSPECIFICPATH}/FionaLib/linux/lib/libvrpn.a pthread Xext Xxf86vm)
