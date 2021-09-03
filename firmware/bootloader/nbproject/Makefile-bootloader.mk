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
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/bootloader.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/bootloader.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
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
SOURCEFILES_QUOTED_IF_SPACED=lib/eeprom.c lib/flash.c lib/protocol.c lib/sfr_setters.s lib/timer.c lib/uart.c lib/utils.c main.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/lib/eeprom.o ${OBJECTDIR}/lib/flash.o ${OBJECTDIR}/lib/protocol.o ${OBJECTDIR}/lib/sfr_setters.o ${OBJECTDIR}/lib/timer.o ${OBJECTDIR}/lib/uart.o ${OBJECTDIR}/lib/utils.o ${OBJECTDIR}/main.o
POSSIBLE_DEPFILES=${OBJECTDIR}/lib/eeprom.o.d ${OBJECTDIR}/lib/flash.o.d ${OBJECTDIR}/lib/protocol.o.d ${OBJECTDIR}/lib/sfr_setters.o.d ${OBJECTDIR}/lib/timer.o.d ${OBJECTDIR}/lib/uart.o.d ${OBJECTDIR}/lib/utils.o.d ${OBJECTDIR}/main.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/lib/eeprom.o ${OBJECTDIR}/lib/flash.o ${OBJECTDIR}/lib/protocol.o ${OBJECTDIR}/lib/sfr_setters.o ${OBJECTDIR}/lib/timer.o ${OBJECTDIR}/lib/uart.o ${OBJECTDIR}/lib/utils.o ${OBJECTDIR}/main.o

# Source Files
SOURCEFILES=lib/eeprom.c lib/flash.c lib/protocol.c lib/sfr_setters.s lib/timer.c lib/uart.c lib/utils.c main.c



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
	${MAKE}  -f nbproject/Makefile-bootloader.mk dist/${CND_CONF}/${IMAGE_TYPE}/bootloader.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=24FJ1024GA606
MP_LINKER_FILE_OPTION=,--script="bootloader.gld"
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/lib/eeprom.o: lib/eeprom.c  .generated_files/flags/bootloader/65c8d37683ed732f9849da0e8cd96b545b0696fe .generated_files/flags/bootloader/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/eeprom.o.d 
	@${RM} ${OBJECTDIR}/lib/eeprom.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/eeprom.c  -o ${OBJECTDIR}/lib/eeprom.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/eeprom.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/flash.o: lib/flash.c  .generated_files/flags/bootloader/73f72b62c70cbdcdda2d4fc863fae8e45e080ca1 .generated_files/flags/bootloader/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/flash.o.d 
	@${RM} ${OBJECTDIR}/lib/flash.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/flash.c  -o ${OBJECTDIR}/lib/flash.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/flash.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/protocol.o: lib/protocol.c  .generated_files/flags/bootloader/10bde5b7480b699fbd1b844564aaf86e3b30be34 .generated_files/flags/bootloader/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/protocol.o.d 
	@${RM} ${OBJECTDIR}/lib/protocol.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/protocol.c  -o ${OBJECTDIR}/lib/protocol.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/protocol.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/timer.o: lib/timer.c  .generated_files/flags/bootloader/34e4eee1cdf6f0505b49043eba8b2429a7f74563 .generated_files/flags/bootloader/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/timer.o.d 
	@${RM} ${OBJECTDIR}/lib/timer.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/timer.c  -o ${OBJECTDIR}/lib/timer.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/timer.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/uart.o: lib/uart.c  .generated_files/flags/bootloader/723cb0ece4e30dee8013952a8ed2f59972eb55a9 .generated_files/flags/bootloader/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/uart.o.d 
	@${RM} ${OBJECTDIR}/lib/uart.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/uart.c  -o ${OBJECTDIR}/lib/uart.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/uart.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/utils.o: lib/utils.c  .generated_files/flags/bootloader/d8213b4f98a1b341433dab4f36a8580691535e77 .generated_files/flags/bootloader/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/utils.o.d 
	@${RM} ${OBJECTDIR}/lib/utils.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/utils.c  -o ${OBJECTDIR}/lib/utils.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/utils.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/main.o: main.c  .generated_files/flags/bootloader/e514e2b5158a854cf01539ac6bd1a0c04428166e .generated_files/flags/bootloader/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  main.c  -o ${OBJECTDIR}/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/main.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
else
${OBJECTDIR}/lib/eeprom.o: lib/eeprom.c  .generated_files/flags/bootloader/114767bcd834fb72465bc4cab97db0bf8073ea6 .generated_files/flags/bootloader/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/eeprom.o.d 
	@${RM} ${OBJECTDIR}/lib/eeprom.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/eeprom.c  -o ${OBJECTDIR}/lib/eeprom.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/eeprom.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/flash.o: lib/flash.c  .generated_files/flags/bootloader/6a1b42d6b2675fc3e47b8a8f97818ae1c716a60a .generated_files/flags/bootloader/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/flash.o.d 
	@${RM} ${OBJECTDIR}/lib/flash.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/flash.c  -o ${OBJECTDIR}/lib/flash.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/flash.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/protocol.o: lib/protocol.c  .generated_files/flags/bootloader/ca50b56aa28c9c8c465faefe485c231aa3879530 .generated_files/flags/bootloader/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/protocol.o.d 
	@${RM} ${OBJECTDIR}/lib/protocol.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/protocol.c  -o ${OBJECTDIR}/lib/protocol.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/protocol.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/timer.o: lib/timer.c  .generated_files/flags/bootloader/ebd92914b7b6b5086dd3e71af7da458c2794b861 .generated_files/flags/bootloader/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/timer.o.d 
	@${RM} ${OBJECTDIR}/lib/timer.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/timer.c  -o ${OBJECTDIR}/lib/timer.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/timer.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/uart.o: lib/uart.c  .generated_files/flags/bootloader/794fc427c3c5e663fe2b95ce380acf12b52c437b .generated_files/flags/bootloader/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/uart.o.d 
	@${RM} ${OBJECTDIR}/lib/uart.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/uart.c  -o ${OBJECTDIR}/lib/uart.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/uart.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/utils.o: lib/utils.c  .generated_files/flags/bootloader/9db049af1ff0db8aea67381ff8859b05a7a4051e .generated_files/flags/bootloader/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/utils.o.d 
	@${RM} ${OBJECTDIR}/lib/utils.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/utils.c  -o ${OBJECTDIR}/lib/utils.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/utils.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/main.o: main.c  .generated_files/flags/bootloader/ce5a62d7029723c3785ef7f36640b19848e591df .generated_files/flags/bootloader/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  main.c  -o ${OBJECTDIR}/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/main.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/lib/sfr_setters.o: lib/sfr_setters.s  .generated_files/flags/bootloader/d8a6ee19736fb7c81f89febf77967b05f457b3e9 .generated_files/flags/bootloader/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/sfr_setters.o.d 
	@${RM} ${OBJECTDIR}/lib/sfr_setters.o 
	${MP_CC} $(MP_EXTRA_AS_PRE)  lib/sfr_setters.s  -o ${OBJECTDIR}/lib/sfr_setters.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -D__DEBUG   -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  -Wa,-MD,"${OBJECTDIR}/lib/sfr_setters.o.d",--defsym=__MPLAB_BUILD=1,--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,,-g,--no-relax$(MP_EXTRA_AS_POST)  -mdfp="${DFP_DIR}/xc16"
	
else
${OBJECTDIR}/lib/sfr_setters.o: lib/sfr_setters.s  .generated_files/flags/bootloader/e09a0fe2f21e6a9ef1ce54192d563f0d09f382d1 .generated_files/flags/bootloader/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/sfr_setters.o.d 
	@${RM} ${OBJECTDIR}/lib/sfr_setters.o 
	${MP_CC} $(MP_EXTRA_AS_PRE)  lib/sfr_setters.s  -o ${OBJECTDIR}/lib/sfr_setters.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  -Wa,-MD,"${OBJECTDIR}/lib/sfr_setters.o.d",--defsym=__MPLAB_BUILD=1,-g,--no-relax$(MP_EXTRA_AS_POST)  -mdfp="${DFP_DIR}/xc16"
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemblePreproc
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/bootloader.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    bootloader.gld
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/bootloader.${IMAGE_TYPE}.${OUTPUT_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -D__DEBUG=__DEBUG   -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)   -mreserve=data@0x800:0x81B -mreserve=data@0x81C:0x81D -mreserve=data@0x81E:0x81F -mreserve=data@0x820:0x821 -mreserve=data@0x822:0x823 -mreserve=data@0x824:0x827 -mreserve=data@0x82A:0x84F   -Wl,--local-stack,,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D__DEBUG=__DEBUG,,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml$(MP_EXTRA_LD_POST)  -mdfp="${DFP_DIR}/xc16" 
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/bootloader.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   bootloader.gld
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/bootloader.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -omf=elf -DXPRJ_bootloader=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -Wl,--local-stack,,--defsym=__MPLAB_BUILD=1,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml$(MP_EXTRA_LD_POST)  -mdfp="${DFP_DIR}/xc16" 
	${MP_CC_DIR}/xc16-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/bootloader.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} -a  -omf=elf   -mdfp="${DFP_DIR}/xc16" 
	
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
