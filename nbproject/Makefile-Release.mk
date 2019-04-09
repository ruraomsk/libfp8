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
CND_PLATFORM=None-Linux
CND_DLIB_EXT=so
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/UDPTrasport.o \
	${OBJECTDIR}/cycleBuff.o \
	${OBJECTDIR}/drvio.o \
	${OBJECTDIR}/jfes.o \
	${OBJECTDIR}/modbus-cpc.o \
	${OBJECTDIR}/netphoto.o \
	${OBJECTDIR}/savervar.o \
	${OBJECTDIR}/variables.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/liblibfp8.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/liblibfp8.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/liblibfp8.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/UDPTrasport.o: UDPTrasport.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/UDPTrasport.o UDPTrasport.c

${OBJECTDIR}/cycleBuff.o: cycleBuff.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/cycleBuff.o cycleBuff.c

${OBJECTDIR}/drvio.o: drvio.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/drvio.o drvio.c

${OBJECTDIR}/jfes.o: jfes.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/jfes.o jfes.c

${OBJECTDIR}/modbus-cpc.o: modbus-cpc.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/modbus-cpc.o modbus-cpc.c

${OBJECTDIR}/netphoto.o: netphoto.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/netphoto.o netphoto.c

${OBJECTDIR}/savervar.o: savervar.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/savervar.o savervar.c

${OBJECTDIR}/variables.o: variables.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/variables.o variables.c

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
