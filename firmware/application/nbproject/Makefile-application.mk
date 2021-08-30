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
SOURCEFILES_QUOTED_IF_SPACED=lib/bc127.c lib/char_queue.c lib/config.c lib/eeprom.c lib/event.c lib/i2c.c lib/ibus.c lib/locale.c lib/log.c lib/pcm51xx.c lib/sfr_setters.s lib/timer.c lib/uart.c lib/utils.c lib/wm88xx.c ui/menu/singleline.c ui/bmbt.c ui/cd53.c ui/cli.c ui/mid.c main.c handler.c upgrade.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/lib/bc127.o ${OBJECTDIR}/lib/char_queue.o ${OBJECTDIR}/lib/config.o ${OBJECTDIR}/lib/eeprom.o ${OBJECTDIR}/lib/event.o ${OBJECTDIR}/lib/i2c.o ${OBJECTDIR}/lib/ibus.o ${OBJECTDIR}/lib/locale.o ${OBJECTDIR}/lib/log.o ${OBJECTDIR}/lib/pcm51xx.o ${OBJECTDIR}/lib/sfr_setters.o ${OBJECTDIR}/lib/timer.o ${OBJECTDIR}/lib/uart.o ${OBJECTDIR}/lib/utils.o ${OBJECTDIR}/lib/wm88xx.o ${OBJECTDIR}/ui/menu/singleline.o ${OBJECTDIR}/ui/bmbt.o ${OBJECTDIR}/ui/cd53.o ${OBJECTDIR}/ui/cli.o ${OBJECTDIR}/ui/mid.o ${OBJECTDIR}/main.o ${OBJECTDIR}/handler.o ${OBJECTDIR}/upgrade.o
POSSIBLE_DEPFILES=${OBJECTDIR}/lib/bc127.o.d ${OBJECTDIR}/lib/char_queue.o.d ${OBJECTDIR}/lib/config.o.d ${OBJECTDIR}/lib/eeprom.o.d ${OBJECTDIR}/lib/event.o.d ${OBJECTDIR}/lib/i2c.o.d ${OBJECTDIR}/lib/ibus.o.d ${OBJECTDIR}/lib/locale.o.d ${OBJECTDIR}/lib/log.o.d ${OBJECTDIR}/lib/pcm51xx.o.d ${OBJECTDIR}/lib/sfr_setters.o.d ${OBJECTDIR}/lib/timer.o.d ${OBJECTDIR}/lib/uart.o.d ${OBJECTDIR}/lib/utils.o.d ${OBJECTDIR}/lib/wm88xx.o.d ${OBJECTDIR}/ui/menu/singleline.o.d ${OBJECTDIR}/ui/bmbt.o.d ${OBJECTDIR}/ui/cd53.o.d ${OBJECTDIR}/ui/cli.o.d ${OBJECTDIR}/ui/mid.o.d ${OBJECTDIR}/main.o.d ${OBJECTDIR}/handler.o.d ${OBJECTDIR}/upgrade.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/lib/bc127.o ${OBJECTDIR}/lib/char_queue.o ${OBJECTDIR}/lib/config.o ${OBJECTDIR}/lib/eeprom.o ${OBJECTDIR}/lib/event.o ${OBJECTDIR}/lib/i2c.o ${OBJECTDIR}/lib/ibus.o ${OBJECTDIR}/lib/locale.o ${OBJECTDIR}/lib/log.o ${OBJECTDIR}/lib/pcm51xx.o ${OBJECTDIR}/lib/sfr_setters.o ${OBJECTDIR}/lib/timer.o ${OBJECTDIR}/lib/uart.o ${OBJECTDIR}/lib/utils.o ${OBJECTDIR}/lib/wm88xx.o ${OBJECTDIR}/ui/menu/singleline.o ${OBJECTDIR}/ui/bmbt.o ${OBJECTDIR}/ui/cd53.o ${OBJECTDIR}/ui/cli.o ${OBJECTDIR}/ui/mid.o ${OBJECTDIR}/main.o ${OBJECTDIR}/handler.o ${OBJECTDIR}/upgrade.o

# Source Files
SOURCEFILES=lib/bc127.c lib/char_queue.c lib/config.c lib/eeprom.c lib/event.c lib/i2c.c lib/ibus.c lib/locale.c lib/log.c lib/pcm51xx.c lib/sfr_setters.s lib/timer.c lib/uart.c lib/utils.c lib/wm88xx.c ui/menu/singleline.c ui/bmbt.c ui/cd53.c ui/cli.c ui/mid.c main.c handler.c upgrade.c



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
${OBJECTDIR}/lib/bc127.o: lib/bc127.c  .generated_files/flags/application/281f50d0456fa0219da46fb0c383b5d677f10ef7 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/bc127.o.d 
	@${RM} ${OBJECTDIR}/lib/bc127.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/bc127.c  -o ${OBJECTDIR}/lib/bc127.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/bc127.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/char_queue.o: lib/char_queue.c  .generated_files/flags/application/d06a22b42ac1a5140cbde84ce3a65227498bf409 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/char_queue.o.d 
	@${RM} ${OBJECTDIR}/lib/char_queue.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/char_queue.c  -o ${OBJECTDIR}/lib/char_queue.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/char_queue.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/config.o: lib/config.c  .generated_files/flags/application/f48a4744a0654557c3dba52e18a7ad3987632440 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/config.o.d 
	@${RM} ${OBJECTDIR}/lib/config.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/config.c  -o ${OBJECTDIR}/lib/config.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/config.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/eeprom.o: lib/eeprom.c  .generated_files/flags/application/dba0029d23f27358aa5892babdaf7ee846c42900 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/eeprom.o.d 
	@${RM} ${OBJECTDIR}/lib/eeprom.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/eeprom.c  -o ${OBJECTDIR}/lib/eeprom.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/eeprom.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/event.o: lib/event.c  .generated_files/flags/application/c27ef71e97b4dcaead19075f8f593cc04f075fca .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/event.o.d 
	@${RM} ${OBJECTDIR}/lib/event.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/event.c  -o ${OBJECTDIR}/lib/event.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/event.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/i2c.o: lib/i2c.c  .generated_files/flags/application/c6165d23cb08f231b229a743646404b2fec9ea13 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/i2c.o.d 
	@${RM} ${OBJECTDIR}/lib/i2c.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/i2c.c  -o ${OBJECTDIR}/lib/i2c.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/i2c.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/ibus.o: lib/ibus.c  .generated_files/flags/application/af70d94eb549f1d69b81236396cace3b1296f3df .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/ibus.o.d 
	@${RM} ${OBJECTDIR}/lib/ibus.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/ibus.c  -o ${OBJECTDIR}/lib/ibus.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/ibus.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/locale.o: lib/locale.c  .generated_files/flags/application/6462efc3cd8bb390026e1ce3623ff594aa4690c4 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/locale.o.d 
	@${RM} ${OBJECTDIR}/lib/locale.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/locale.c  -o ${OBJECTDIR}/lib/locale.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/locale.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/log.o: lib/log.c  .generated_files/flags/application/72643195afd963b1b0d87147dc843cf7b3f82c00 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/log.o.d 
	@${RM} ${OBJECTDIR}/lib/log.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/log.c  -o ${OBJECTDIR}/lib/log.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/log.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/pcm51xx.o: lib/pcm51xx.c  .generated_files/flags/application/c5653bb75796a2912623dae1527be65944f79cb7 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/pcm51xx.o.d 
	@${RM} ${OBJECTDIR}/lib/pcm51xx.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/pcm51xx.c  -o ${OBJECTDIR}/lib/pcm51xx.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/pcm51xx.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/timer.o: lib/timer.c  .generated_files/flags/application/a8b1156ffe3ac5920c358dd1e19fdc5ef5be970a .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/timer.o.d 
	@${RM} ${OBJECTDIR}/lib/timer.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/timer.c  -o ${OBJECTDIR}/lib/timer.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/timer.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/uart.o: lib/uart.c  .generated_files/flags/application/881d8467d4665f1e0a8f23267f7f2b563987cd46 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/uart.o.d 
	@${RM} ${OBJECTDIR}/lib/uart.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/uart.c  -o ${OBJECTDIR}/lib/uart.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/uart.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/utils.o: lib/utils.c  .generated_files/flags/application/6c918a441a3267e2b7f026454da13f92ef49d5dd .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/utils.o.d 
	@${RM} ${OBJECTDIR}/lib/utils.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/utils.c  -o ${OBJECTDIR}/lib/utils.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/utils.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/wm88xx.o: lib/wm88xx.c  .generated_files/flags/application/dd433984b96247c8fd4cc32019d449657033ed5d .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/wm88xx.o.d 
	@${RM} ${OBJECTDIR}/lib/wm88xx.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/wm88xx.c  -o ${OBJECTDIR}/lib/wm88xx.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/wm88xx.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/ui/menu/singleline.o: ui/menu/singleline.c  .generated_files/flags/application/290f9cec06ecaa96cd5d11286f13eed08732d9df .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/ui/menu" 
	@${RM} ${OBJECTDIR}/ui/menu/singleline.o.d 
	@${RM} ${OBJECTDIR}/ui/menu/singleline.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ui/menu/singleline.c  -o ${OBJECTDIR}/ui/menu/singleline.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/ui/menu/singleline.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/ui/bmbt.o: ui/bmbt.c  .generated_files/flags/application/c91cb29b99c0d450ea6b4d08013bb5b9dc856cbb .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/ui" 
	@${RM} ${OBJECTDIR}/ui/bmbt.o.d 
	@${RM} ${OBJECTDIR}/ui/bmbt.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ui/bmbt.c  -o ${OBJECTDIR}/ui/bmbt.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/ui/bmbt.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/ui/cd53.o: ui/cd53.c  .generated_files/flags/application/172c079ae143cd4062ef1c856407358eb57a0815 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/ui" 
	@${RM} ${OBJECTDIR}/ui/cd53.o.d 
	@${RM} ${OBJECTDIR}/ui/cd53.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ui/cd53.c  -o ${OBJECTDIR}/ui/cd53.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/ui/cd53.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/ui/cli.o: ui/cli.c  .generated_files/flags/application/764160d335a307728e7bcd02e862456d52eecbe4 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/ui" 
	@${RM} ${OBJECTDIR}/ui/cli.o.d 
	@${RM} ${OBJECTDIR}/ui/cli.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ui/cli.c  -o ${OBJECTDIR}/ui/cli.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/ui/cli.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/ui/mid.o: ui/mid.c  .generated_files/flags/application/adbf0c4b418e878bff6c8bd58e0088d43c23acc4 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/ui" 
	@${RM} ${OBJECTDIR}/ui/mid.o.d 
	@${RM} ${OBJECTDIR}/ui/mid.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ui/mid.c  -o ${OBJECTDIR}/ui/mid.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/ui/mid.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/main.o: main.c  .generated_files/flags/application/21d18bf1e98da23f548f9b575d4861f80d7b2be1 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  main.c  -o ${OBJECTDIR}/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/main.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/handler.o: handler.c  .generated_files/flags/application/1d2fb5ac833e295be41bae1919f44ecb62cfebef .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/handler.o.d 
	@${RM} ${OBJECTDIR}/handler.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  handler.c  -o ${OBJECTDIR}/handler.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/handler.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/upgrade.o: upgrade.c  .generated_files/flags/application/97cd3db7e64c033e106b3051fffbf35a50cd0501 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/upgrade.o.d 
	@${RM} ${OBJECTDIR}/upgrade.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  upgrade.c  -o ${OBJECTDIR}/upgrade.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/upgrade.o.d"      -g -D__DEBUG   -mno-eds-warn  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
else
${OBJECTDIR}/lib/bc127.o: lib/bc127.c  .generated_files/flags/application/a000bbb0f9240dca2a2ad76410be093f5d31d55 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/bc127.o.d 
	@${RM} ${OBJECTDIR}/lib/bc127.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/bc127.c  -o ${OBJECTDIR}/lib/bc127.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/bc127.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/char_queue.o: lib/char_queue.c  .generated_files/flags/application/f927869f11a4335b23cb337af49b2820b19b0e28 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/char_queue.o.d 
	@${RM} ${OBJECTDIR}/lib/char_queue.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/char_queue.c  -o ${OBJECTDIR}/lib/char_queue.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/char_queue.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/config.o: lib/config.c  .generated_files/flags/application/f485656177602226f875c8a924e79c245767ee96 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/config.o.d 
	@${RM} ${OBJECTDIR}/lib/config.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/config.c  -o ${OBJECTDIR}/lib/config.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/config.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/eeprom.o: lib/eeprom.c  .generated_files/flags/application/d5d28bfc17925a0a1cbfb21bb901bc5f623d6aea .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/eeprom.o.d 
	@${RM} ${OBJECTDIR}/lib/eeprom.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/eeprom.c  -o ${OBJECTDIR}/lib/eeprom.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/eeprom.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/event.o: lib/event.c  .generated_files/flags/application/4920f6a8630c79d4124bf6aadc8edb18a22e4962 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/event.o.d 
	@${RM} ${OBJECTDIR}/lib/event.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/event.c  -o ${OBJECTDIR}/lib/event.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/event.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/i2c.o: lib/i2c.c  .generated_files/flags/application/98d4137d3e2bdbb8482f831c477ea3e74ee6715c .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/i2c.o.d 
	@${RM} ${OBJECTDIR}/lib/i2c.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/i2c.c  -o ${OBJECTDIR}/lib/i2c.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/i2c.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/ibus.o: lib/ibus.c  .generated_files/flags/application/bc2b7bed63f5e6eccfad94be2ac0e7114cbae2ca .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/ibus.o.d 
	@${RM} ${OBJECTDIR}/lib/ibus.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/ibus.c  -o ${OBJECTDIR}/lib/ibus.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/ibus.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/locale.o: lib/locale.c  .generated_files/flags/application/f601444024f36a0b1e45da7e7d09e0e7b2f2d9db .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/locale.o.d 
	@${RM} ${OBJECTDIR}/lib/locale.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/locale.c  -o ${OBJECTDIR}/lib/locale.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/locale.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/log.o: lib/log.c  .generated_files/flags/application/3ab1e6b9606e01be00c56ae9960d9960c421c466 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/log.o.d 
	@${RM} ${OBJECTDIR}/lib/log.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/log.c  -o ${OBJECTDIR}/lib/log.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/log.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/pcm51xx.o: lib/pcm51xx.c  .generated_files/flags/application/8cfa68c1983643620a934303b574fca4e657cacc .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/pcm51xx.o.d 
	@${RM} ${OBJECTDIR}/lib/pcm51xx.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/pcm51xx.c  -o ${OBJECTDIR}/lib/pcm51xx.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/pcm51xx.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/timer.o: lib/timer.c  .generated_files/flags/application/d5f815433de4de939a9fb61275c063fe621f3b2c .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/timer.o.d 
	@${RM} ${OBJECTDIR}/lib/timer.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/timer.c  -o ${OBJECTDIR}/lib/timer.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/timer.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/uart.o: lib/uart.c  .generated_files/flags/application/348df68eb2ae1993b95e4c933e4db57e498b3b7d .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/uart.o.d 
	@${RM} ${OBJECTDIR}/lib/uart.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/uart.c  -o ${OBJECTDIR}/lib/uart.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/uart.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/utils.o: lib/utils.c  .generated_files/flags/application/1818dae3dbfdf349a2e69dcc3439fafdf573fc36 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/utils.o.d 
	@${RM} ${OBJECTDIR}/lib/utils.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/utils.c  -o ${OBJECTDIR}/lib/utils.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/utils.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/lib/wm88xx.o: lib/wm88xx.c  .generated_files/flags/application/d7fde23b5bd25e0676c9d775a2a90b1a67b50d77 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/wm88xx.o.d 
	@${RM} ${OBJECTDIR}/lib/wm88xx.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  lib/wm88xx.c  -o ${OBJECTDIR}/lib/wm88xx.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/lib/wm88xx.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/ui/menu/singleline.o: ui/menu/singleline.c  .generated_files/flags/application/ae423037af4c40e60c0f8dbbd769dcb514fe21b7 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/ui/menu" 
	@${RM} ${OBJECTDIR}/ui/menu/singleline.o.d 
	@${RM} ${OBJECTDIR}/ui/menu/singleline.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ui/menu/singleline.c  -o ${OBJECTDIR}/ui/menu/singleline.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/ui/menu/singleline.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/ui/bmbt.o: ui/bmbt.c  .generated_files/flags/application/5ce91a1ae2ace7b121678590bceee97ea4bf77ad .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/ui" 
	@${RM} ${OBJECTDIR}/ui/bmbt.o.d 
	@${RM} ${OBJECTDIR}/ui/bmbt.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ui/bmbt.c  -o ${OBJECTDIR}/ui/bmbt.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/ui/bmbt.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/ui/cd53.o: ui/cd53.c  .generated_files/flags/application/d545f4c9b7341e694d6c3035a94a25aa39802f8c .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/ui" 
	@${RM} ${OBJECTDIR}/ui/cd53.o.d 
	@${RM} ${OBJECTDIR}/ui/cd53.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ui/cd53.c  -o ${OBJECTDIR}/ui/cd53.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/ui/cd53.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/ui/cli.o: ui/cli.c  .generated_files/flags/application/d261f8aa83e9e4f74b8952503f9cf9dfaba83969 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/ui" 
	@${RM} ${OBJECTDIR}/ui/cli.o.d 
	@${RM} ${OBJECTDIR}/ui/cli.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ui/cli.c  -o ${OBJECTDIR}/ui/cli.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/ui/cli.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/ui/mid.o: ui/mid.c  .generated_files/flags/application/6bf55ab1193e1c40715b7c3342d27820edea4e02 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/ui" 
	@${RM} ${OBJECTDIR}/ui/mid.o.d 
	@${RM} ${OBJECTDIR}/ui/mid.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  ui/mid.c  -o ${OBJECTDIR}/ui/mid.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/ui/mid.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/main.o: main.c  .generated_files/flags/application/a03a8bef7abd6872a47971c0ada46d569ca5b4d1 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  main.c  -o ${OBJECTDIR}/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/main.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/handler.o: handler.c  .generated_files/flags/application/cac2f679054a8975d3294799f048b51a6cf0b321 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/handler.o.d 
	@${RM} ${OBJECTDIR}/handler.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  handler.c  -o ${OBJECTDIR}/handler.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/handler.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
${OBJECTDIR}/upgrade.o: upgrade.c  .generated_files/flags/application/82ceead13dfceab9dbe16f4ca09fe40c21d20db9 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}" 
	@${RM} ${OBJECTDIR}/upgrade.o.d 
	@${RM} ${OBJECTDIR}/upgrade.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  upgrade.c  -o ${OBJECTDIR}/upgrade.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MP -MMD -MF "${OBJECTDIR}/upgrade.o.d"      -mno-eds-warn  -g -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -mlarge-code -O1 -msmart-io=1 -Wall -msfr-warn=off    -mdfp="${DFP_DIR}/xc16"
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/lib/sfr_setters.o: lib/sfr_setters.s  .generated_files/flags/application/f97ebe1d774f93300f18a19e4eb1e24ca929caa0 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/sfr_setters.o.d 
	@${RM} ${OBJECTDIR}/lib/sfr_setters.o 
	${MP_CC} $(MP_EXTRA_AS_PRE)  lib/sfr_setters.s  -o ${OBJECTDIR}/lib/sfr_setters.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -D__DEBUG   -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  -Wa,-MD,"${OBJECTDIR}/lib/sfr_setters.o.d",--defsym=__MPLAB_BUILD=1,--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,,-g,--no-relax$(MP_EXTRA_AS_POST)  -mdfp="${DFP_DIR}/xc16"
	
else
${OBJECTDIR}/lib/sfr_setters.o: lib/sfr_setters.s  .generated_files/flags/application/2d2bb16a252980c1a39680802cb2e562e5b9a093 .generated_files/flags/application/5a4ad4fd7ae8d0ceff73ee8bddd3fb0fd736ed46
	@${MKDIR} "${OBJECTDIR}/lib" 
	@${RM} ${OBJECTDIR}/lib/sfr_setters.o.d 
	@${RM} ${OBJECTDIR}/lib/sfr_setters.o 
	${MP_CC} $(MP_EXTRA_AS_PRE)  lib/sfr_setters.s  -o ${OBJECTDIR}/lib/sfr_setters.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  -Wa,-MD,"${OBJECTDIR}/lib/sfr_setters.o.d",--defsym=__MPLAB_BUILD=1,-g,--no-relax$(MP_EXTRA_AS_POST)  -mdfp="${DFP_DIR}/xc16"
	
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
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/application.${IMAGE_TYPE}.${OUTPUT_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -D__DEBUG=__DEBUG   -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)   -mreserve=data@0x800:0x81B -mreserve=data@0x81C:0x81D -mreserve=data@0x81E:0x81F -mreserve=data@0x820:0x821 -mreserve=data@0x822:0x823 -mreserve=data@0x824:0x827 -mreserve=data@0x82A:0x84F   -Wl,--local-stack,,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,-D__DEBUG=__DEBUG,,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--cref,--warn-section-align,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml$(MP_EXTRA_LD_POST)  -mdfp="${DFP_DIR}/xc16" 
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/application.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   application.gld
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/application.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -omf=elf -DXPRJ_application=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -Wl,--local-stack,,--defsym=__MPLAB_BUILD=1,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--cref,--warn-section-align,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml$(MP_EXTRA_LD_POST)  -mdfp="${DFP_DIR}/xc16" 
	${MP_CC_DIR}/xc16-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/application.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} -a  -omf=elf   -mdfp="${DFP_DIR}/xc16" 
	
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
