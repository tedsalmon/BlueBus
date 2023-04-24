#!perl
# Usage: 
#
# in your terminal run and treat it as any tool that processes text files and pipes:
#
# ./bb-log-parse.pl [... parameters ...] your_blubus_session.log
#
# optional parameters
# --time		Try provide real-time with event lines based on time set by IKE
# --no-time		... or not 
#
# --raw			Print original full line from source
# --no-raw		... or not
#
# --payload		Always provide the raw command payload, even for parsed events
# --no-payload	... or not. Original payload is provided only for functions where we do not have parser
#
# --unprocessed	Also return lines that we cannot parse, eg. Debug messages, warnings, terminal input
# --no-unprocessed	Lines we cannot parse are not returned at all.
#
# --stats		Return simple statistics about commands and payload lengths, devices talking on a bus
#
# --ignore-commands
# -i	
#				Ignores default set of noise commands
#
# --ignore-commands=cmd1,cmd2 --ignore-command=03
#				Ignore specified commands by name or hex ID, comma separate, or multiple
#
# --ignore-device=dev1,dev2 --ignore-device=FF
#				Ignore listed devices, specified by name, or hex ID
#
# Examples:
# parse the file adding time and ignoring non bus messages, page and interactively search:
# ./bb-log-parse.pl your_blubus_session.log | more
#
# find all messages to GT and return few lines around it:
# ./bb-log-parse.pl your_blubus_session.log | grep -a --context=5 ' -> GT'
#
# parse the file and store it to new file:
# ./bb-log-parse.pl your_blubus_session.log > parsed_log.txt
#
# translate logs as they appear in live session:
# 1. start your "screen" command with -L 
#    screen -L /dev/tty.usbserial-AB0M7T7E 115200
#
# 2. in new terminal window, setup the file logging to be instant
#    screen -r -x -p0 -X logfile flush 0
#
# 3. watch the live log ( filename may be different - but similar ) - notice the dash (-) at the end of command - important for PIPE usage
#    tail -f screenlog.0 | ./bb-log-parse.pl 
#

use strict;
use DateTime;
use Getopt::Long;
use Data::Dumper;

# return also real local time of events, when possible
my $config_local_time = 1;
my $config_original_line = 0;
my $config_original_data = 0;
my $config_nonparsed_lines = 1;
my $config_stats = 0;
my @ignore_devices = ( );
my @ignore_commands = ( );

GetOptions(
	"time!"			=> \$config_local_time,
	"raw!"			=> \$config_original_line,
	"payload!"		=> \$config_original_data,
	"unprocessed!" 	=> \$config_nonparsed_lines,
	"stats!"		=> \$config_stats,
	"ignore-device=s"	=> \@ignore_devices,
	"ignore-commands|i:s" => \@ignore_commands,
);
@ignore_commands = split(/,/,join(',',@ignore_commands));
@ignore_devices = split(/,/,join(',',@ignore_devices));

if (in_array('', \@ignore_commands)) {
	push(@ignore_commands, ( 
		'RAD_TMC_REQUEST', 
		'RAD_TMC_RESPONSE', 
		'TEL_TELEMATICS_LOCATION', 
		'TEL_TELEMATICS_COORDINATES', 
		'RAD_BMW_ASSIST_DATA', 
		'TEL_BMW_ASSIST_DATA', 
		'GM_RLS_STATUS', 
		'LCM_RLS_STATUS', 
		'IKE_BROADCAST_SPEED_RPM_UPDATE', 
		'IKE_BROADCAST_TEMP_UPDATE', 
		'LCM_BROADCAST_INSTRUMENT_BACKLIGHTING',
		'LCM_BROADCAST_INDICATORS_RESP',
		'GM_BROADCAST_DOORS_STATUS_RESP',
		'IKE_BROADCAST_SENSOR_RESP',
		'RAD_BROADCAST_STATUS_RESP',
		'BC127_AVRCP_MEDIA_RESPONSE',
		'IKE_83_UNK'
	));
};

# end of configuration
###########################################################

my $time_offset = 0;

my %counters_commands;
my %counters_devices;
my %counters_payload_size;

my %bus = (
	"00" => "GM",	# Body module
	"08" => "SDH",	# Tilt/Slide Sunroof
	"18" => "CDC",	# CD Changer
	"24" => "HKM",	# Trunk Lid Module
	"28" => "FUH",	# Radio controlled clock
	"2E" => "EDC",	# Electronic Damper Control
	"30" => "CCM",	# Check control module
	"3B" => "GT",	# Graphics driver (in navigation system)
	"3F" => "DIA",	# Diagnostic
	"40" => "FBZV", # Remote Control for Central Locking [E38]
	"43" => "GT2",	# Graphics driver for rear screen (in navigation system)
	"44" => "EWS",	# EWS (Immobiliser)
	"45" => "DWA",	# Anti-Theft System (DWA3, DWA4)
	"47" => "RCM",	# Rear Compartment Monitor (FOND_BT) [E38]
	"46" => "CID",	# Central information display (flip-up LCD screen)
	"50" => "MFL",	# Multi function steering wheel
	"51" => "MMP",	# Mirror Memory (Passenger) [ZKE5]
	"53" => "MUL",	# Multicast, broadcast address
	"57" => "LWS",	# Steering Angle Sensor (LWS) [D-BUS]
	"5B" => "IHKA",	# HVAC
	"60" => "PDC",	# Park Distance Control
	"66" => "ALC",	# Active Light Control
	"68" => "RAD",	# Radio
	"69" => "EKM",	# Electronic Body Module
	"6A" => "DSP",	# DSP
	"6B" => "HEAT",	# Webasto
	"70" => "RDC",	# Tire Pressure Control/Warning (RDC/W)
	"71" => "SM0",	# Seat memory - 0
	"72" => "SMD",	# Seat Memory (Driver) [SM] [ZKE5]
	"73" => "SDRS",	# Sirius Radio
	"76" => "CDCD",	# CD changer, DIN size.
	"7F" => "NAV",	# Navigation (Europe)
	"80" => "IKE",	# Instrument cluster electronics
	"86" => "XENR", # Xenon Light Right [E46?]
	"98" => "XENL", # Xenon Light Left [E46?]
	"9B" => "MMD",	# Mirror Memory (Driver) [seat control, driver (SBFA)] [ZKE5]
	"9C" => "CVM",	# The Convertible Top Module (CVM) [ZKE5]
	"9E" => "RPS",	# Roll-over Protection System (RPS) [D-BUS] [E46?]
	"A0" => "FMID",	# Rear Multi-info display
	"A4" => "MRS",	# Multiple Restraint System
	"A6" => "CC",	# GR2, FGR2, FGR2_5, FGR_KW
	"A7" => "FHK",	# Rear compartment heating/air conditioning [E38]
	"AC" => "EHC",	# Electronic Height Control (EHC), Self Leveling Suspension (SLS)
	"B0" => "SES",	# Speech Input System
	"B9" => "RC",	# compact radio/IR remote control (FUNKKOMP, IRS_KOMP)
	"BB" => "NAVJ",	# Navigation (Japan)
	"BF" => "GLO",	# Global, broadcast address
	"C0" => "MID",	# Multi-info display
	"C2" => "SVT",	# Servotronic for E83
	"C8" => "TEL",	# Telephone
	"CA" => "TCU",	# BMW Assist
	"D0" => "LCM",	# Light control module
	"DA" => "SMB",	# Seat Memory (Passenger)
	"E0" => "IRIS",	# Integrated radio information system
	"E7" => "ANZV",	# Displays Multicast
	"E8" => "RLS",	# Rain/Driving Light Sensor
	"EA" => "DSPC",	# DSP Controler
	"ED" => "VMTV",	# Video Module, TV
	"F0" => "BMBT",	# On-board monitor
	"F5" => "SZM",	# Center Console Switch Center (SZM) [E38, E46], LKM2
	"FF" => "LOC"	# Local
);

my @broadcast_addresses = ("LOC", "GLO", "GLOH", "GLOL", "MUL", "ANZV");

my %cmd_ibus = (
	"00" => "GET_STATUS",
	"01" => "STATUS_REQ",
	"02" => "STATUS_RESP",
	"07" => "PDC_STATUS",
	"0B" => "DIA_STATUS",
	"0C" => "DIA_JOB_REQUEST",
	"10" => "IGN_STATUS_REQ",
	"11" => "IGN_STATUS_RESP",
	"12" => "SENSOR_REQ",
	"13" => "SENSOR_RESP",
	"14" => "REQ_VEHICLE_TYPE",
	"15" => "RESP_VEHICLE_CONFIG",
	"16" => "ODO_REQUEST",
	"17" => "ODO_RESPONSE",
	"18" => "SPEED_RPM_UPDATE",
	"19" => "TEMP_UPDATE",
	"1A" => "DISPLAY_GONG",
	"1B" => "STATUS",
	"1C" => "GONG",
	"1D" => "TEMP_REQUEST",
	"1F" => "GPS_TIMEDATE",
	"20" => "MODE",
	"GT_20" => "GT_CHANGE_UI_REQ",
	"21" => "MAIN_MENU",
	"GT_21" => "GT_WRITE_MENU", 
	"TEL_21" => "TEL_MAIN_MENU",
	"RAD_21" => "RAD_C43_SCREEN_UPDATE",
	"MID_21" => "RAD_WRITE_MID_MENU",
	"22" => "WRITE_RESPONSE",
	"23" => "WRITE_TITLE",
	"IKE_23" => "IKE_WRITE_TITLE",
	"GT_23" => "GT_WRITE_TITLE",
	"TEL_23" => "TEL_TITLETEXT",
	"RAD_23" => "RAD_UPDATE_MAIN_AREA",
	"24" => "OBC_TEXT",
	"27" => "SET_MODE",
	"2A" => "OBC_STATUS",
	"2B" => "LED_STATUS",
	"2C" => "TEL_STATUS",
	"31" => "MENU_SELECT",
	"32" => "VOLUME",
	"36" => "CONFIG_SET",
	"37" => "DISPLAY_RADIO_TONE_SELECT",
	"38" => "REQUEST",
	"39" => "RESPONSE",
	"3B" => "BTN_PRESS",
	"40" => "OBC_INPUT",
	"41" => "OBC_CONTROL",
	"42" => "OBC_REMOTE_CONTROL",
	"44" => "WRITE_NUMERIC",
	"45" => "SCREEN_MODE_SET",
	"46" => "SCREEN_MODE_REQUEST",
	"47" => "SOFT_BUTTON",
	"48" => "BUTTON",
	"49" => "DIAL_KNOB",
	"4A" => "LED_TAPE_CTRL",
	"4E" => "TV_STATUS",
	"4F" => "MONITOR_CONTROL",
	"53" => "REQ_REDUNDANT_DATA",
	"54" => "RESP_REDUNDANT_DATA",
	"55" => "REPLICATE_REDUNDANT_DATA",
	"58" => "RLS_STATUS",
	"59" => "RLS_STATUS",
	"5A" => "INDICATORS_REQ",
	"5B" => "INDICATORS_RESP",
	"5C" => "INSTRUMENT_BACKLIGHTING",
	"5D" => "INSTRUMENT_BACKLIGHTING_REQUEST",
	"60" => "WRITE_INDEX",
	"61" => "WRITE_INDEX_TMC",
	"62" => "WRITE_ZONE",
	"63" => "WRITE_STATIC",
	"72" => "KEYLESS_STATUS",
	"74" => "IMMOBILISER_STATUS",
	"75" => "RLS_REQUEST",
	"76" => "VIS_ACK",
	"77" => "RLS_RESPONSE",
	"79" => "DOORS_STATUS_REQUEST",
	"7A" => "DOORS_STATUS_RESP",
	"9F" => "DIA_DIAG_TERMINATE",
	"A0" => "DIA_DIAG_RESPONSE",
	"A1" => "DIA_DIAG_RESPONSE_BUSY",
	"A2" => "TELEMATICS_COORDINATES",
	"A4" => "TELEMATICS_LOCATION",
	"A5" => "WRITE_WITH_CURSOR",
	"A7" =>	"TMC_REQUEST",
	"A8" =>	"TMC_RESPONSE",
	"A9" =>	"BMW_ASSIST_DATA",
	"AA" => "NAV_CONTROL_REAR",
	"AB" => "NAV_CONTROL_FRONT",
	"C0" => "C43_SET_MENU_MODE",
	"D4" => "DYNAMIC_STATIONS"
);

my %cmd_bm83 = (
	"BM83_CMD_00" => "Make_Call",
	"BM83_CMD_01" => "Make_Extension_Call",
	"BM83_CMD_02" => "MMI_Action",
	"BM83_CMD_03" => "Event_Mask_Setting",
	"BM83_CMD_04" => "Music_Control",
	"BM83_CMD_05" => "Change_Device_Name",
	"BM83_CMD_06" => "Change_PIN_Code",
	"BM83_CMD_07" => "BTM_Parameter_Setting",
	"BM83_CMD_08" => "Read_BTM_Version",
	"BM83_CMD_0A" => "Vendor_AT_Command",
	"BM83_CMD_0B" => "AVC_Vendor_Dependent_Cmd",
	"BM83_CMD_0C" => "AVC_Group_Navigation",
	"BM83_CMD_0D" => "Read_Link_Status",
	"BM83_CMD_0E" => "Read_Paired_Device_Record",
	"BM83_CMD_0F" => "Read_Local_BD_Address",
	"BM83_CMD_10" => "Read_Local_Device_Name",
	"BM83_CMD_12" => "Send_SPP/iAP_Or _LE_Data",
	"BM83_CMD_13" => "BTM_Utility_Function",
	"BM83_CMD_14" => "Event_ACK",
	"BM83_CMD_15" => "Additional_Profiles_Link_Setup",
	"BM83_CMD_16" => "Read_Linked_Device_Information",
	"BM83_CMD_17" => "Profiles_Link_Back",
	"BM83_CMD_18" => "Disconnect",
	"BM83_CMD_19" => "MCU_Status_Indication",
	"BM83_CMD_1A" => "User_Confirm_SPP_Req_Reply",
	"BM83_CMD_1B" => "Set_HF_Speaker_Gain_Level",
	"BM83_CMD_1C" => "EQ_Mode_Setting",
	"BM83_CMD_1D" => "DSP_NR_CTRL",
	"BM83_CMD_1E" => "GPIO__Control",
	"BM83_CMD_1F" => "MC_UART_Rx_Buffer_Size",
	"BM83_CMD_20" => "Voice_Prompt_Cmd",
	"BM83_CMD_23" => "Set_Overall_Gain",
	"BM83_CMD_24" => "Read_BTM_Setting",
	"BM83_CMD_25" => "Read_BTM_Battery__Charge_Status",
	"BM83_CMD_26" => "MCU_Update_Cmd",
	"BM83_CMD_27" => "Report_Battery_Capacity",
	"BM83_CMD_28" => "LE_ANCS_Service_Cmd",
	"BM83_CMD_29" => "LE_Signaling_Cmd",
	"BM83_CMD_2A" => "MSPK_Vendor_Cmd",
	"BM83_CMD_2B" => "Read_MSPK_Link_Status",
	"BM83_CMD_2C" => "MSPK_Sync_Audio_Effect",
	"BM83_CMD_2D" => "LE__GATT_CMD",
	"BM83_CMD_2F" => "LE_App_CMD",
	"BM83_CMD_30" => "Dsp_Runtime_Program",
	"BM83_CMD_31" => "Read_Vendor_Stored_Data",
	"BM83_CMD_32" => "Read_IC_Version_linfo",
	"BM83_CMD_34" => "Read_BTM_Link_Mode",
	"BM83_CMD_35" => "Configure_Vendor_Parameter",
	"BM83_CMD_37" => "MSPK_Exchange _Link _Info_Cmd",
	"BM83_CMD_38" => "MSPK_Set_GIAC",
	"BM83_CMD_39" => "Read_Feature_List",
	"BM83_CMD_3A" => "Personal_MSPK_GROUP_Control",
	"BM83_CMD_3B" => "Test_Device",
	"BM83_CMD_3C" => "Read_EEPROM_Data",
	"BM83_CMD_3D" => "Write_EEPROM_Data",
	"BM83_CMD_3E" => "LE_Signaling2_Cmd",
	"BM83_CMD_3F" => "PBAPC_Cmd",
	"BM83_CMD_40" => "TWS_CMD",
	"BM83_CMD_41" => "AVRCP_Browsing_Cmd",
	"BM83_CMD_42" => "Read_Paired_Link_Key_lifo",
	"BM83_CMD_44" => "Audio_Transceiver_Cmd",
	"BM83_CMD_46" => "Button_MMI_Setting_Cmd",
	"BM83_CMD_47" => "Button_Operation_Cmd",
	"BM83_CMD_48" => "Read_Button_MMI_Setting_Cmd",
	"BM83_CMD_49" => "DFU",
	"BM83_CMD_4A" => "AVRCP_Vendor_Dependent_Cmd",
	"BM83_CMD_4B" => "Concert_Mode_Endless_Grouping",
	"BM83_CMD_4C" => "Read_Runtime_Latency",
	"BM83_CMD_CC" => "Toggle_Audio_Source",

	"BM83_EVT_00" => "Command_ACK",
	"BM83_EVT_01" => "BTM_Status",
	"BM83_EVT_02" => "Call_Status",
	"BM83_EVT_03" => "Caller_ID",
	"BM83_EVT_04" => "SMS_Received_Indication",
	"BM83_EVT_05" => "Missed_Call_Indication",
	"BM83_EVT_06" => "Phone_Max_Battery_Level",
	"BM83_EVT_07" => "Phone_Current_Battery_Level",
	"BM83_EVT_08" => "Roaming_Status",
	"BM83_EVT_09" => "Phone_Max_Signal_Strength_Level",
	"BM83_EVT_0A" => "Phone_Current_Signal_Strength_Level ",
	"BM83_EVT_0B" => "Phone_Service_Status",
	"BM83_EVT_0C" => "BTM_Battery_Status",
	"BM83_EVT_0D" => "BTM_Charging_Status",
	"BM83_EVT_0E" => "Reset_To_Default",
	"BM83_EVT_0F" => "Report_HF_Gain_Level",
	"BM83_EVT_10" => "EQ_Mode_Indication",
	"BM83_EVT_17" => "Read_Linked_Device_Information_Reply",
	"BM83_EVT_18" => "Read_BTM_Version_Reply",
	"BM83_EVT_19" => "Call_List_Report",
	"BM83_EVT_1A" => "AVC_Specific_Rsp",
	"BM83_EVT_1B" => "BTM_Utility_Req",
	"BM83_EVT_1C" => "Vendor_AT_Cmd_Rsp",
	"BM83_EVT_1E" => "Read_Link_Status_Reply",
	"BM83_EVT_1F" => "Read_Paired_Device_Record_Reply",
	"BM83_EVT_20" => "Read_Local_BD_Address_Reply",
	"BM83_EVT_22" => "Report_SPP/iAP_Data",
	"BM83_EVT_23" => "Report_Link_Back_Status",
	"BM83_EVT_24" => "Report_Ring_Tone_Status",
	"BM83_EVT_26" => "Report_AVRCP_Vol_Ctrl",
	"BM83_EVT_28" => "Report_iAP_Info",
	"BM83_EVT_2A" => "Report_Voice_Prompt_Status",
	"BM83_EVT_2D" => "Report_Type_Codec",
	"BM83_EVT_2E" => "Report_Type_BTM_Setting ",
	"BM83_EVT_30" => "Report_BTM_Initial_Status ",
	"BM83_EVT_32" => "LE_Signaling_Event",
	"BM83_EVT_33" => "Report_MSPK_Link_Status",
	"BM83_EVT_34" => "Report_MSPK_Vendor_Event ",
	"BM83_EVT_35" => "Report_MSPK_Audio_Setting",
	"BM83_EVT_36" => "Report_Sound_Effect_Status ",
	"BM83_EVT_37" => "Report_Vendor_Stored_Data",
	"BM83_EVT_38" => "Report_IC_Version_Info ",
	"BM83_EVT_39" => "Report_LE_GATT_Event",
	"BM83_EVT_3A" => "Report_BTM_Link_Mode ",
	"BM83_EVT_3C" => "Reserved",
	"BM83_EVT_3D" => "Report_MSPK_Exchange_Link_Info",
	"BM83_EVT_3E" => "Report_Customized_Information ",
	"BM83_EVT_3F" => "Report_CSB_CLK",
	"BM83_EVT_40" => "Report_Read_Feature_List_Reply ",
	"BM83_EVT_41" => "Report_Test_Result_Reply",
	"BM83_EVT_42" => "Report_Read_EEPROM_Data ",
	"BM83_EVT_43" => "PBAPC_Event",
	"BM83_EVT_44" => "AVRCP_Browsing_Event",
	"BM83_EVT_45" => "Report_Paired_Link_Key_Info",
	"BM83_EVT_53" => "Report_TWS_Rx_Vendor_Event",
	"BM83_EVT_54" => "Report_TWS_Local_Device_Status ",
	"BM83_EVT_55" => "Report_TWS_VAD_Data",
	"BM83_EVT_56" => "Report_TWS_Radio_Condition",
	"BM83_EVT_57" => "Report_TWS_Ear_Bud_Position",
	"BM83_EVT_58" => "Report_TWS_Secondary_Device_Status ",
	"BM83_EVT_59" => "Reserved",
	"BM83_EVT_5A" => "Audio_Transceiver_Event_Status",
	"BM83_EVT_5C" => "Read_Button_MMI_Setting_Reply",
	"BM83_EVT_5D" => "AVRCP_Vendor_Dependent_Rsp",
	"BM83_EVT_5E" => "Runtime_Latency",
);

my %data_parsers = (
	"BMBT_STATUS_RESP" => \&ibus_data_parsers_module_status,
	"TEL_STATUS_RESP" => \&ibus_data_parsers_module_status,
	"NAVE_STATUS_RESP" => \&ibus_data_parsers_module_status,
	"GT_STATUS_RESP" => \&ibus_data_parsers_module_status,

	"BMBT_MONITOR_CONTROL" => \&ibus_data_parsers_monitor_control,

	"TEL_BTN_PRESS" => \&ibus_data_parsers_mfl_buttons,
	"RAD_BTN_PRESS" => \&ibus_data_parsers_mfl_buttons,

	"GT_BUTTON" => \&ibus_data_parsers_bmbt_buttons,
	"BMBT_BROADCAST_BUTTON" => \&ibus_data_parsers_bmbt_buttons,
	"RAD_BUTTON" => \&ibus_data_parsers_bmbt_buttons,

	"BMBT_SOFT_BUTTON" => \&ibus_data_parsers_bmbt_soft_buttons,

	"RAD_VOLUME" => \&ibus_data_parsers_volume,
	"TEL_VOLUME" => \&ibus_data_parsers_volume,

	"BMBT_DIAL_KNOB" => \&ibus_data_parsers_navi_knob,
	"GT_DIAL_KNOB" => \&ibus_data_parsers_navi_knob,

	"GT_SCREEN_MODE_REQUEST" => \&ibus_data_parsers_request_screen,
	"RAD_SCREEN_MODE_SET" => \&ibus_data_parsers_set_radio_ui,

	"GT_WRITE_WITH_CURSOR" => \&ibus_data_parsers_gt_write,
	"GT_WRITE_MENU" => \&ibus_data_parsers_gt_write,

	"GT_WRITE_TITLE" =>  \&ibus_data_parsers_gt_write_menu,
	"IKE_WRITE_TITLE" =>  \&ibus_data_parsers_gt_write_menu,
	"RAD_BROADCAST_WRITE_TITLE" =>  \&ibus_data_parsers_gt_write_menu,

	"GT_DYNAMIC_STATIONS" => \&ibus_data_parsers_gt_dynamic_stations,

	"IKE_WRITE_NUMERIC" => \&ibus_data_parsers_write_numeric,

	"CDC_RESPONSE" => \&ibus_data_parsers_cdc_response,
	"CDC_REQUEST" => \&ibus_data_parsers_cdc_request,

	"IKE_OBC_INPUT" => \&ibus_data_parsers_ike_obc_input,
	"IKE_BROADCAST_OBC_TEXT" => \&ibus_data_parsers_obc_text,
	"IKE_BROADCAST_SPEED_RPM_UPDATE" => \&ibus_data_parsers_speed_rpm,
	"IKE_BROADCAST_TEMP_UPDATE" => \&ibus_data_parsers_ike_temp,
	"IKE_BROADCAST_SENSOR_RESP" => \&ibus_data_parsers_ike_sensor,
	"IKE_BROADCAST_IGN_STATUS_RESP" => \&ibus_data_parsers_ike_ignition,
	"IKE_BROADCAST_REPLICATE_REDUNDANT_DATA" => \&ibus_data_parsers_redundant_data,
	"IKE_BROADCAST_ODO_RESPONSE" => \&ibus_data_parsers_ike_odo_response,
	"LCM_RESP_REDUNDANT_DATA" => \&ibus_data_parsers_lcm_redundant_data,
	"LCM_BROADCAST_INDICATORS_RESP" =>\&ibus_data_parsers_lcm_indicator_resp,

	"IKE_GPS_TIMEDATE" => \&ibus_data_parsers_ike_gps_time,

	"GM_BROADCAST_DOORS_STATUS_RESP" => \&ibus_data_parsers_doors_status,

	"BM83_CMD_Event_ACK" => \&bm83_data_parsers_event_ack,
	"BM83_EVT_Command_ACK" => \&bm83_data_parsers_command_ack,
	"BM83_EVT_BTM_Status" => \&bm83_data_parsers_bmt_status,

	"BM83_EVT_Call_Status" => \&bm83_data_parsers_call_status,
	"BM83_EVT_Caller_ID" => \&bm83_data_parsers_caller_id,

	"BM83_EVT_Read_Linked_Device_Information_Reply" => \&bm83_data_parsers_device_info,
	"BM83_EVT_Read_Paired_Device_Record_Reply" => \&bm83_data_parsers_paired_device_info,
	"BM83_EVT_Report_Link_Back_Status" => \&bm83_data_parsers_link_back_status,

	"BM83_EVT_AVC_Specific_Rsp" => \&bm83_data_parsers_avc_spec_response,
	"BM83_EVT_AVRCP_Browsing_Event" => \&bm83_data_parsers_avrcp_browsing,
	"BM83_EVT_AVRCP_Vendor_Dependent_Rsp" => \&bm83_data_parsers_avrcp_vendor_dep,

	"BM83_CMD_Music_Control" => \&bm83_data_parsers_music_control,
	"BM83_CMD_MMI_Action" => \&bm83_data_parsers_mmi_action,
	"BM83_CMD_Profiles_Link_Back" => \&bm83_data_parsers_profile_link_back,
	"BM83_CMD_AVC_Vendor_Dependent_Cmd" => \&bm83_data_parsers_avc_command,
);

sub bm83_data_parsers_avc_command {
	my ($src, $dst, $string, $data) = @_;

	my $db = $data->[0];
	my $pdu = $data->[1];


	my %pdu = (
		0x10 => "GET_CAPABILITIES",
		0x11 => "LIST_PLAYER_APPLICATION_SETTING_ATTRIBUTES",
		0x12 => "LIST_PLAYER_APPLICATION_SETTING_VALUES",
		0x13 => "GET_CURRENT_PLAYER_APPLICATION_SETTING_VALUE",
		0x14 => "SET_PLAYER_APPLICATION_SETTING_VALUE",
		0x15 => "GET_PLAYER_APPLICATION_SETTING_ATTRIBUTE_TEXT",
		0x16 => "GET_PLAYER_APPLICATION_SETTING_VALUE_TEXT",
		0x17 => "INFORM_DISPLAYABLE_CHARACTER_SET",
		0x18 => "INFORM_BATTERY_STATUS_OF_CT",
		0x20 => "GET_ELEMENT_ATTRIBUTES",
		0x30 => "GET_PLAY_STATUS",
		0x31 => "REGISTER_NOTIFICATION",
		0x40 => "REQUEST_CONTINUING_RESPONSE",
		0x41 => "ABORT_CONTINUING_RESPONSE",
	);

	my $resp = lookup_value($pdu,\%pdu)." ( ";

	if ($pdu == 0x31) {
		my %notifications = (
			0x01 => "PLAYBACK_STATUS_CHANGED",
			0x02 => "TRACK_CHANGED",
			0x03 => "TRACK_REACHED_END",
			0x04 => "TRACK_REACHED_START",
			0x05 => "PLAYBACK_POS_CHANGED",
			0x06 => "BATT_STATUS_CHANGED",
			0x07 => "SYSTEM_STATUS_CHANGED",
			0x08 => "PLAYER_APPLICATION_SETTING_CHANGED",
		);
		$resp .= lookup_value($data->[5],\%notifications)." ";

	} elsif ($pdu == 0x20) {
		my $attr_start = 13;

		my %attributes = (
			0x0001	=> "TITLE",
			0x0002	=> "ARTIST",
			0x0003	=> "ALBUM",
			0x0004	=> "TRACK",
			0x0005	=> "TRACKS",
			0x0006	=> "GENRE",
			0x0007	=> "PLAYTIME_MS",
			0x0008	=> "COVER_ART"
		);

		$resp .= '[ ';
		my $count = $data->[$attr_start++];

		while ($count-->0) {
			my $attr_id = ( $data->[$attr_start] << 24 ) + ( $data->[$attr_start+1] << 16 ) + ( $data->[$attr_start+2] << 8 ) + $data->[$attr_start+3];
			$resp .= lookup_value($attr_id, \%attributes).", ";
			$attr_start+=4;
		}

		$resp .= '] ';
	} else {
		for (my $i = 5; $i<scalar(@$data); $i++ ) {
			$resp .= print_hex($data->[$i])." ";
		}
	}
	$resp .= ")";

	return $resp;

}

sub avrcp_pdu_response {
	my ( $pdu, $data, $start, $avrcp_vendor_dep ) = @_;

	my %pdu = (
		0x10 => "CAPABILITIES",
		0x11 => "PLAYER_APPLICATION_SETTING_ATTRIBUTES",
		0x12 => "PLAYER_APPLICATION_SETTING_VALUES",
		0x13 => "CURRENT_PLAYER_APPLICATION_SETTING_VALUE",
		0x14 => "PLAYER_APPLICATION_SETTING_VALUE",
		0x15 => "PLAYER_APPLICATION_SETTING_ATTRIBUTE_TEXT",
		0x16 => "PLAYER_APPLICATION_SETTING_VALUE_TEXT",
		0x17 => "DISPLAYABLE_CHARACTER_SET",
		0x18 => "BATTERY_STATUS_OF_CT",
		0x20 => "ELEMENT_ATTRIBUTES",
		0x30 => "PLAY_STATUS",
		0x31 => "NOTIFICATION",
		0x40 => "CONTINUING_RESPONSE",
		0x41 => "ABORT_CONTINUING_RESPONSE",
	);

	my $resp = lookup_value($pdu,\%pdu)." ( ";


	if ($pdu == 0x31) {
		my %notifications = (
			0x01 => "PLAYBACK_STATUS_CHANGED",
			0x02 => "TRACK_CHANGED",
			0x03 => "TRACK_REACHED_END",
			0x04 => "TRACK_REACHED_START",
			0x05 => "PLAYBACK_POS_CHANGED",
			0x06 => "BATT_STATUS_CHANGED",
			0x07 => "SYSTEM_STATUS_CHANGED",
			0x08 => "PLAYER_APPLICATION_SETTING_CHANGED",
		);

		$resp .= lookup_value($data->[$start],\%notifications);

		if ($data->[$start] == 0x01) {
			my %play_status = (
				0x00 => "STOPPED",
				0x01 => "PLAYING",
				0x02 => "PAUSED",
				0x03 => "FWD_SEEK",
				0x04 => "REV_SEEK",
				0x05 => "ERROR"
			);

			$resp .= ", status=".lookup_value($data->[$start+1], \%play_status);
		} elsif ($data->[$start] == 0x02) {
			$resp .= ", track=";
			for (my $i = $start+1; $i<$start+1+9; $i++ ) {
				$resp .= print_hex($data->[$i]);
			};
		} elsif ($data->[$start] == 0x05) {
			my $pos_ms = ( $data->[$start+1] << 24 ) + ( $data->[$start+2] << 16 ) + ( $data->[$start+3] << 8 ) + $data->[$start+4];

			if ($pos_ms =! 0xFFFFFFFF) {
				$resp .= ', pos='.print_time($pos_ms, 1, 1);
			} else {
				$resp .= ', pos=UNKNOWN';
			}
		} elsif ($data->[$start] == 0x06 || $data->[$start] == 0x07 ) {
			$resp .= ', status='.print_hex($data->[$start+1]);
		} else {
			$resp .= ' ( ';
			for (my $i = $start+1; $i<scalar(@$data); $i++ ) {
				$resp .= print_hex($data->[$i])." ";
			}
			$resp .= ') ';
		}

	} elsif ($pdu == 0x20) {
		my $num_attr = $data->[$start];

		my %attributes = (
			0x0001	=> "TITLE",
			0x0002	=> "ARTIST",
			0x0003	=> "ALBUM",
			0x0004	=> "TRACK",
			0x0005	=> "TRACKS",
			0x0006	=> "GENRE",
			0x0007	=> "PLAYTIME_MS",
			0x0008	=> "COVER_ART"
		);

		my %charsets = (

			0x0003	=> "ASCII",
			0x0004	=> "ISO_8859_1",
			0x000F	=> "JIS_X0201",
			0x0011	=> "SHIFT_JIS",
			36		=> "KS_C_5601_1987",
			0x006A  => "UTF8",
			1013	=> "UTF16_BE",
			2025	=> "GB2312",
			2026	=> "BIG5"
		);

		$resp .= ' [';
		my $attr = 0;
		my $attr_start = $start+1;

		if ($avrcp_vendor_dep) {
			$attr_start += 2;
		}

		while ($attr<$num_attr) {
			my $attr_id = ( $data->[$attr_start] << 24 ) + ( $data->[$attr_start+1] << 16 ) + ( $data->[$attr_start+2] << 8 ) + $data->[$attr_start+3];
			my $attr_charset = ($data->[$attr_start+4] << 8) + $data->[$attr_start+5];
			my $attr_len = ($data->[$attr_start+6] << 8) + $data->[$attr_start+7];

			my $value = cleanup_string(array_to_string($data, $attr_start+8, $attr_len));
			if ($attr_id == 0x0007) {
				$value = print_time($value,1,1);
			}
			$resp .= sprintf("{ %s=\"%s\", attr_charset=%s }, ", lookup_value($attr_id,\%attributes), $value, lookup_value($attr_charset,\%charsets));
			$attr++;
			$attr_start += 8+$attr_len;
		}
		$resp .= ' ] ';


	} else {
		for (my $i = $start; $i<scalar(@$data); $i++ ) {
			$resp .= print_hex($data->[$i])." ";
		}
	};

	$resp .= ') ';

	return $resp;
}

sub bm83_data_parsers_avrcp_vendor_dep {
	my ($src, $dst, $string, $data) = @_;

	my $pdu = $data->[0];
	my $db = $data->[1];
	my $resp = $data->[2];
	my $full_packer = $data->[3];

	my %responses = (
		0x08 => "NOT_IMPLEMENTED",
		0x09 => "ACCEPT",
		0x0A => "REJECT",
		0x0C => "STABLE",
		0x0D => "CHANGED",
		0x0F => "INTERIM"
	);

	return "db=$db, resp=".lookup_value($resp,\%responses).", pdu=".avrcp_pdu_response($pdu, $data, 4, 1);
}

sub bm83_data_parsers_avc_spec_response {
	my ($src, $dst, $string, $data) = @_;

	my $db = $data->[0];
	my $resp = $data->[1];

	my $subunit_type = ($data->[2] & 0b1111_1000 ) >> 3;
	my $subunit_id = ($data->[2] & 0b0000_0111 );

	my $opcode = $data->[3];

	my @company_id = ( $data->[4], $data->[5], $data->[6] );
	my $pdu = $data->[7];
	my $packet_length = ($data->[9] << 8) + $data->[10];


	my %responses = (
		0x08 => "NOT_IMPLEMENTED",
		0x09 => "ACCEPT",
		0x0A => "REJECT",
		0x0C => "STABLE",
		0x0D => "CHANGED",
		0x0F => "INTERIM"
	);

	my $resp = "db=$db, resp=".lookup_value($resp,\%responses).", pdu=".avrcp_pdu_response($pdu, $data, 11);

	return $resp;
};


my @avrcp_browsing_packet = ();
sub bm83_data_parsers_avrcp_browsing {
	my ($src, $dst, $string, $data) = @_;
	
	my $type = $data->[0];
	my $total_length = ($data->[1] << 8) + $data->[2];
	my $packet_length = ($data->[3] << 8) + $data->[4];

	if (($type == 0x00)||($type == 0x01)) {
		@avrcp_browsing_packet = ();
	};

	for (my $i = 0; $i < $packet_length; $i++) {
		push @avrcp_browsing_packet, $data->[5+$i];
	}

	if (($type == 0x00) || ($type == 0x03)) {
# single or last packet

		my $subevent = $avrcp_browsing_packet[0];
		my $db = $avrcp_browsing_packet[1];

		my %subevents = (
			0x08 => "ADD_TO_NOW_PLAYING_RSP",
			0x09 => "GENERAL_REJECT_RSP",
			0x0A => "NOW_PLAYING_CONTENT_CHANGED",
			0x0B => "AVAILABLE_PLAYER_CHANGED",
			0x0C => "ADDRESSED_PLAYER_CHANGED",
			0x0D => "UIDS_CHANGED",
			0x0E => "CONNECTION_STATUS",
		);

		my %states = (
			0x00 => "INVALID_COMMAND",
			0x01 => "INVALID_PARAMETER",
			0x02 => "PARAMETER_CONTENT_ERROR",
			0x03 => "INTERNAL_ERROR",
			0x04 => "OPERATION_COMPLETED_WITHOUT_ERROR",
			0x05 => "UID_CHANGED",
			0x06 => "RESERVED",
			0x07 => "INVALID_DIRECTION",
			0x08 => "NOT_A_DIRECTORY",
			0x09 => "DOES_NOT_EXIST",
			0x0A => "INVALID_SCOPE",
			0x0B => "RANGE_OUT_OF_BOUNDS",
			0x0C => "FOLDER_ITEM_IS_NOT_PLAYABLE",
			0x0D => "MEDIA_IN_USE",
			0x0E => "NOW_PLAYING_LIST_FULL",
			0x0F => "SEARCH_NOT_SUPPORTED",
			0x10 => "SEARCH_IN_PROGRESS",
			0x11 => "INVALID_PLAYER_ID",
			0x12 => "PLAYER_NOT_BROWSABLE",
			0x13 => "PLAYER_NOT_ADDRESSED",
			0x14 => "NO_VALID_SEARCH_RESULTS",
			0x15 => "NO_AVAILABLE_PLAYERS",
			0x16 => "ADDRESSED_PLAYER_CHANGED",
		);

		my %responses = (
			0x08 => "NOT_IMPLEMENTED",
			0x09 => "ACCEPT",
			0x0A => "REJECT",
			0x0C => "STABLE",
			0x0D => "CHANGED",
			0x0F => "INTERIM"
		);

		my $resp = (($type == 0x00)?'full_packet':'final_packet').", subevent=".lookup_value($subevent,\%subevents);

		if ($subevent == 0x02 || $subevent == 0x07 || $subevent == 0x08 ) {
			$resp .= ", db=$db, response=". lookup_value($avrcp_browsing_packet[2],\%responses).", status=".lookup_value($avrcp_browsing_packet[3],\%states);
		} elsif ($subevent == 0x09 ) {
			$resp .= ", db=$db, status=".lookup_value($avrcp_browsing_packet[2],\%states);
		} elsif ($subevent == 0x0A || $subevent == 0x0B ) {
			$resp .= ", db=$db, status=".lookup_value($avrcp_browsing_packet[2],\%responses);
		} elsif ($subevent == 0x0C) {
			$resp .= ", db=$db, status=".lookup_value($avrcp_browsing_packet[2],\%responses).", playerId=".print_hex(($avrcp_browsing_packet[3]<<8)+$avrcp_browsing_packet[4],4).", UIDCounter=".print_hex(($avrcp_browsing_packet[5]<<8)+$avrcp_browsing_packet[6],4);
		} elsif ($subevent == 0x0D) {
			$resp .= ", db=$db, status=".lookup_value($avrcp_browsing_packet[2],\%responses).", UIDCounter=".print_hex(($avrcp_browsing_packet[3]<<8)+$avrcp_browsing_packet[4],4);
		} elsif ($subevent == 0x0E) {
			$resp .= ", db=$db, status=".$avrcp_browsing_packet[2];
		} else {
			$resp .= " ( ";
			for (my $i = 1; $i<scalar(@avrcp_browsing_packet); $i++ ) {
				$resp .= print_hex($avrcp_browsing_packet[$i])." ";
			}
			$resp .= ")";
		}

		@avrcp_browsing_packet = ();
		return $resp;

	} else {
		return "patial_packet, sequence=".(($type==0x01)?'START':'MIDDLE')."total=$total_length, packet=$packet_length";
	}
};

sub bm83_data_parsers_link_back_status {
	my ($src, $dst, $string, $data) = @_;

	my $connection = $data->[0];
	my $val = $data->[1];

	my %connections = (
		0x00 => "ACL",
		0x01 => "HFP",
		0x02 => "A2DP",
		0x03 => "SPP",
	);

	my $con_r = lookup_value($connection,\%connections);

	if ($connection == 0x00) {
		if ($val == 0xFF) {
			return "connection=$con_r, FAIL";
		} else {
			my $device = ( $val & 0b0111_0000 ) >> 4;
			my $db = ( $val & 0b0000_1111 );
			return "connection=$con_r, SUCCESS, dev=$device(".(($val & 0b1111_0000 )>>4)."), db=$db";
		};
	} else {
		return "connection=$con_r, ".(($val == 0)?'SUCCSS':'FAIL');
	}


};

sub bm83_data_parsers_profile_link_back {
	my ($src, $dst, $string, $data) = @_;

	my $type = $data->[0];

	my %types = (
		0x00 => "CONNECT_LAST_DEVICE_ALL",
		0x01 => "CONNECT_LAST_DEVICE_HF",
		0x02 => "CONNECT_LAST_DEVIDCE_A2DP",
		0x03 => "CONNECT_LAST_DEVICE_SPP",
		0x04 => "CONNECT_PROFILE_DEVICE",
		0x05 => "CONNECT_PROFILE_MAC",
		0x07 => "CONNECT_PROFILE_UNPAIRED",
	);

	my $type_r = lookup_value($type, \%types);

	if ($type == 0x00 || $type == 0x01 || $type == 0x02 || $type == 0x03) {
		return "type=$type_r";
	}

	if ( $type == 0x04 || $type == 0x05 ) {
		my $dev = $data->[1];
		my $profile = $data->[2];

		my $hs = $profile & 0b0000_0001;
		my $hf = ($profile & 0b0000_0010)>>1;
		my $a2dp = ($profile & 0b0000_0100)>>2;

		my $resp = "type=$type_r, device=$dev, profile=".(($profile == 0)?'DEFAULT':(($hs?'HSP ':'').($hf?' HFP ':'').($a2dp?'A2DP':'')));

		if ($type == 0x05) {
			$resp .= ", mac=".print_mac($data, 3, 1);
		}

		return $resp;
	}

	if ($type == 0x07) {
		my $profile = $data->[1];

		my $hs = $profile & 0b0000_0001;
		my $hf = ($profile & 0b0000_0010)>>1;
		my $a2dp = ($profile & 0b0000_0100)>>2;

		return "type=$type_r, profile=".(($profile == 0)?'DEFAULT':(($hs?'HSP ':'').($hf?' HFP ':'').($a2dp?'A2DP':''))).", mac=".print_mac($data, 2, 1);
	}
}

sub bm83_data_parsers_music_control {
	my ($src, $dst, $string, $data) = @_;

	my $db = $data->[0];
	my $action = $data->[1];

	my %actions = (
		0x00 => "STOP_FF_RW",
		0x01 => "FF",
		0x02 => "FF_REPEAT",
		0x03 => "RW",
		0x04 => "RW_REPEAT",
		0x05 => "PLAY",
		0x06 => "PAUSE",
		0x07 => "PLAY_PAUSE_TOGGLE",
		0x08 => "STOP",
		0x09 => "NEXT",
		0x0A => "PREVIOUS",
	);

	return "action=".lookup_value($action,\%actions).", reserved=".print_hex($db);
};

sub bm83_data_parsers_mmi_action {
	my ($src, $dst, $string, $data) = @_;

	my $db = $data->[0];
	my $action = $data->[1];

	my %actions = (
		0x01 => "ADD_REMOVE_SCO_LINK",
		0x02 => "FORCE_END_CALL_DEPRECATED",
		0x03 => "ENABLE_DEVICE_TEST_MODE",
		0x04 => "ACCEPT_CALL",
		0x05 => "REJECT_CALL",
		0x06 => "END_CALL",
		0x07 => "TOGGLE_MIC",
		0x08 => "MUTE_MIC",
		0x09 => "ACTIVATE_MIC",
		0x0A => "VR_OPEN",
		0x0B => "VR_CLOSE",
		0x0C => "REDIAL",
		0x0D => "SWITCH_HELD_CALLS",
		0x0E => "SWITCH_HEADSET_PHONE",
		0x0F => "QUERY_CALL_LISTS",
		0x10 => "THREE_WAY_CALL",
		0x11 => "RELEASE_WAITING_CALL",
		0x12 => "ACCEPT_WAITING_HOLD_CALL",
		0x16 => "INITIATE_HF_CALL_DEPRECATED",
		0x17 => "DISCONNECT_HF_LINK",
		0x18 => "ENABLE_RX_NR",
		0x19 => "DISABLE_RX_NR",
		0x1A => "TOGGLE_RX_NR_DEPRECATED",
		0x1B => "ENABLE_TX_NR_DEPRECATED",
		0x1C => "DISABLE_TX_NR_DEPRECATED",
		0x1D => "TOGGLE_TX_NR_DEPRECATED",
		0x1E => "ENABLE_AEC_WHEN_SCO_READY",
		0x1F => "DISABLE_AEC_WHEN_SCO_READY",
		0x20 => "SWITCH_AC_ENABLE/DISABLE_WHEN_SCO_READY",
		0x21 => "ENABLE_AC_RX_NOISE_REDUCTION_WHEN_SCO_READY",
		0x22 => "DISABLE_AC_RX_NOISE_REDUCTION_WHEN_SCO_READY",
		0x23 => "SWITCH_AEC_RX_NOISE_REDUCTION_WHEN_SCO_READY",
		0x24 => "INCREASE_MICROPHONE_GAIN",
		0x25 => "DECREASE_MICROPHONE_GAIN",
		0x26 => "SWITCH_PRIMARY_HF_DEVICE_AND_SECONDARY_HF_DEVICE_ROLE",
		0x30 => "INCREASE_SPEAKER_GAIN_DEPRECATED",
		0x31 => "DECREASE_SPEAKER_GAIN_DEPRECATED",
		0x32 => "PLAY/PAUSE_MUSIC_DEPRECATED",
		0x33 => "STOP_MUSIC_DEPRECATED",
		0x34 => "NEXT_SONG_DEPRECATED",
		0x35 => "PREVIOUS_SONG_DEPRECATED",
		0x36 => "FAST_FORWARD_DEPRECATED",
		0x37 => "REWIND_DEPRECATED",
		0x38 => "EQ_MODE_UP_DEPRECATED",
		0x39 => "EQ_MODE_DOWN_DEPRECATED",
		0x3A => "LOCK_BUTTON",
		0x3B => "DISCONNECT_A2DP_LINK",
		0x3C => "NEXT_AUDIO_EFFECT",
		0x3D => "PREVIOUS_AUDIO_EFFECT",
		0x3E => "TOGGLE_3D_EFFECT_DEPRECATED",
		0x3F => "REPORT_CURRENT_EQ_MODE",
		0x40 => "REPORT_CURRENT_AUDIO_EFFECT_STATUS",
		0x41 => "TOGGLE_AUDIO_PLAYBACK",
		0x50 => "ENTER_PAIRING_MODE_FROM_POWER_OFF_STATE_DEPRECATED",
		0x51 => "POWER_ON_BUTTON_PRESS",
		0x52 => "POWER_ON_BUTTON_RELEASE",
		0x53 => "POWER_OFF_BUTTON_PRESS",
		0x54 => "POWER_OFF_BUTTON_RELEASE",
		0x55 => "RESERVED",
		0x56 => "RESET_SOME_EEPROM_SETTING_TO_DEFAULT_SETTING",
		0x57 => "FORCE_SPEAKER_GAIN_TOGGLE",
		0x58 => "TOGGLE_BUTTON_INDICATION",
		0x59 => "COMBINE_FUNCTION_0",
		0x5A => "COMBINE_FUNCTION_1",
		0x5B => "COMBINE_FUNCTION_2",
		0x5C => "COMBINE_FUNCTION_3",
		0x5D => "FAST_ENTER_PAIRING_MODE_FROM_NON-OFF_MODE",
		0x5E => "SWITCH_POWER_OFF",
		0x5F => "DISABLE_LED",
		0x60 => "TOGGLE_BUZZER",
		0x61 => "DISABLE_BUZZER",
		0x62 => "ENABLE_BUZZER",
		0x63 => "CHANGE_TONE_SET_SPK_MODULE_SUPPORT_TWO_SETS_OF_TONE",
		0x64 => "RETRIEVE_PHONEBOOK_DEPRECATED",
		0x65 => "RETRIEVE_MCH_DEPRECATED",
		0x66 => "RETRIEVE_ICH_DEPRECATED",
		0x67 => "RETRIEVE_OCH_DEPRECATED",
		0x68 => "RETRIEVE_CCH_DEPRECATED",
		0x69 => "CANCEL_ACCESS_PBAP_DEPRECATED",
		0x6A => "INDICATE_BATTERY_STATUS",
		0x6B => "EXIT_PAIRING_MODE",
		0x6C => "LINK_LAST_DEVICE_DEPRECATED",
		0x6D => "DISCONNECT_ALL_LINK_DEPRECATED",
		0x6E => "OHS_EVENT_1",
		0x6F => "OHS_EVENT_2",
		0x70 => "OHS_EVENT_3",
		0x71 => "OHS_EVENT_4",
		0x72 => "SHS_SEND_USER_DATA_1_FOR_EMBEDDED_APPLICATION_MODE",
		0x73 => "SHS_SEND_USER_DATA_2_FOR_EMBEDDED_APPLICATION_MODE",
		0x74 => "SHS_SEND_USER_DATA_3_FOR_EMBEDDED_APPLICATION_MODE",
		0x75 => "SHS_SEND_USER_DATA_4_FOR_EMBEDDED_APPLICATION_MODE",
		0x76 => "SHS_SEND_USER_DATA_5_FOR_EMBEDDED_APPLICATION_MODE",
		0x77 => "REPORT_CURRENT_RX_NR_STATUS",
		0x78 => "REPORT_CURRENT_TX_NR_STATUS",
		0x79 => "FORCE_BUZZER_ALARM",
		0x7A => "CANCEL_ALL_BT_PAGING",
		0x7B => "OHS_EVENT_5",
		0x7C => "OHS_EVENT_6",
		0x7D => "DISCONNECT_SPP_LINK",
		0x80 => "ENABLE_A2DP_MIX_LINEIN",
		0x81 => "DISABLE_A2DP_MIX_LINEIN",
		0x82 => "INCREASE_LINEIN_INPUT_GAIN",
		0x83 => "DECREASE_LINEIN_INPUT_GAIN",
	);

	return "action=".lookup_value($action,\%actions).", db=$db";
};

sub bm83_data_parsers_paired_device_info {
	my ($src, $dst, $string, $data) = @_;

	my $db = $data->[0];
	my $prio = $data->[1];

	my $bt_mac = print_mac($data, 2, 1);

	return "dev=$db, prio=$prio, mac=$bt_mac";
};

sub bm83_data_parsers_device_info {
	my ($src, $dst, $string, $data) = @_;

	my $db = $data->[0];
	my $type = $data->[1];

	my %types = (
		0x00 => 'DEVICE_NAME',
		0x01 => 'IN-BAND-RINGTONE',
		0x02 => 'DEVICE_TYPE',
		0x03 => 'AVRCP_SUPPORTED',
		0x04 => 'HF_A2DP_GAIN',
		0x05 => 'LINE_IN_GAIN',
		0x06 => 'A2DP_CODEC'
	);

	my $property = lookup_value($type,\%types);

	if ($type == 0x00) {
		return "property=$property, value=\"".cleanup_string(array_to_string($data, 2))."\"";
	}

	if ($type == 0x03) {
		my $player_notification = $data->[2] & 0b0000_0001;
		my $abs_volume_control = ($data->[2] & 0b0000_0010) >> 1;

		return "property=$property, player_notification=$player_notification, abs_volume_control=$abs_volume_control";
	}

	if ($type == 0x04) {
		my $a2dp_gain = $data->[2] & 0b0000_1111;
		my $hf_gain = ($data->[2] & 0b1111_0000) >> 4;

		return "property=$property, a2dp_gain=$a2dp_gain, hf_gain=$hf_gain";
	}

	if ($type == 0x05) {
		my $gain = $data->[2];
		return "property=$property, linein_gain=$gain";
	}

	if ($type == 0x06) {
		return "property=$property, value=".($data->[2]==0x00?'SBC':$data->[2]==0x02?'AAC':print_hex($data->[2],2,'0x'));
	};


	return "property=$property, value=".print_hex($data->[2],2,'0x');
};

sub bm83_data_parsers_caller_id {
	my ($src, $dst, $string, $data) = @_;

	my $db = $data->[0];
	my $caller_id = cleanup_string(array_to_string($data, 1));

	return "caller_id=\"$caller_id\", db=$db";
}

sub bm83_data_parsers_call_status {
	my ($src, $dst, $string, $data) = @_;

	my $db = $data->[0];
	my $status = $data->[1];

	my %states = (
		0x00 => "IDLE",
		0x01 => "VR",
		0x02 => "INCOMING",
		0x03 => "OUTGOING",
		0x04 => "ACTIVE",
		0x05 => "ACTIVE_CALL_WAITING",
		0x06 => "ACTIVE_CALL_HOLD",
	);

	return "status=".lookup_value($status,\%states).", db=$db";
};

sub bm83_data_parsers_bmt_status {
	my ($src, $dst, $string, $data) = @_;

	my $state = $data->[0];

	my %states = (
		0x00 => "POWER_OFF",
		0x01 => "PAIRING_ON",
		0x02 => "POWER_ON",
		0x03 => "PAIRING_OK",
		0x04 => "PAIRING_NOK",
		0x05 => "HFP_CONN",
		0x06 => "A2DP_CONN",
		0x07 => "HFP_DISCO",
		0x08 => "A2DP_DISCO",
		0x09 => "SCO_CONN",
		0x0A => "SCO_DISCO",
		0x0B => "AVRCP_CONN",
		0x0C => "AVRCP_DISCO",
		0x0D => "STANDARD_SPP_CONN",
		0x0E => "STANDARD_SPP_IAP_DISCO",
		0x0F => "STANDBY_ON",
		0x10 => "IAP_CONN",
		0x11 => "ACL_DISCO",
		0x12 => "MAP_CONN",
		0x13 => "MAP_OPERATION_FORBIDDEN",
		0x14 => "MAP_DISCONN",
		0x15 => "ACL_CONN",
		0x16 => "SPP_IAP_DISCONN_NO_OTHER_PROFILE",
		0x17 => "LINK_BACK_ACL",
		0x18 => "INQUIRY_ON",
		0x80 => "AUDIO_SRC_NOT_AUX_NOT_A2DP",
		0x81 => "AUDIO_SRC_AUX_IN",
		0x82 => "AUDIO_SRC_A2DP",
	);

	my $state_readable = lookup_value($state,\%states);

	if ( $state == 0x02 ) {
		return "state=$state_readable, ".(($data->[0]==1)?'ALREADY_':''."POWER_ON");
	}
	if ( $state == 0x03 || $state == 0x09 || $state == 0x0A ) {
		return "state=$state_readable, link=".$data->[1];
	}
	if ( $state == 0x04 ) {
		return "state=$state_readable, ".(($data->[1]==0x01)?'TIMEOUT':($data->[1]==0x02)?'FAIL':($data->[1]==0x03)?'EXIT_PAIRING':print_hex($data->[1],2,'0x'));
	}
	if ( $state == 0x05 ) {
		my $link = ($data->[1] & 0b1111_0000) >> 4;
		my $db = $data->[1] & 0b0000_1111;

		return "state=$state_readable, ".((($data->[2]==0x00)?'HSP':($data->[2]==0x01)?'HFP':print_hex($data->[2],2,'0x')).", link=$link, db=$db");
	}
	if ( $state == 0x06 || $state == 0x0b ) {
		my $link = ($data->[1] & 0b1111_0000) >> 4;
		my $db = $data->[1] & 0b0000_1111;

		return "state=$state_readable, link=$link, db=$db";
	}

	if ( $state == 0x07 || $state == 0x08 || $state == 0x0C || $state == 0x15 || $state == 0x17) {
		return "state=$state_readable, db=".$data->[1];
	}

	if ( $state == 0x11 ) {
		return "state=$state_readable, ".(($data->[1]==0x00)?'DISCONNNECT':($data->[1]==0x01)?'LINK_LOSS':print_hex($data->[1],2,'0x'));
	}

	if ( $state == 0x16 ) {
		my $bt_mac = print_mac($data, 1, 1);
		return "state=$state_readable, mac=$bt_mac";
	}
	return "state=$state_readable, link_info=($string)";
}

sub bm83_data_parsers_event_ack {
	my ($src, $dst, $string, $data) = @_;

	my $hex_event =  print_hex($data->[0],2,'');
	return "event=$hex_event, name=".$cmd_bm83{"BM83_EVT_${hex_event}"};
}

sub bm83_data_parsers_command_ack {
	my ($src, $dst, $string, $data) = @_;

	my $hex_cmd =  print_hex($data->[0]);
	my $status = $data->[1];

	my %status = (
		0x00 => "COMPLETE",
		0x01 => "NOT_ALLOWED",
		0x02 => "UNKNOWN",
		0x03 => "PARAMETER_ERROR",
		0x04 => "BTM_BUSY",
		0x05 => "BTM_MEMORY_FULL",
	);

	$status = lookup_value($status,\%status);

	return "command=$hex_cmd, name=".$cmd_bm83{"BM83_CMD_${hex_cmd}"}.", status=$status";
}

sub print_hex {
	my ($value, $length, $prefix ) = @_;

	$length = 2 if (!$length);
	$prefix = '' if (!$prefix);

	return sprintf("$prefix\%0${length}X",$value);
};


sub ibus_data_parsers_module_status {
	my ($src, $dst, $string, $data) = @_;
	my $announce = $data->[0] & 0b00000001;
	my $variant = ($data->[0] & 0b11111000) >> 3;

	my %variants = (
		"BMBT" => {
			0b00000 => "BMBT_4_3",
			0b00110 => "BMBT_16_9",
			0b01110 => "BMBT_16_9"
			},
		"TEL" => {
			0b00111 => "EVEREST",
			0b00110 => "MOTOROLA",
			0b00000 => "CMT3000"
			},
		"NAVE" => {
			0b01000 => "NAV_MK4",
			0b11000 => "NAV_MK4_ASSIST"
			},
		"GT" => {
			0b00010 => "GT_VM",
			0b01000 => "GT_NAV"
			}
	);

	$variant = lookup_value($variant,$variants{$src});
	return "announce=$announce, variant=$variant";
};

sub ibus_data_parsers_mfl_buttons {
	my ($src, $dst, $string, $data) = @_;
	my $button = $data->[0] & 0b1100_1001;
	my $state = $data->[0] & 0b0011_0000;

	my %states = (
		0b0000_0000 => "PRESS",
		0b0001_0000 => "HOLD",
		0b0010_0000 => "RELEASE",
	);

	my %buttons = (
		0b0000_0001 => "FORWARD",
		0b0000_1000 => "BACK",
		0b0100_0000 => "RT",
		0b1000_0000 => "TEL"
	);

	$button = lookup_value($button,\%buttons);
	$state = lookup_value($state,\%states);

	return "button=$button, state=$state";
}

sub ibus_data_parsers_bmbt_buttons {
	my ($src, $dst, $string, $data) = @_;
	my $button = $data->[0] & 0b0011_1111;
	my $state = $data->[0] & 0b1100_0000;

	my %states = (
		0b0000_0000 => "PRESS",
		0b0100_0000 => "HOLD",
		0b1000_0000 => "RELEASE",
	);

	my %buttons = (
		0b00_0100 => "TONE",
		0b10_0000 => "SEL",
		0b01_0000 => "PREV",
		0b00_0000 => "NEXT",
		0b01_0001 => "PRESET_1",
		0b00_0001 => "PRESET_2",
		0b01_0010 => "PRESET_3",
		0b00_0010 => "PRESET_4",
		0b01_0011 => "PRESET_5",
		0b00_0011 => "PRESET_6",
		0b10_0001 => "AM",
		0b11_0001 => "FM",
		0b10_0011 => "MODE_PREV",
		0b11_0011 => "MODE_NEXT",
		0b11_0000 => "OVERLAY",
		0b00_0110 => "POWER",
		0b10_0100 => "EJECT",
		0b01_0100 => "SWITCH_SIDE",
		0b11_0010 => "TP",
		0b10_0010 => "RDS",

		0b00_1000 => "TELEPHONE",
		0b00_0111 => "AUX_HEAT",

		0b11_0100 => "MENU",
		0b00_0101 => "CONFIRM",

	);

	$button = lookup_value($button,\%buttons);
	$state = lookup_value($state,\%states);

	return "button=$button, state=$state";
}

sub ibus_data_parsers_bmbt_soft_buttons {
	my ($src, $dst, $string, $data) = @_;
	my $button = $data->[1] & 0b0011_1111;
	my $state = $data->[1] & 0b1100_0000;

	my %states = (
		0b0000_0000 => "PRESS",
		0b0100_0000 => "HOLD",
		0b1000_0000 => "RELEASE",
	);

	my %buttons = (
		0b00_1111 => "SELECT",
		0b11_1000 => "INFO",
	);
	$button = lookup_value($button,\%buttons);
	$state = lookup_value($state,\%states);

	return "button=$button, state=$state, extra=$data->[0]";
}

sub ibus_data_parsers_volume {
	my ($src, $dst, $string, $data) = @_;
	my $direction = $data->[0] & 0b0000_0001;
	my $steps = ($data->[0] & 0b1111_0000) >> 4;

	return "volume_change=".(($direction==0)?'-':'+').$steps;
}

sub ibus_data_parsers_navi_knob {
	my ($src, $dst, $string, $data) = @_;
	my $direction = $data->[0] & 0b1000_0000;
	my $steps = $data->[0] & 0b0000_1111;

	return "turn=".(($direction==0)?'-':'+').$steps;
}

sub ibus_data_parsers_monitor_control {
	my ($src, $dst, $string, $data) = @_;

	my $source = $data->[0] & 0b0000_0011;
	my $power = ($data->[0] & 0b0001_0000) >> 4;
	my $encoding = $data->[1] & 0b0000_0011;
	my $aspect = ($data->[1] & 0b0011_0000) >> 4;

	my %sources = (
		0b0000_0000 => "NAV_GT",
		0b0000_0001 => "TV",
		0b0000_0010 => "VID_GT",
	);

	my %encodings = (
		0b0000_0010 => "PAL",
		0b0000_0001 => "NTSC",
	);

	my %aspects = (
		0b0000_0000 => "4:3",
		0b0000_0001 => "16:9",
		0b0000_0011 => "ZOOM",
	);

	$source = lookup_value($source,\%sources);
	$encoding = lookup_value($encoding,\%encodings);
	$aspect = lookup_value($aspect,\%aspects);

	return "power=$power, source=$source, aspect=$aspect, enc=$encoding";
}

sub ibus_data_parsers_request_screen {
	my ($src, $dst, $string, $data) = @_;

	my $priority = $data->[0] & 0b0000_0001;
	my $hide_header = ($data->[0] & 0b0000_0010) >> 1;
	my $hide_body = $data->[0] & 0b0000_1100;

	my %bodies = (
		0b0000_0100 => "HIDE_BODY_SEL",
		0b0000_1000 => "HIDE_BODY_TONE",
		0b0000_1100 => "HIDE_BODY_MENU",
	);

	$hide_body = lookup_value($hide_body,\%bodies);

	return "priority=".(($priority==0)?"RAD":"GT").", hide_header=".(($hide_header==1)?"HIDE":"SHOW").", hide=$hide_body";
}

sub ibus_data_parsers_set_radio_ui {
	my ($src, $dst, $string, $data) = @_;

	my $priority = $data->[0] & 0b0000_0001;
	my $audio_obc = ($data->[0] & 0b0000_0010) >> 1;
	my $new_ui = ($data->[0] & 0b0001_0000) >> 4;
	my $new_ui_hide = ($data->[0] & 0b1000_0000) >> 7;

	return "priority=".(($priority==0)?"RAD":"GT").", audio+obc=$audio_obc, new_ui=$new_ui, new_ui_hide=$new_ui_hide";
}

sub ibus_data_parsers_gt_dynamic_stations {
	my ($src, $dst, $string, $data) = @_;

	my $data1 = $data->[0];
	my $count = $data->[1];
	my $page  = $data->[2];

	my $text1 = cleanup_string(array_to_string($data,3,8));
	my $text2 = cleanup_string(array_to_string($data,12,8));
	my $text3 = cleanup_string(array_to_string($data,21,8));

	my $sel1 = ($data->[11]!=0)?' selected':'';
	my $sel2 = ($data->[20]!=0)?' selected':'';
	my $sel3 = ($data->[29]!=0)?' selected':'';

	return "val1=$data1, count=$count, page=$page, text1=\"$text1\"$sel1, text2=\"$text2\"$sel2, text3=\"$text3\"$sel3";
}

sub ibus_data_parsers_gt_write {
	my ($src, $dst, $string, $data) = @_;

	my $layout = $data->[0];
	my $function = $data->[1];
	my $index = $data->[2] & 0b0001_1111;
	my $clear = ($data->[2] & 0b0010_0000 ) >> 5;
	my $buffer = ($data->[2] & 0b0100_0000 ) >> 6;
	my $highlight = ($data->[2] & 0b1000_0000 ) >> 7;

	my $text = cleanup_string(array_to_string($data,3));

	my %layouts = (
		0x42 => "DIAL",
		0x43 => "DIRECTORY",
		0x60 => "WRITE_INDEX",
		0x61 => "WRITE_INDEX_TMC",
		0x62 => "WRITE_ZONE",
		0x63 => "WRITE_STATIC",
		0x80 => "TOP-8",
		0xf0 => "LIST",
		0xf1 => "DETAIL"
	);

	my %functions = (
		0x00 => "NULL",
		0x01 => "CONTACT",
		0x05 => "SOS",
		0x07 => "NAVIGATION",
		0x08 => "INFO"
	);

	$layout = lookup_value($layout,\%layouts);
#	$function = lookup_value($function,\%functions);

	return "layout=$layout, func/cursor=$function, index=$index, clear=$clear, buffer=$buffer, highlight=$highlight, text=\"$text\"";

}

sub ibus_data_parsers_write_numeric {
	my ($src, $dst, $string, $data) = @_;

	my $cmd = $data->[0];
	my $value = unpack_8bcd($data->[1]);

	return "command=".($cmd==0x20?'clear':($cmd==0x2B?'extended, display='.($value*10):'normal, display='.$value));
}

sub ibus_data_parsers_gt_write_menu {
	my ($src, $dst, $string, $data) = @_;

	my $source = ($data->[0] & 0b1110_0000) >> 5;
	my $config = ($data->[0] & 0b1111_1111);
	my $option = ($data->[1] & 0b0011_1111);

	my $text = cleanup_string(array_to_string($data, 2));

	my %sources = (
		0b000 => "SERVICE",
		0b001 => "WEATHER",
		0b010 => "ANALOGUE",
		0b011 => "DIGITAL",
		0b100 => "TAPE",
		0b101 => "TRAFFIC",
		0b110 => "CDC"
	);

	my %configs = (
		# Source: Service Mode (0b000 << 5)
		0b0000_0010 => "SERVICE_SERIAL_NUMBER", 
		0b0000_0011 => "SERVICE_SOFTWARE_VERSION", 
		0b0000_0100 => "SERVICE_GAL", 
		0b0000_0101 => "SERVICE_FQ", 
		0b0000_0110 => "SERVICE_DSP", 
		0b0000_0111 => "SERVICE_SEEK_LEVEL", 
		0b0000_1000 => "SERVICE_TP_VOLUME", 
		0b0000_1001 => "SERVICE_AF", 
		0b0000_1010 => "SERVICE_REGION", 

		# Source: Weather Band (0b001 << 5)
		0b0010_0000 => "WEATHER_NONE", 
		0b0010_0001 => "WEATHER_CH_1", 
		0b0010_0010 => "WEATHER_CH_2", 
		0b0010_0011 => "WEATHER_CH_3", 
		0b0010_0100 => "WEATHER_CH_4", 
		0b0010_0101 => "WEATHER_CH_5", 
		0b0010_0110 => "WEATHER_CH_6", 
		0b0010_0111 => "WEATHER_CH_7", 

		# Source: Analogue (i.e. FMA) (0b010 << 5)    
		0b0100_0000 => "ANALOGUE_UPDATE", 
		0b0100_0001 => "ANALOGUE_MODE_MANUAL",   # [m]
		0b0100_0010 => "ANALOGUE_MODE_SCAN",   # [SCA]
		0b0100_0011 => "ANALOGUE_MODE_SENSITIVE",   # [II]
		0b0100_0100 => "ANALOGUE_MODE_NON_SENSITIVE",   # [I]
		0b0100_0110 => "ANALOGUE_TRAFFIC",   # < 3/1-30
		0b0100_0111 => "ANALOGUE_TRAFFIC_PROGRAM",   # < 3/1-30

		0b0100_0000 => "ANALOGUE_TRAFFIC_OFF", 
		0b0100_1000 => "ANALOGUE_TRAFFIC_ON",   # [T]

		0b0101_0000 => "ANALOGUE_ST",   # < 3/1-30

		# Source: Digital (i.e. FMD) (0b011 << 5)
		0b0110_0000 => "DIGITAL_MENU",   # Stations, Info.
		0b0110_0001 => "DIGITAL_RDS", 
		0b0110_0010 => "DIGITAL_HEADER", 
		0b0110_0011 => "DIGITAL_MP3", 

		# Source: Tape (0b100 << 5)
		0b1000_0000 => "TAPE_ERROR",   # "TAPE ERROR"
		0b1000_0010 => "TAPE_PRES", 
		0b1000_0011 => "TAPE_FFW", 
		0b1000_0110 => "TAPE_FW", 
		0b1000_0100 => "TAPE_RRW", 
		0b1000_0111 => "TAPE_RW", 
		0b1000_1000 => "TAPE_CLEAN", 
		0b1000_1001 => "TAPE_INVERSE", 

		0b1000_0000 => "TAPE_SIDE_A", 
		0b1001_0000 => "TAPE_SIDE_B", 

		# Source: Traffic (0b101 << 5)
		0b1010_0000 => "TRAFFIC_NO_STATION", 
		0b1010_0001 => "TRAFFIC_1", 
		0b1010_0010 => "TRAFFIC_2", 

		# Source: CDC (0b110 << 5)
		0b1100_0000 => "CDC_ERROR",   # "CD ERROR"
		0b1100_0001 => "CDC_NO_MAG",   # "NO MAGAZINE"
		0b1100_0010 => "CDC_NO_DISC",   # "NO DISC"
		0b1100_0011 => "CDC_CHECK",   # "CD CHECK"

		0b1100_0101 => "CDC_FF",   # [>>]
		0b1100_0110 => "CDC_RW",   # [<<R]

		0b1100_0100 => "CDC_SEARCH",   # [< >] Default
		0b1100_0111 => "CDC_SCAN",   # [SCAN]
		0b1100_1000 => "CDC_RAND",   # [RND]
		0b1100_1010 => "CDC_FF_RW",   # [<< >>]

		0b1100_1011 => "CDC_LOADING",   # I think...?
	);

	my %options = (
		0x10	=> "SET_0x10",
		0x20	=> "UPDATE",
		0x30	=> "SET_0x30",
	);

	$source = lookup_value($source,\%sources);
	$config = lookup_value($config,\%configs);
	$option = lookup_value($option,\%options);

	return "source=$source, config=$config, options=$option, text=\"$text\"";
}

sub ibus_data_parsers_obc_text {
	my ($src, $dst, $string, $data, $time) = @_;

	my $property = $data->[0];

	my %properties = (
		0x01 => "TIME",
		0x02 => "DATE",
		0x03 => "TEMP",
		0x04 => "CONSUMPTION_1",
		0x05 => "CONSUMPTION_2",
		0x06 => "RANGE",
		0x07 => "DISTANCE",
		0x08 => "ARRIVAL",
		0x09 => "LIMIT",
		0x0a => "AVG_SPEED",
		0x0e => "TIMER",
		0x0f => "AUX_TIMER_1",
		0x10 => "AUX_TIMER_2",
		0x16 => "CODE_EMERGENCY_DEACIVATION",
		0x1a => "TIMER_LAP",
	);

	my $text = cleanup_string(array_to_string($data,2));
	$property = lookup_value($property,\%properties);


	if ($property eq 'TIME') {
		if ($text=~/(\d+):(\d+)(.)/o) {
			my $time_local = ( $1 * 60 + $2 ) * 60;
			if (($3 eq 'P' || $3 eq 'p') && ($1 < 12)) {
				$time_local += 12 * 60 * 60;
			};
			if (($3 eq 'A' || $3 eq 'a') && ($1 == 12)) {
				$time_local -= 12 * 60 * 60;
			}
			$time_offset = $time_local*1000 - $time;
		}
	}


	return "property=$property, text=\"$text\"";
};

sub ibus_data_parsers_ike_obc_input {
	my ($src, $dst, $string, $data) = @_;

	my $property = $data->[0];

	my %properties = (
		0x01 => "TIME",
		0x02 => "DATE",
		0x03 => "TEMP",
		0x04 => "CONSUMPTION_1",
		0x05 => "CONSUMPTION_2",
		0x06 => "RANGE",
		0x07 => "DISTANCE",
		0x08 => "ARRIVAL",
		0x09 => "LIMIT",
		0x0a => "AVG_SPEED",
		0x0e => "TIMER",
		0x0f => "AUX_TIMER_1",
		0x10 => "AUX_TIMER_2",
		0x16 => "CODE_EMERGENCY_DEACIVATION",
		0x1a => "TIMER_LAP",
	);

	$property = lookup_value($property,\%properties);

	my $value = '';

	if (($property eq 'TIME')||($property eq 'AUX_TIMER_1')||($property eq 'AUX_TIMER_2')) {
		my $pm = '';
		$value = $data->[1];
		if ($value >= 0x80) {
			$value -= 0x80;
			$pm = 'PM';
		};
		$value = sprintf("%02d:%02d$pm", $value, $data->[2]);
	} elsif ($property eq 'DATE') {
		$value = sprintf("%02d.%02d.%04d", $data->[1], $data->[2], $data->[3]+2000);
	} elsif (($property eq 'DISTANCE')||($property eq 'LIMIT')) {
		$value = $data->[1] << 8 +  $data->[2];
	} elsif ($property eq 'CODE_EMERGENCY_DEACIVATION') {
		$value = sprintf("%04d", $data->[1] << 8 +  $data->[2]);
	} else {
		$string =~ s/^...//o;
		$value = sprintf("($string)");
	}

	return "property=$property, value=$value";
}

sub ibus_data_parsers_ike_odo_response {
	my ($src, $dst, $string, $data) = @_;

	my $km = ($data->[2] << 16) + ($data->[1] << 8) + $data->[0];

	return "odo=${km} km";
}

sub ibus_data_parsers_ike_gps_time {
	my ($src, $dst, $string, $data) = @_;

	my $hour = unpack_8bcd($data->[1]);
	my $min  = unpack_8bcd($data->[2]);
	my $day  = unpack_8bcd($data->[3]);
	my $mon  = unpack_8bcd($data->[5]);
	my $year = unpack_8bcd($data->[6])*100 + unpack_8bcd($data->[7]);

# fix epoch problem
	my $dt = DateTime->new(
	    year       => $year,
	    month      => $mon,
	    day        => $day,
	    hour       => $hour,
	    minute     => $min,
	    second     => 0,
	    time_zone  => 'UTC'
    );
    $dt->add( weeks => 1024 );

	return sprintf("UTC,24,dd.mm.yyyy time=%02d:%02d, date=%02d.%02d.%04d, gps_date=%02d.%02d.%04d",$hour,$min,$dt->day(),$dt->month(),$dt->year(), $day, $mon, $year);
}


sub ibus_data_parsers_cdc_request {
	my ($src, $dst, $string, $data) = @_;

	my $command = $data->[0];

	my %commands = (
		0x00 => "GET_STATUS",
		0x01 => "STOP_PLAYING",
		0x02 => "PAUSE_PLAYING",
		0x03 => "START_PLAYING",
		0x0A => "CHANGE_TRACK",
		0x04 => "SEEK",
		0x05 => "CHANGE_TRACK_BLAUPUNKT",
		0x06 => "CD_CHANGE",
		0x07 => "SCAN",
		0x08 => "RANDOM_MODE",
	);

	$command = lookup_value($command,\%commands);
	return "command=$command";
};

sub ibus_data_parsers_cdc_response {
	my ($src, $dst, $string, $data) = @_;

	my $status = $data->[0];
	my $audio = $data->[1];
	my $error = $data->[2];
	my $magazine = $data->[3];
	my $disk = $data->[5];
	my $track = $data->[6];

	my %statuses = (
		0X00 => "STOPPED",
		0X01 => "PAUSED",
		0X02 => "PLAYING",
		0X03 => "FFW",
		0X04 => "RWD",
		0X05 => "NEXT_TRACK",
		0X06 => "PREVIOUS_TRACK",
		0X07 => "PENDING/ACKNOWLEDGE",
		0X08 => "MAGAZINE_READY",
		0X09 => "MAGAZINE_CHECKING",
		0X0A => "MAGAZINE_EJECTED",
	);

	my %audios = (
		0X02 => "STOPPED",
		0X09 => "PLAYING",
		0X0C => "READY?",

		0X82 => "NEWCDC_STOPPED",
		0X89 => "NEWCDC_PLAYING",
		0X8C => "NEWCDC_READY?",
	);

	my %errors = (
		0X00 => "NO_ERROR",
		0X02 => "HIGH_TEMP",
		0X08 => "NO_DISC",
		0X10 => "NO_MAGAZINE",
	);

	$status = lookup_value($status,\%statuses);
	$audio = lookup_value($audio,\%audios);
	$error = lookup_value($error,\%errors);

	return sprintf("status=%s, audio=%s, error=%s, magazines=0b%06b, disk=%x, track=%x", $status, $audio, $error, $magazine, $disk, $track);
};

sub ibus_data_parsers_speed_rpm {
	my ($src, $dst, $string, $data) = @_;

	my $speed = $data->[0] * 2;
	my $rpm = $data->[1] * 100;

	return "speed=$speed km/h, rpm=$rpm";
};

sub ibus_data_parsers_ike_temp {
	my ($src, $dst, $string, $data) = @_;

	my $coolant = $data->[1];
	my $amb = $data->[0];
	if ($amb>127) {
		$amb = $amb-256;
	}
	return "coolant=$coolant, amb=$amb C";
};

sub ibus_data_parsers_ike_sensor {
	my ($src, $dst, $string, $data) = @_;

	my $handbrake 		= ( $data->[0] & 0b0000_0001 );
	my $oil_pressure 	= ( $data->[0] & 0b0000_0010 ) >> 1;
	my $brake_pads 		= ( $data->[0] & 0b0000_0100 ) >> 2;
	my $transmission 	= ( $data->[0] & 0b0001_0000 ) >> 4;

	my $ignition		= ( $data->[1] & 0b0000_0001 ) ;
	my $door 		= ( $data->[1] & 0b0000_0010 ) >> 1;
	my $gear		= ( $data->[1] & 0b1111_0000 ) >> 4;

	my $aux_vent		= ( $data->[2] & 0b0000_1000 ) >> 3;

	my %gears = (
		0b0000 => "GEAR_NONE",
		0b1011 => "GEAR_PARK",
		0b0001 => "GEAR_REVERSE",
		0b0111 => "GEAR_NEUTRAL",
		0b1000 => "GEAR_DRIVE",
		0b0010 => "GEAR_FIRST",
		0b0110 => "GEAR_SECOND",
		0b1101 => "GEAR_THIRD",
		0b1100 => "GEAR_FOURTH",
		0b1110 => "GEAR_FIFTH",
		0b1111 => "GEAR_SIXTH"
	);

	$gear = lookup_value($gear,\%gears);

	return "handbrake=$handbrake, ignition=$ignition, gear=$gear, ?door=$door, , aux_vent=$aux_vent, warn_oil_pressure=$oil_pressure, warn_brake_pads=$brake_pads, warn_transmission=$transmission";
};

sub ibus_data_parsers_ike_ignition {
	my ($src, $dst, $string, $data) = @_;

	my $ignition = ( $data->[0] & 0b0000_0111 );

	my %ign = (
		0b0000_0000   => "POS_0",
		0b0000_0001   => "POS_1",
		0b0000_0011   => "POS_2",
		0b0000_0111   => "POS_3",
	);

	$ignition = lookup_value($ignition,\%ign);

	return "ignition=$ignition";
};

sub ibus_data_parsers_redundant_data {
	my ($src, $dst, $string, $data) = @_;

	my $odo  = ( ($data->[0] << 8) + $data->[1] ) * 100;
	my $fuel = $data->[3]*10;
	my $oil  = ( ($data->[4] << 8) + $data->[5] );
	my $time = ( ($data->[6] << 8) + $data->[7] );

	return "odo=$odo km, fuel=$fuel l, ?oil=$oil, time=$time days";
};

sub ibus_data_parsers_lcm_redundant_data {
	my ($src, $dst, $string, $data) = @_;

	my $odo  = ( ($data->[5] << 8) + $data->[6] ) * 100;
	my $fuel = $data->[8]*10;
	my $oil  = ( ($data->[9] << 8) + $data->[10] );
	my $time = ( ($data->[11] << 8) + $data->[12] );
	my $vin  = sprintf("%c%c%02X%02X%1X", $data->[0], $data->[1], $data->[2],  $data->[3], $data->[4]>>4);

	return "vin=$vin, odo=$odo km, fuel=$fuel l, ?oil=$oil, time=$time days";
};

sub ibus_data_parsers_lcm_indicator_resp {
	my ($src, $dst, $string, $data) = @_;

	my $turn_rapid = 	$data->[0] & 0b1000_0000;
	my $turn_right = 	$data->[0] & 0b0100_0000;
	my $turn_left =  	$data->[0] & 0b0010_0000;
	my $fog_rear =   	$data->[0] & 0b0001_0000;
	my $fog_front =  	$data->[0] & 0b0000_1000;
	my $beam_high =  	$data->[0] & 0b0000_0100;
	my $beam_low  =  	$data->[0] & 0b0000_0010;
	my $parking =    	$data->[0] & 0b0000_0001;

	my $ccm_lic_plate =  	$data->[1] & 0b1000_0000;
	my $ccm_turn_right = 	$data->[1] & 0b0100_0000;
	my $ccm_turn_left =  	$data->[1] & 0b0010_0000;
	my $ccm_fog_rear =   	$data->[1] & 0b0001_0000;
	my $ccm_fog_front =  	$data->[1] & 0b0000_1000;
	my $ccm_beam_high =  	$data->[1] & 0b0000_0100;
	my $ccm_beam_low  =  	$data->[1] & 0b0000_0010;
	my $ccm_parking =    	$data->[1] & 0b0000_0001;

	my $ccm_reverse = 	$data->[2] & 0b0010_0000;
	my $indicators = 	$data->[2] & 0b0000_0100;
	my $ccm_brake = 	$data->[2] & 0b0000_0010;

	my $fog_rear_switch = 	$data->[3] & 0b0100_0000;
	my $kombi_low_left = 	$data->[3] & 0b0010_0000;
	my $kombi_low_right = 	$data->[3] & 0b0001_0000;
	my $kombi_brake_left = 	$data->[3] & 0b0000_0010;
	my $kombi_brake_right =	$data->[3] & 0b0000_0001;

	my $resp = "turn=".($turn_left?'LEFT,':'').($turn_right?'RIGHT,':'').($turn_rapid?'RAPID,':'').($indicators?'INDICATORS,':'').
			" fog=".($fog_front?'FRONT,':'').($fog_rear?'REAR,':'').
			" fog_switch=".($fog_rear_switch?'REAR,':'').
			" kombi_low=".($kombi_low_left?'LEFT,':'').($kombi_low_right?'RIGHT,':'').
			" kombi_brake=".($kombi_brake_left?'LEFT,':'').($kombi_brake_right?'RIGHT,':'').
			" other=".($parking?'PARKING,':'').($beam_low?'BEAM_LOW,':'').($beam_high?'BEAM_HIGH,':'').
			" ccm=".($ccm_turn_left?'TURN_LEFT,':'').($ccm_turn_right?'TURN_RIGHT,':'').
				($ccm_fog_front?'FOG_FRONT,':'').($ccm_fog_rear?'FOG_REAR,':'').
				($ccm_beam_low?'BEAM_LOW,':'').($ccm_beam_high?'BEAM_HIGH,':'').
				($ccm_lic_plate?'LICENCE_PLATE,':'').
				($ccm_parking?'PARKING,':'').
				($ccm_reverse?'REVERSE,':'').
				($ccm_brake?'BRAKE,':'').
			" raw=$string";
	$resp =~ s/[a-z_]+=\s//go;

	return $resp;
}


sub ibus_data_parsers_doors_status {
	my ($src, $dst, $string, $data) = @_;

	my $doors 		= ( $data->[0] & 0b0000_1111 );
	my $central_lock 	= ( $data->[0] & 0b0011_0000 ) >> 4;
	my $lamp_interiour	= ( $data->[0] & 0b0100_0000 ) >> 6;

	my $windows		= ( $data->[1] & 0b0000_1111 ) ;
	my $sunroof		= ( $data->[1] & 0b0001_0000 ) >> 4;
	my $rear_trunk	 	= ( $data->[1] & 0b0010_0000 ) >> 5;
	my $front_boot	 	= ( $data->[1] & 0b0100_0000 ) >> 6;

	my %locks = (
		0b0001 => "CENTRAL_LOCKING_UNLOCKED",
		0b0010 => "CENTRAL_LOCKING_LOCKED",
		0b0011 => "CENTRAL_LOCKING_ARRESTED",
	);

	$central_lock = lookup_value($central_lock,\%locks);

	return sprintf("doors=0b%04b, windows=0b%04b, locks=%s, lamp=$lamp_interiour, sunroof=$sunroof, front_lid=$front_boot, rear_lid=$rear_trunk", $doors, $windows, $central_lock);
};

sub print_time {
	my ($time, $ms, $print_ms) = @_;

	my $sec;
	my $min;
	my $hour;

	if ($ms) {
		$sec = ($time % 60000)/1000;
		$min = int($time/60000) % 60;
		$hour = int($time/(60*60*1000));
	} else {
		$sec = int($time) % 60;
		$min = int($time/60) % 60;
		$hour = int($time/(60*60));
	}

	if (!$print_ms) {
		return sprintf("%2d:%02d:%02d", $hour, $min, $sec);
	} else {
		return sprintf("%2d:%02d:%06.3f", $hour, $min, $sec)
	}

}

sub print_mac {
	my ($data, $index, $reverse) = @_;

	if ($reverse) {
		return sprintf("%02X%02X%02X%02X%02X%02X", $data->[$index+5], $data->[$index+4], $data->[$index+3], $data->[$index+2], $data->[$index+1], $data->[$index]);
	} else {
		return sprintf("%02X%02X%02X%02X%02X%02X", $data->[$index], $data->[$index+1], $data->[$index+2], $data->[$index+3], $data->[$index+4], $data->[$index+5]);
	}
}

sub lookup_value {
	my ($key, $hash) = @_;

	return %$hash{$key} || print_hex($key);
}

sub array_to_string {
	my ($data, $start, $len) = @_;

	my $text = "";
	my $done = 0;
	for (my $i = $start; $i<scalar(\$data); $i++) {
		if ($data->[$i] == 0) {
			last;
		}
		$text .= chr($data->[$i]);
		$done++;
		last if ($len && ($done>=$len));
	}
	return $text;
}

sub hex_string_to_array {
	my ($string, $len) = @_;
	my @data;
	my $cnt = 0;

	foreach(split(/\s+/,$string)) {
		if ($cnt<$len) {
			push(@data, hex($_));
			$cnt++;
		} else {
			last;
		}
	}

	return @data;
}

sub cleanup_string {
	my ($text) = @_;

	$text =~ s/\x06/<nl>/go;
	$text =~ s/\r/<cr>/go;
	$text =~ s/\n/<nl>/go;
	$text =~ s/[^[:ascii:]]/~/go;
	return $text;	
}

sub unpack_8bcd {
	my ($data) = @_;
	return ($data >> 4)*10 + ($data & 0x0F);
}


sub local_time {
	my ($time) = @_;

	if ($config_local_time) {
		my $time_local;
		if ($time_offset != 0) {
			$time_local = $time + $time_offset;
			$time_local = print_time($time_local,1,0);
		} else {
			$time_local = ' ' x 8;
		}
		return $time_local;
	} else {
		return;
	}
};

sub in_array {
	my($needle, $array) = @_;

	foreach (@$array) {
		return 1 if ($needle eq $_);
	}
	return 0;
};

sub process_ibus_packet {
	my ($time, $packet, $self) = @_;

	my @packet = split(/\s+/,$packet);

	my $src = $bus{$packet[0]} || "0x".$3;
	my $dst = $bus{$packet[2]} || "0x".$5;
	my $src_orig = hex($packet[0]);
	my $dst_orig = hex($packet[2]);
	my $cmd_raw = $packet[3];
	my $data = join(' ', @packet[4..scalar(@packet)]);

	my $cmd_assumed;
	my $cmd;
	my $broadcast = " ";

	if (in_array($dst, \@broadcast_addresses)) {
		$broadcast = "B";

		$cmd_assumed = $src."_BROADCAST_".$cmd_raw;
		if ($cmd_ibus{$cmd_assumed}) {
			$cmd = $cmd_ibus{$cmd_assumed};
		} elsif ($cmd_ibus{$cmd_raw}) {
			$cmd = $src."_BROADCAST_".$cmd_ibus{$cmd_raw};
		} else { 
			$cmd = $cmd_assumed."_UNK";
		};
	} elsif ($cmd_ibus{$cmd_raw} =~ /RESP/io) {
			$cmd = $src."_".$cmd_ibus{$cmd_raw};
	} else {
		$cmd_assumed = $dst."_".$cmd_raw;
		if ($cmd_ibus{$cmd_assumed}) {
			$cmd = $cmd_ibus{$cmd_assumed};
		} else {
			$cmd_assumed = $src."_".$cmd_raw;
			if ($cmd_ibus{$cmd_assumed}) {
				$cmd = $cmd_ibus{$cmd_assumed};
			} elsif ($cmd_ibus{$cmd_raw}) {
				$cmd = $dst."_".$cmd_ibus{$cmd_raw};
			} else { 
				$cmd = $dst."_".$cmd_raw."_UNK";
			};
		}
	}

	return if (in_array($src, \@ignore_devices) || in_array($dst, \@ignore_devices) || in_array($src_orig, \@ignore_devices) || in_array($dst_orig, \@ignore_devices));
	return if (in_array($cmd_raw, \@ignore_commands) || in_array(hex($cmd_raw), \@ignore_commands));
	return if (in_array($cmd, \@ignore_commands));

	my $self = ($self == 1)?"*":" ";

	$data =~ s/\s*[0-9a-fA-F][0-9a-fA-F]\s*$//;

	my $data_parsed;
	if ($data_parsers{$cmd}) {
		my @data = hex_string_to_array($data, scalar(@packet) - 5);
		if (scalar(@data) > 0) {
			$data_parsed = $data_parsers{$cmd}->($src, $dst, $data, \@data, $time);
			$counters_payload_size{$cmd}+=scalar(@data);
		} else {
			$data_parsed = "";
			$counters_payload_size{$cmd}+=0;
		}
		$counters_commands{$cmd}++;
	} else {
		$data_parsed = $data;
		if ($data eq "") {
			$counters_commands{$cmd}++;
			$counters_payload_size{$cmd}+=0;
		} else {
			$counters_commands{$cmd.' (payload not processed)'}++;
			$counters_payload_size{$cmd.' (payload not processed)'}+=int((length($data)+1)/3);
		}
	}

	$counters_devices{$src}++;

	return sprintf("%1s %1s %4s -> %-4s %2s %s (%s)", $self, $broadcast, $src, $dst, $cmd_raw, $cmd, $data_parsed);
};

my $time;

while (<>) {
	chomp;
	s/[\n\s\r]+$//o;

	my $line = $_;

	if (/^[#\s]*\[(\d+)\]\s+DEBUG:\s+IBus:\s+RX\[(\d+)\]:\s+?(..)\s+(..)\s+(..)\s+(..)\s*(.*?)$/osi) {	
# IBUS message
		$time = $1;
		my $len = $2;
		my $packet = "$3 $4 $5 $6 $7";
		my $self;
		my $broadcast;


		my $self = 0;

		if ($packet =~ s#\s+\[SELF\]##o) {
			$self = 1;
		}

		my $resp = process_ibus_packet($time, $packet, $self);
		my $time_local = local_time($time);

		if ($config_original_line) {
			print $line."\n";
		}

		if ($config_local_time) {
			print print_time($time, 1, 1)." ($time_local) ";
		} else {
			print print_time($time, 1, 1)." ";
		};

		printf $resp;

		if ($config_original_data) {
			printf (" [%s]", $packet);
		};
		print "\n";
	} elsif (/^([0-9a-fA-F\s]+)\s*$/o) {
#raw IBUS packet

		my $packet = $1;
		$time += 10;
		my $resp = process_ibus_packet($time, $packet, 0);
		my $time_local = local_time($time);

		if ($config_original_line) {
			print $line."\n";
		}

		if ($config_local_time) {
			print print_time($time, 1, 1)." ($time_local) ";
		} else {
			print print_time($time, 1, 1)." ";
		};

		printf $resp;

		if ($config_original_data) {
			printf (" [%s]", $packet);
		};
		print "\n";


	} elsif (/^\[(\d+)\]\s+DEBUG:\s+BT:\s+([RW]):\s+'(\S+)\s*(.*)\s*'$/os) {
# BlueTooth BC127 Messages
		my $time = $1;
		my $cmd = 'BC127_'.$3;
		my $data = $4;
		my $src;
		my $dst;
		my $self;

		if ($2 eq 'R') {
			$src = "BT";
			$dst = "BBUS";
			$self = " ";
			$cmd .= '_RESPONSE';
		} else {
			$src = "BBUS";
			$dst = "BT";
			$self = "*";
		}

		next if (in_array($src, \@ignore_devices) || in_array($dst, \@ignore_devices));
		next if (in_array($cmd, \@ignore_commands));

		$counters_commands{$cmd}++;
		$counters_devices{$src}++;

		my $time_local = local_time($time);

		if ($config_original_line) {
			print $line."\n";
		};

		if ($config_local_time) {
			print print_time($time, 1, 1)." ($time_local) ";
		} else {
			print print_time($time, 1, 1)." ";
		};

		printf ("%1s   %4s -> %-4s    %s %s\n", $self, $src, $dst, $cmd, $data);
	} elsif (/^\[(\d+)\]\s+DEBUG:\s+BM83:\s+([RT])X:\s+AA\s(..)\s(..)\s(..)\s(.*)\s*$/os) {
# BlueTooth BM83 Messages
		my $packet = "AA $3 $4 $5 $6";
		my $time = $1;
		my $len = (hex($3) << 8) + hex($4) - 1;
		my $cmd;
		my $cmd_raw = $5;
		my $data = $6;
		my $src;
		my $dst;
		my $self;

		if ($2 eq 'R') {
			$src = "BT";
			$dst = "BBUS";
			$self = " ";
			$cmd = 'BM83_EVT_'.$cmd_raw;
			if ($cmd_bm83{$cmd}) {
				$cmd = 'BM83_EVT_'.$cmd_bm83{$cmd};
			} else {
				$cmd.='_UNK';
			}
		} else {
			$src = "BBUS";
			$dst = "BT";
			$self = "*";
			$cmd = 'BM83_CMD_'.$cmd_raw;
			if ($cmd_bm83{$cmd}) {
				$cmd = 'BM83_CMD_'.$cmd_bm83{$cmd};
			} else {
				$cmd.='_UNK';
			}
		}

		next if (in_array($src, \@ignore_devices) || in_array($dst, \@ignore_devices));
		next if (in_array($cmd, \@ignore_commands));

		$counters_devices{$src}++;

		my $time_local = local_time($time);

		$data =~ s/\s*..$//;

		my $data_parsed;
		if ($data_parsers{$cmd}) {
			my @data = hex_string_to_array($data,$len);
			if (scalar(@data) > 0) {
				$data_parsed = $data_parsers{$cmd}->($src,$dst, $data, \@data, $time);
			} else {
				$data_parsed = "";
			}
			$counters_commands{$cmd}++;
			$counters_payload_size{$cmd}+=$len;
		} else {
			$data_parsed = $data;
			if ($data eq "") {
				$counters_commands{$cmd}++;
				$counters_payload_size{$cmd}+=0;
			} else {
				$counters_payload_size{$cmd.' (payload not processed)'}+=$len;
				$counters_commands{$cmd.' (payload not processed)'}++;
			}
		}


		if ($config_original_line) {
			print $line."\n";
		};

		if ($config_local_time) {
			print print_time($time, 1, 1)." ($time_local) ";
		} else {
			print print_time($time, 1, 1)." ";
		};

		printf ("%1s   %4s -> %-4s %2s %s (%s)", $self, $src, $dst, $cmd_raw, $cmd, $data_parsed);

		if ($config_original_data) {
			printf (" [%s]", $packet);
		};
		print "\n";

	} else {
		$counters_commands{"UNPROCESSED_LINE"}++;
		print $line."\n" if ($config_nonparsed_lines);
	}
};

if ($config_stats) {
	print "---------------\nStatistics:\n";

	print "Count,\tAvg sz\tof non-ignored commands:\n";
	foreach (sort { $counters_commands{$b} <=> $counters_commands{$a} } keys(%counters_commands)) {
		print $counters_commands{$_}."\t".(defined($counters_payload_size{$_})?int($counters_payload_size{$_}/$counters_commands{$_}):' ')."\t$_\n";
	}

	print "\nCount\tof non-ignored devices sending packets:\n";
	foreach (sort { $counters_devices{$b} <=> $counters_devices{$a} } keys(%counters_devices)) {
		print $counters_devices{$_}."\t$_\n";
	}
}
