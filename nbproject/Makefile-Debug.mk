#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/src/BoostTCPServer.o \
	${OBJECTDIR}/src/BoostTCPSession.o \
	${OBJECTDIR}/src/Chat.o \
	${OBJECTDIR}/src/Logger.o \
	${OBJECTDIR}/src/Redis.o \
	${OBJECTDIR}/src/WebsocketServer.o \
	${OBJECTDIR}/src/WsClientData.o \
	${OBJECTDIR}/src/base64.o \
	${OBJECTDIR}/src/main.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-std=c++17 -Wdeprecated-declarations
CXXFLAGS=-std=c++17 -Wdeprecated-declarations

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/chatserver

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/chatserver: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/chatserver ${OBJECTFILES} ${LDLIBSOPTIONS} -static-libgcc -lpthread -lcpp_redis -ltacopie -lcrypto -ljsoncpp -lboost_system

${OBJECTDIR}/src/BoostTCPServer.o: src/BoostTCPServer.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Iincludes -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/BoostTCPServer.o src/BoostTCPServer.cpp

${OBJECTDIR}/src/BoostTCPSession.o: src/BoostTCPSession.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Iincludes -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/BoostTCPSession.o src/BoostTCPSession.cpp

${OBJECTDIR}/src/Chat.o: src/Chat.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Iincludes -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Chat.o src/Chat.cpp

${OBJECTDIR}/src/Logger.o: src/Logger.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Iincludes -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Logger.o src/Logger.cpp

${OBJECTDIR}/src/Redis.o: src/Redis.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Iincludes -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/Redis.o src/Redis.cpp

${OBJECTDIR}/src/WebsocketServer.o: src/WebsocketServer.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Iincludes -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/WebsocketServer.o src/WebsocketServer.cpp

${OBJECTDIR}/src/WsClientData.o: src/WsClientData.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Iincludes -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/WsClientData.o src/WsClientData.cpp

${OBJECTDIR}/src/base64.o: src/base64.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Iincludes -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/base64.o src/base64.cpp

${OBJECTDIR}/src/main.o: src/main.cpp
	${MKDIR} -p ${OBJECTDIR}/src
	${RM} "$@.d"
	$(COMPILE.cc) -g -Iincludes -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/src/main.o src/main.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
