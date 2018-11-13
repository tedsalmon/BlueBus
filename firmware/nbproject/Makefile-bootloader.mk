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
ifeq "$(wildcard nbproject/Makefile-local-bootloader.mk)" "nbproject/Makefile-local-bootloader.mk"
include nbproject/Makefile-local-bootloader.mk
endif
endif

# Environment
MKDIR=mkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=bootloader
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/firmware.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/firmware.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
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
SOURCEFILES_QUOTED_IF_SPACED=lib/char_queue.c lib/eeprom.c lib/sfr_setters.s lib/utils.c bootloader.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/lib/char_queue.o ${OBJECTDIR}/lib/eeprom.o ${OBJECTDIR}/lib/sfr_setters.o ${OBJECTDIR}/lib/utils.o ${OBJECTDIR}/bootloader.o
POSSIBLE_DEPFILES=${OBJECTDIR}/lib/char_queue.o.d ${OBJECTDIR}/lib/eeprom.o.d ${OBJECTDIR}/lib/sfr_setters.o.d ${OBJECTDIR}/lib/utils.o.d ${OBJECTDIR}/bootloader.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/lib/char_queue.o ${OBJECTDIR}/lib/eeprom.o ${OBJECTDIR}/lib/sfr_setters.o ${OBJECTDIR}/lib/utils.o ${OBJECTDIR}/bootloader.o

# Source Files
SOURCEFILES=lib/char_queue.c lib/eeprom.c lib/sfr_setters.s lib/utils.c bootloader.c


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
	${MAKE}  -f nbproject/Makefile-bootloader.mk dist/${CND_CONF}/${IMAGE_TYPE}/firmware.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=24FJ1024GA606
MP_LINKER_FILE_OPTION=,--script="linkers/bootloader.gld"
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/lib/char_queue.o: lib/char_queue.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/char_queue.o.d 
	@${RM} ${OBJECTDIR}/lib/char_queue.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/char_queue.c  -o ${OBJECTDIR}/lib/char_queue.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/char_queue.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/char_queue.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/eeprom.o: lib/eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/eeprom.o.d 
	@${RM} ${OBJECTDIR}/lib/eeprom.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/eeprom.c  -o ${OBJECTDIR}/lib/eeprom.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/eeprom.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/eeprom.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/utils.o: lib/utils.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/utils.o.d 
	@${RM} ${OBJECTDIR}/lib/utils.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/utils.c  -o ${OBJECTDIR}/lib/utils.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/utils.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/utils.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/bootloader.o: bootloader.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/bootloader.o.d 
	@${RM} ${OBJECTDIR}/bootloader.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  bootloader.c  -o ${OBJECTDIR}/bootloader.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/bootloader.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/bootloader.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
else
${OBJECTDIR}/lib/char_queue.o: lib/char_queue.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/char_queue.o.d 
	@${RM} ${OBJECTDIR}/lib/char_queue.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/char_queue.c  -o ${OBJECTDIR}/lib/char_queue.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/char_queue.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/char_queue.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/eeprom.o: lib/eeprom.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/eeprom.o.d 
	@${RM} ${OBJECTDIR}/lib/eeprom.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/eeprom.c  -o ${OBJECTDIR}/lib/eeprom.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/eeprom.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/eeprom.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/lib/utils.o: lib/utils.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/utils.o.d 
	@${RM} ${OBJECTDIR}/lib/utils.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/utils.c  -o ${OBJECTDIR}/lib/utils.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/lib/utils.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/lib/utils.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/bootloader.o: bootloader.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/bootloader.o.d 
	@${RM} ${OBJECTDIR}/bootloader.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  bootloader.c  -o ${OBJECTDIR}/bootloader.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/bootloader.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/bootloader.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/lib/sfr_setters.o: lib/sfr_setters.s  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/sfr_setters.o.d 
	@${RM} ${OBJECTDIR}/lib/sfr_setters.o 
	${MP_CC} $(MP_EXTRA_AS_PRE)  lib/sfr_setters.s  -o ${OBJECTDIR}/lib/sfr_setters.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -D__DEBUG   -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  -Wa,-MD,"${OBJECTDIR}/lib/sfr_setters.o.d",--defsym=__MPLAB_BUILD=1,--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,,-g,--no-relax$(MP_EXTRA_AS_POST)
	@${FIXDEPS} "${OBJECTDIR}/lib/sfr_setters.o.d"  $(SILENT)  -rsi ${MP_CC_DIR}../  
	
else
${OBJECTDIR}/lib/sfr_setters.o: lib/sfr_setters.s  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/sfr_setters.o.d 
	@${RM} ${OBJECTDIR}/lib/sfr_setters.o 
	${MP_CC} $(MP_EXTRA_AS_PRE)  lib/sfr_setters.s  -o ${OBJECTDIR}/lib/sfr_setters.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  -Wa,-MD,"${OBJECTDIR}/lib/sfr_setters.o.d",--defsym=__MPLAB_BUILD=1,-g,--no-relax$(MP_EXTRA_AS_POST)
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
dist/${CND_CONF}/${IMAGE_TYPE}/firmware.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    linkers/bootloader.gld
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/firmware.${IMAGE_TYPE}.${OUTPUT_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -D__DEBUG=__DEBUG   -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)   -mreserve=data@0x800:0x81B -mreserve=data@0x81C:0x81D -mreserve=data@0x81E:0x81F -mreserve=data@0x820:0x821 -mreserve=data@0x822:0x823 -mreserve=data@0x824:0x827 -mreserve=data@0x82A:0x84F   -Wl,--local-stack,,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D__DEBUG=__DEBUG,,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--cref,--warn-section-align,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml$(MP_EXTRA_LD_POST) 
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/firmware.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   linkers/bootloader.gld
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/firmware.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -Wl,--local-stack,,--defsym=__MPLAB_BUILD=1,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--cref,--warn-section-align,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml$(MP_EXTRA_LD_POST) 
	${MP_CC_DIR}/xc16-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/firmware.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} -a  -omf=elf  
	
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/bootloader
	${RM} -r dist/bootloader

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell "${PATH_TO_IDE_BIN}"mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
