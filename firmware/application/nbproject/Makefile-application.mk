#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-application.mk)" "nbproject/Makefile-local-application.mk"
include nbproject/Makefile-local-application.mk
endif
endif

# Environment
MKDIR=mkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=application
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/application.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/application.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

ifdef SUB_IMAGE_ADDRESS
SUB_IMAGE_ADDRESS_COMMAND=--image-address $(SUB_IMAGE_ADDRESS)
else
SUB_IMAGE_ADDRESS_COMMAND=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=lib/bc127.c lib/char_queue.c lib/cli.c lib/eeprom.c lib/event.c lib/ibus.c lib/log.c lib/sfr_setters.s lib/timer.c lib/uart.c lib/utils.c lib/config.c ui/bmbt.c ui/cd53.c main.c handler.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/lib/bc127.o ${OBJECTDIR}/lib/char_queue.o ${OBJECTDIR}/lib/cli.o ${OBJECTDIR}/lib/eeprom.o ${OBJECTDIR}/lib/event.o ${OBJECTDIR}/lib/ibus.o ${OBJECTDIR}/lib/log.o ${OBJECTDIR}/lib/sfr_setters.o ${OBJECTDIR}/lib/timer.o ${OBJECTDIR}/lib/uart.o ${OBJECTDIR}/lib/utils.o ${OBJECTDIR}/lib/config.o ${OBJECTDIR}/ui/bmbt.o ${OBJECTDIR}/ui/cd53.o ${OBJECTDIR}/main.o ${OBJECTDIR}/handler.o
POSSIBLE_DEPFILES=${OBJECTDIR}/lib/bc127.o.d ${OBJECTDIR}/lib/char_queue.o.d ${OBJECTDIR}/lib/cli.o.d ${OBJECTDIR}/lib/eeprom.o.d ${OBJECTDIR}/lib/event.o.d ${OBJECTDIR}/lib/ibus.o.d ${OBJECTDIR}/lib/log.o.d ${OBJECTDIR}/lib/sfr_setters.o.d ${OBJECTDIR}/lib/timer.o.d ${OBJECTDIR}/lib/uart.o.d ${OBJECTDIR}/lib/utils.o.d ${OBJECTDIR}/lib/config.o.d ${OBJECTDIR}/ui/bmbt.o.d ${OBJECTDIR}/ui/cd53.o.d ${OBJECTDIR}/main.o.d ${OBJECTDIR}/handler.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/lib/bc127.o ${OBJECTDIR}/lib/char_queue.o ${OBJECTDIR}/lib/cli.o ${OBJECTDIR}/lib/eeprom.o ${OBJECTDIR}/lib/event.o ${OBJECTDIR}/lib/ibus.o ${OBJECTDIR}/lib/log.o ${OBJECTDIR}/lib/sfr_setters.o ${OBJECTDIR}/lib/timer.o ${OBJECTDIR}/lib/uart.o ${OBJECTDIR}/lib/utils.o ${OBJECTDIR}/lib/config.o ${OBJECTDIR}/ui/bmbt.o ${OBJECTDIR}/ui/cd53.o ${OBJECTDIR}/main.o ${OBJECTDIR}/handler.o

# Source Files
SOURCEFILES=lib/bc127.c lib/char_queue.c lib/cli.c lib/eeprom.c lib/event.c lib/ibus.c lib/log.c lib/sfr_setters.s lib/timer.c lib/uart.c lib/utils.c lib/config.c ui/bmbt.c ui/cd53.c main.c handler.c


CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-application.mk dist/${CND_CONF}/${IMAGE_TYPE}/application.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=24FJ1024GA606
MP_LINKER_FILE_OPTION=,--script="application.gld"
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/lib/bc127.o: lib/bc127.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/bc127.o.d 
	@${RM} ${OBJECTDIR}/lib/bc127.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/bc127.c  -o ${OBJECTDIR}/lib/bc127.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/bc127.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/bc127.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/char_queue.o: lib/char_queue.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/char_queue.o.d 
	@${RM} ${OBJECTDIR}/lib/char_queue.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/char_queue.c  -o ${OBJECTDIR}/lib/char_queue.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/char_queue.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/char_queue.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/cli.o: lib/cli.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/cli.o.d 
	@${RM} ${OBJECTDIR}/lib/cli.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/cli.c  -o ${OBJECTDIR}/lib/cli.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/cli.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/cli.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/eeprom.o: lib/eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/eeprom.o.d 
	@${RM} ${OBJECTDIR}/lib/eeprom.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/eeprom.c  -o ${OBJECTDIR}/lib/eeprom.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/eeprom.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/eeprom.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/event.o: lib/event.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/event.o.d 
	@${RM} ${OBJECTDIR}/lib/event.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/event.c  -o ${OBJECTDIR}/lib/event.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/event.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/event.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/ibus.o: lib/ibus.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/ibus.o.d 
	@${RM} ${OBJECTDIR}/lib/ibus.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/ibus.c  -o ${OBJECTDIR}/lib/ibus.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/ibus.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/ibus.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/log.o: lib/log.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/log.o.d 
	@${RM} ${OBJECTDIR}/lib/log.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/log.c  -o ${OBJECTDIR}/lib/log.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/log.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/log.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/timer.o: lib/timer.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/timer.o.d 
	@${RM} ${OBJECTDIR}/lib/timer.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/timer.c  -o ${OBJECTDIR}/lib/timer.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/timer.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/timer.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/uart.o: lib/uart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/uart.o.d 
	@${RM} ${OBJECTDIR}/lib/uart.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/uart.c  -o ${OBJECTDIR}/lib/uart.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/uart.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/uart.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/utils.o: lib/utils.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/utils.o.d 
	@${RM} ${OBJECTDIR}/lib/utils.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/utils.c  -o ${OBJECTDIR}/lib/utils.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/utils.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/utils.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/config.o: lib/config.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/config.o.d 
	@${RM} ${OBJECTDIR}/lib/config.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/config.c  -o ${OBJECTDIR}/lib/config.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/config.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/config.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/ui/bmbt.o: ui/bmbt.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/ui" 
	@${RM} ${OBJECTDIR}/ui/bmbt.o.d 
	@${RM} ${OBJECTDIR}/ui/bmbt.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ui/bmbt.c  -o ${OBJECTDIR}/ui/bmbt.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/ui/bmbt.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/ui/bmbt.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/ui/cd53.o: ui/cd53.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/ui" 
	@${RM} ${OBJECTDIR}/ui/cd53.o.d 
	@${RM} ${OBJECTDIR}/ui/cd53.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ui/cd53.c  -o ${OBJECTDIR}/ui/cd53.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/ui/cd53.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/ui/cd53.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/main.o: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  main.c  -o ${OBJECTDIR}/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/main.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/main.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/handler.o: handler.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/handler.o.d 
	@${RM} ${OBJECTDIR}/handler.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  handler.c  -o ${OBJECTDIR}/handler.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/handler.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/handler.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
else
${OBJECTDIR}/lib/bc127.o: lib/bc127.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/bc127.o.d 
	@${RM} ${OBJECTDIR}/lib/bc127.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/bc127.c  -o ${OBJECTDIR}/lib/bc127.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/bc127.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/bc127.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/char_queue.o: lib/char_queue.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/char_queue.o.d 
	@${RM} ${OBJECTDIR}/lib/char_queue.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/char_queue.c  -o ${OBJECTDIR}/lib/char_queue.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/char_queue.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/char_queue.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/cli.o: lib/cli.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/cli.o.d 
	@${RM} ${OBJECTDIR}/lib/cli.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/cli.c  -o ${OBJECTDIR}/lib/cli.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/cli.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/cli.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/eeprom.o: lib/eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/eeprom.o.d 
	@${RM} ${OBJECTDIR}/lib/eeprom.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/eeprom.c  -o ${OBJECTDIR}/lib/eeprom.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/eeprom.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/eeprom.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/event.o: lib/event.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/event.o.d 
	@${RM} ${OBJECTDIR}/lib/event.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/event.c  -o ${OBJECTDIR}/lib/event.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/event.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/event.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/ibus.o: lib/ibus.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/ibus.o.d 
	@${RM} ${OBJECTDIR}/lib/ibus.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/ibus.c  -o ${OBJECTDIR}/lib/ibus.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/ibus.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/ibus.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/log.o: lib/log.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/log.o.d 
	@${RM} ${OBJECTDIR}/lib/log.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/log.c  -o ${OBJECTDIR}/lib/log.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/log.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/log.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/timer.o: lib/timer.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/timer.o.d 
	@${RM} ${OBJECTDIR}/lib/timer.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/timer.c  -o ${OBJECTDIR}/lib/timer.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/timer.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/timer.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/uart.o: lib/uart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/uart.o.d 
	@${RM} ${OBJECTDIR}/lib/uart.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/uart.c  -o ${OBJECTDIR}/lib/uart.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/uart.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/uart.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/utils.o: lib/utils.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/utils.o.d 
	@${RM} ${OBJECTDIR}/lib/utils.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/utils.c  -o ${OBJECTDIR}/lib/utils.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/utils.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/utils.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/config.o: lib/config.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/config.o.d 
	@${RM} ${OBJECTDIR}/lib/config.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/config.c  -o ${OBJECTDIR}/lib/config.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/config.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/config.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/ui/bmbt.o: ui/bmbt.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/ui" 
	@${RM} ${OBJECTDIR}/ui/bmbt.o.d 
	@${RM} ${OBJECTDIR}/ui/bmbt.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ui/bmbt.c  -o ${OBJECTDIR}/ui/bmbt.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/ui/bmbt.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/ui/bmbt.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/ui/cd53.o: ui/cd53.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/ui" 
	@${RM} ${OBJECTDIR}/ui/cd53.o.d 
	@${RM} ${OBJECTDIR}/ui/cd53.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ui/cd53.c  -o ${OBJECTDIR}/ui/cd53.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/ui/cd53.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/ui/cd53.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/main.o: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  main.c  -o ${OBJECTDIR}/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/main.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/main.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/handler.o: handler.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/handler.o.d 
	@${RM} ${OBJECTDIR}/handler.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  handler.c  -o ${OBJECTDIR}/handler.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/handler.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/handler.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/lib/sfr_setters.o: lib/sfr_setters.s  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/sfr_setters.o.d 
	@${RM} ${OBJECTDIR}/lib/sfr_setters.o 
	${MP_CC} $(MP_EXTRA_AS_PRE)  lib/sfr_setters.s  -o ${OBJECTDIR}/lib/sfr_setters.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -D__DEBUG   -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  -Wa,-MD,"${OBJECTDIR}/lib/sfr_setters.o.d",--defsym=__MPLAB_BUILD=1,--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,,-g,--no-relax$(MP_EXTRA_AS_POST)
	@${FIXDEPS} "${OBJECTDIR}/lib/sfr_setters.o.d"  $(SILENT)  -rsi ${MP_CC_DIR}../  
	
else
${OBJECTDIR}/lib/sfr_setters.o: lib/sfr_setters.s  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/sfr_setters.o.d 
	@${RM} ${OBJECTDIR}/lib/sfr_setters.o 
	${MP_CC} $(MP_EXTRA_AS_PRE)  lib/sfr_setters.s  -o ${OBJECTDIR}/lib/sfr_setters.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  -Wa,-MD,"${OBJECTDIR}/lib/sfr_setters.o.d",--defsym=__MPLAB_BUILD=1,-g,--no-relax$(MP_EXTRA_AS_POST)
	@${FIXDEPS} "${OBJECTDIR}/lib/sfr_setters.o.d"  $(SILENT)  -rsi ${MP_CC_DIR}../  
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemblePreproc
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/application.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    application.gld
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/application.${IMAGE_TYPE}.${OUTPUT_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -D__DEBUG=__DEBUG   -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)   -mreserve=data@0x800:0x81B -mreserve=data@0x81C:0x81D -mreserve=data@0x81E:0x81F -mreserve=data@0x820:0x821 -mreserve=data@0x822:0x823 -mreserve=data@0x824:0x827 -mreserve=data@0x82A:0x84F   -Wl,--local-stack,,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D__DEBUG=__DEBUG,,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--cref,--warn-section-align,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml$(MP_EXTRA_LD_POST) 
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/application.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   application.gld
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/application.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -Wl,--local-stack,,--defsym=__MPLAB_BUILD=1,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--cref,--warn-section-align,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml$(MP_EXTRA_LD_POST) 
	${MP_CC_DIR}/xc16-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/application.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} -a  -omf=elf  
	
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/application
	${RM} -r dist/application

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell "${PATH_TO_IDE_BIN}"mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
