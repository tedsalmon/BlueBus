#!perl
use strict;

# Usage: in your terminal run and treat it as any tool that processes text files in pipes:
# ./bb-log-parse.pl < your_blubus_session.log
#
# page the file and interactivelly search:
# ./bb-log-parse.pl < your_blubus_session.log | more
#
# find all messaget to GT and return few lines around it:
# ./bb-log-parse.pl < your_blubus_session.log | grep -a --context=5 ' -> GT'
#
# parse the file and store it to new file:
# ./bb-log-parse.pl < your_blubus_session.log > parsed_log.txt
#
# translate logs as they appear in live session:
# 1. start your "screen" command with -L 
#    screen -L /dev/tty.usbserial-AB0M7T7E 115200
#
# 2. in new terminal window, setup the file logging to be instant
#    screen -r -x -p0 -X logfile flush 0
#
# 3. watch the live log ( filename may be different - but similar )
#    tail -f screenlog.0 | ./bb-log-parse.pl
#
# configuration options:

# return also real local time of events, when possible
my $config_local_time = 1;

# return also original line, to validate the parsing 
my $config_original_line = 0;

# return also original packet, to validate the payload parsing 
my $config_original_data = 1;

# return also lines that cannot be parsed ( eg. Notifications and debug messages )
my $config_nonparsed_lines = 1;

# list device names or IDs that are to be ignored ( both when they send, or when messages are for them )
#my @ignore_devices = ( "LCM", 0x18 );
my @ignore_devices = ( );

# list of command names or ids that are ignored in output
#my @ignore_commands = ( 0x7D, "RAD_TV_STATUS" );
my @ignore_commands = ( );

# end of configuration
###########################################################

my $time_offset = 0;

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
	"43" => "GTR",	# Graphics driver for rear screen (in navigation system)
	"44" => "EWS",	# EWS (Immobiliser)
	"46" => "CID",	# Central information display (flip-up LCD screen)
	"50" => "MFL",	# Multi function steering wheel
	"51" => "SM1",	# Seat memory - 1
	"53" => "MUL",	# Multicast, broadcast address
	"5B" => "IHK",	# HVAC
	"60" => "PDC",	# Park Distance Control
	"66" => "ALC",	# Active Light Control
	"68" => "RAD",	# Radio
	"69" => "EKM",	# Electronic Body Module
	"6A" => "DSP",	# DSP
	"6B" => "HEAT",	# Webasto
	"71" => "SM0",	# Seat memory - 0
	"72" => "SM0",	# Seat memory - 0
	"73" => "SDRS",	# Sirius Radio
	"76" => "CDCD",	# CD changer, DIN size.
	"7F" => "NAVE",	# Navigation (Europe)
	"80" => "IKE",	# Instrument cluster electronics
	"A0" => "MIDR",	# Rear Multi-info display
	"A4" => "MRS",	# Multiple Restraint System
	"B0" => "SES",	# Speech Input System
	"BB" => "NAVJ",	# Navigation (Japan)
	"BF" => "GLO",	# Global, broadcast address
	"C0" => "MID",	# Multi-info display
	"C8" => "TEL",	# Telephone
	"CA" => "TCU",	# BMW Assist
	"D0" => "LCM",	# Light control module
	"E0" => "IRI",	# Integrated radio information system
	"E7" => "ANZV",	# Displays Multicast
	"E8" => "RLS",	# Rain/Driving Light Sensor
	"EA" => "DSPC",	# DSP Controler
	"ED" => "VM",	# Video Module
	"F0" => "BMBT",	# On-board monitor
	"FF" => "LOC"	# Local
);

my %cmd = (
	"00" => "GET_STATUS",
	"01" => "STATUS_REQ",
	"02" => "STATUS_RESP",
	"07" => "PDC_STATUS",
	"0B" => "IO_STATUS",
	"0C" => "DIA_JOB_REQUEST",
	"10" => "IGN_STATUS_REQ",
	"11" => "IGN_STATUS_RESP",
	"12" => "SENSOR_REQ",
	"13" => "SENSOR_RESP",
	"14" => "REQ_VEHICLE_TYPE",
	"15" => "RESP_VEHICLE_CONFIG",
	"18" => "SPEED_RPM_UPDATE",
	"19" => "TEMP_UPDATE",
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
	"59" => "RLS_STATUS",
	"5A" => "INDICATORS_REQ",
	"5B" => "INDICATORS_RESP",
	"5C" => "INSTRUMENT_BACKLIGHTING",
	"60" => "WRITE_INDEX",
	"61" => "WRITE_INDEX_TMC",
	"62" => "WRITE_ZONE",
	"63" => "WRITE_STATIC",
	"74" => "IMMOBILISER_STATUS",
	"79" => "DOORS_STATUS_REQUEST",
	"7A" => "DOORS_STATUS_RESP",
	"9F" => "DIA_DIAG_TERMINATE",
	"A0" => "DIA_DIAG_RESPONSE",
	"A2" => "TELEMATICS_COORDINATES",
	"A4" => "TELEMATICS_LOCATION",
	"A5" => "WRITE_WITH_CURSOR",
	"A7" =>	"TMC_REQUEST",
	"A8" =>	"TMC_RESPONSE",
	"A9" =>	"BMW_ASSIST_DATA",
	"AA" => "NAV_CONTROL_REAR",
	"AB" => "NAV_CONTROL_FRONT",
	"C0" => "C43_SET_MENU_MODE"
);

sub hex_string_to_array {
	my ($string, $len) = @_;
	my @data;
	my $cnt = 0;

	foreach(split(" ",$string)) {
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

sub data_parsers_module_status {
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

	$variant = $variants{$src}{$variant} || $variant;
	return "announce=$announce, variant=$variant";
};

sub data_parsers_mfl_buttons {
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

	$button = $buttons{$button} || $button;
	$state = $states{$state} || $state;

	return "button=$button, state=$state";
}

sub data_parsers_bmbt_buttons {
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

	$button = $buttons{$button} || $button;
	$state = $states{$state} || $state;

	return "button=$button, state=$state";
}

sub data_parsers_bmbt_soft_buttons {
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
	$button = $buttons{$button} || $button;
	$state = $states{$state} || $state;

	return "button=$button, state=$state, extra=$data->[0]";
}

sub data_parsers_volume {
	my ($src, $dst, $string, $data) = @_;
	my $direction = $data->[0] & 0b0000_0001;
	my $steps = ($data->[0] & 0b1111_0000) >> 4;

	return "volume_change=".(($direction==0)?'-':'+').$steps;
}

sub data_parsers_navi_knob {
	my ($src, $dst, $string, $data) = @_;
	my $direction = $data->[0] & 0b1000_0000;
	my $steps = $data->[0] & 0b0000_1111;

	return "turn=".(($direction==0)?'-':'+').$steps;
}

sub data_parsers_monitor_control {
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

	$source = $sources{$source} || $source;
	$encoding = $encodings{$encoding} || $encoding;
	$aspect = $aspects{$aspect} || $aspect;

	return "power=$power, source=$source, aspect=$aspect, enc=$encoding";
}

sub data_parsers_request_screen {
	my ($src, $dst, $string, $data) = @_;

	my $priority = $data->[0] & 0b0000_0001;
	my $hide_header = ($data->[0] & 0b0000_0010) >> 1;
	my $hide_body = $data->[0] & 0b0000_1100;

	my %bodies = (
		0b0000_0100 => "HIDE_BODY_SEL",
		0b0000_1000 => "HIDE_BODY_TONE",
		0b0000_1100 => "HIDE_BODY_MENU",
	);

	$hide_body = $bodies{$hide_body} || $hide_body;

	return "priority=".(($priority==0)?"RAD":"GT").", hide_header=".(($hide_header==1)?"HIDE":"SHOW").", hide=$hide_body";
}

sub data_parsers_set_radio_ui {
	my ($src, $dst, $string, $data) = @_;

	my $priority = $data->[0] & 0b0000_0001;
	my $audio_obc = ($data->[0] & 0b0000_0010) >> 1;
	my $new_ui = ($data->[0] & 0b0001_0000) >> 4;
	my $new_ui_hide = ($data->[0] & 0b1000_0000) >> 7;

	return "priority=".(($priority==0)?"RAD":"GT").", audio+obc=$audio_obc, new_ui=$new_ui, new_ui_hide=$new_ui_hide";
}

sub data_parsers_gt_write {
	my ($src, $dst, $string, $data) = @_;

	my $layout = $data->[0];
	my $function = $data->[1];
	my $index = $data->[2] & 0b0001_1111;
	my $clear = ($data->[2] & 0b0010_0000 ) >> 5;
	my $buffer = ($data->[2] & 0b0100_0000 ) >> 6;
	my $highlight = ($data->[2] & 0b1000_0000 ) >> 7;

	my $text = "";
	for (my $i = 3; $i<length(\$data); $i++) {
		if ($data->[$i] == 0) {
			last;
		}
		$text .= chr($data->[$i]);
	}

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

	$layout = $layouts{$layout} || $layout;
#	$function = $functions{$function} || $function;

	$text = cleanup_string($text);

	return "layout=$layout, func/cursor=$function, index=$index, clear=$clear, buffer=$buffer, highlight=$highlight, text=\"$text\"";

}

sub data_parsers_gt_write_menu {
	my ($src, $dst, $string, $data) = @_;

	my $source = ($data->[0] & 0b1110_0000) >> 5;
	my $config = ($data->[0] & 0b1111_1111);
	my $option = ($data->[1] & 0b0011_1111);

	my $text = "";
	for (my $i = 2; $i<length(\$data); $i++) {
		if ($data->[$i] == 0) {
			last;
		}
		$text .= chr($data->[$i]);
	}

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


	$source = $sources{$source} || $source;
	$config = $configs{$config} || $config;

	$text = cleanup_string($text);

	return "source=$source, config=$config, options=$option, text=\"$text\"";
}

sub data_parsers_obc_text {
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

	my $text = "";
	for (my $i = 2; $i<length(\$data); $i++) {
		if ($data->[$i] == 0) {
			last;
		}
		$text .= chr($data->[$i]);
	}

	$text = cleanup_string($text);

	$property = $properties{$property} || $property;


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
}

sub data_parsers_cdc_request {
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

	$command = $commands{$command} || $command;
	return "command=$command";
};

sub data_parsers_cdc_response {
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

	$status = $statuses{$status} || $status;
	$audio = $audios{$audio} || $audio;
	$error = $errors{$error} || $error;

	return sprintf("status=%s, audio=%s, error=%s, magazines=%06b, disk=%x, track=%x", $status, $audio, $error, $magazine, $disk, $track);
};

sub data_parsers_speed_rpm {
	my ($src, $dst, $string, $data) = @_;

	my $speed = $data->[0] * 2;
	my $rpm = $data->[1] * 100;

	return "speed=$speed km/h, rpm=$rpm";
};

sub data_parsers_ike_temp {
	my ($src, $dst, $string, $data) = @_;

	my $coolant = $data->[1];
	my $amb = $data->[0];
	if ($amb>127) {
		$amb = $amb-256;
	}
	return "coolant=$coolant, amb=$amb C";
};

sub data_parsers_ike_sensor {
	my ($src, $dst, $string, $data) = @_;

	my $handbrake 		= ( $data->[0] & 0b0000_0001 );
	my $oil_pressure 	= ( $data->[0] & 0b0000_0010 ) >> 1;
	my $brake_pads 		= ( $data->[0] & 0b0000_0100 ) >> 2;
	my $transmission 	= ( $data->[0] & 0b0001_0000 ) >> 4;

	my $ignition		= ( $data->[1] & 0b0000_0001 ) ;
	my $door 			= ( $data->[1] & 0b0000_0010 ) >> 1;
	my $gear		 	= ( $data->[1] & 0b1111_0000 ) >> 4;

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

	$gear = $gears{$gear} || $gear;

	return "handbrake=$handbrake, ignition=$ignition, gear=$gear, ?door=$door, , aux_vent=$aux_vent, warn_oil_pressure=$oil_pressure, warn_brake_pads=$brake_pads, warn_transmission=$transmission";
};

sub data_parsers_ike_ignition {
	my ($src, $dst, $string, $data) = @_;

	my $ignition = ( $data->[0] & 0b0000_0111 );

	my %ign = (
		0b0000_0000   => "POS_0",
		0b0000_0001   => "POS_1",
		0b0000_0011   => "POS_2",
		0b0000_0111   => "POS_3",
	);

	$ignition = $ign{$ignition} || $ignition;

	return "ignition=$ignition";
};

sub data_parsers_redundant_data {
	my ($src, $dst, $string, $data) = @_;

	my $odo  = ( $data->[0]*256+$data->[1] ) * 100;
	my $fuel = $data->[3]*10;
	my $oil  = ( $data->[4]*256+$data->[5] );
	my $time = ( $data->[6]*256+$data->[7] );

	return "odo=$odo km, fuel=$fuel l, ?oil=$oil, time=$time days";
};

sub data_parsers_lcm_redundant_data {
	my ($src, $dst, $string, $data) = @_;

	my $odo  = ( $data->[5]*256+$data->[6] ) * 100;
	my $fuel = $data->[8]*10;
	my $oil  = ( $data->[9]*256+$data->[10] );
	my $time = ( $data->[11]*256+$data->[12] );
	my $vin  = sprintf("%c%c%02X%02X%1X", $data->[0], $data->[1], $data->[2],  $data->[3], $data->[4]>>4);

	return "vin=$vin, odo=$odo km, fuel=$fuel l, ?oil=$oil, time=$time days";
};

sub data_parsers_doors_status {
	my ($src, $dst, $string, $data) = @_;

	my $doors 			= ( $data->[0] & 0b0000_1111 );
	my $central_lock 	= ( $data->[0] & 0b0011_0000 ) >> 4;
	my $lamp_interiour	= ( $data->[0] & 0b0100_0000 ) >> 6;

	my $windows			= ( $data->[1] & 0b0000_1111 ) ;
	my $sunroof			= ( $data->[1] & 0b0001_0000 ) >> 4;
	my $rear_trunk	 	= ( $data->[1] & 0b0010_0000 ) >> 5;
	my $front_boot	 	= ( $data->[1] & 0b0100_0000 ) >> 6;

	my %locks = (
		0b0001 => "CENTRAL_LOCKING_UNLOCKED",
		0b0010 => "CENTRAL_LOCKING_LOCKED",
		0b0011 => "CENTRAL_LOCKING_ARRESTED",
	);

	$central_lock = $locks{$central_lock} || $central_lock;

	return sprintf("doors=%04b, windows=%04b, locks=%s, lamp=$lamp_interiour, sunroof=$sunroof, front_lid=$front_boot, rear_lid=$rear_trunk", $doors, $windows, $central_lock);
};

my %data_parsers = (
	"BMBT_STATUS_RESP" => \&data_parsers_module_status,
	"TEL_STATUS_RESP" => \&data_parsers_module_status,
	"NAVE_STATUS_RESP" => \&data_parsers_module_status,
	"GT_STATUS_RESP" => \&data_parsers_module_status,

	"BMBT_MONITOR_CONTROL" => \&data_parsers_monitor_control,

	"TEL_BTN_PRESS" => \&data_parsers_mfl_buttons,
	"RAD_BTN_PRESS" => \&data_parsers_mfl_buttons,

	"GT_BUTTON" => \&data_parsers_bmbt_buttons,
	"BMBT_BROADCAST_BUTTON" => \&data_parsers_bmbt_buttons,
	"RAD_BUTTON" => \&data_parsers_bmbt_buttons,

	"BMBT_SOFT_BUTTON" => \&data_parsers_bmbt_soft_buttons,

	"RAD_VOLUME" => \&data_parsers_volume,
	"TEL_VOLUME" => \&data_parsers_volume,

	"BMBT_DIAL_KNOB" => \&data_parsers_navi_knob,
	"GT_DIAL_KNOB" => \&data_parsers_navi_knob,

	"GT_SCREEN_MODE_REQUEST" => \&data_parsers_request_screen,
	"RAD_SCREEN_MODE_SET" => \&data_parsers_set_radio_ui,

	"GT_WRITE_WITH_CURSOR" => \&data_parsers_gt_write,
	"GT_WRITE_MENU" => \&data_parsers_gt_write,

	"GT_WRITE_TITLE" =>  \&data_parsers_gt_write_menu,
	"IKE_WRITE_TITLE" =>  \&data_parsers_gt_write_menu,
	"RAD_BROADCAST_WRITE_TITLE" =>  \&data_parsers_gt_write_menu,

	"IKE_BROADCAST_OBC_TEXT" => \&data_parsers_obc_text,

	"CDC_RESPONSE" => \&data_parsers_cdc_response,
	"CDC_REQUEST" => \&data_parsers_cdc_request,

	"IKE_BROADCAST_SPEED_RPM_UPDATE" => \&data_parsers_speed_rpm,
	"IKE_BROADCAST_TEMP_UPDATE" => \&data_parsers_ike_temp,
	"IKE_BROADCAST_SENSOR_RESP" => \&data_parsers_ike_sensor,
	"IKE_BROADCAST_IGN_STATUS_RESP" => \&data_parsers_ike_ignition,
	"IKE_BROADCAST_REPLICATE_REDUNDANT_DATA" => \&data_parsers_redundant_data,
	"LCM_RESP_REDUNDANT_DATA" => \&data_parsers_lcm_redundant_data,

	"GM_BROADCAST_DOORS_STATUS_RESP" => \&data_parsers_doors_status,

);

sub local_time {
	my ($time) = @_;

	if ($config_local_time) {
		my $time_local;
		if ($time_offset != 0) {
			$time_local = $time + $time_offset;
			my $sec = int($time_local/1000) % 60;
			my $min = int($time_local/(60*1000)) % 60;
			my $hour = int($time_local/(60*60*1000));

			$time_local = sprintf("%2d:%02d:%02d", $hour, $min, $sec);
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

while (<>) {
	chomp;
	s/[\n\s\r]+$//o;

	my $line = $_;

	if (/^\[(\d+)\]\s+DEBUG:\s+IBus:\s+RX\[(\d+)\]:\s+?(..)\s+(..)\s+(..)\s+(..)\s*(.*?)$/osi) {	
# IBUS message
		my $time = $1;
		my $len = $2;
		my $packet = "$3 $4 $5 $6 $7";
		my $src = $bus{$3} || "0x".$3;
		my $dst = $bus{$5} || "0x".$5;
		my $src_orig = hex($3);
		my $dst_orig = hex($5);
		my $cmd_raw = $6;
		my $data = $7;

		next if (in_array($src, \@ignore_devices) || in_array($dst, \@ignore_devices) || in_array($src_orig, \@ignore_devices) || in_array($dst_orig, \@ignore_devices));
		next if (in_array($cmd_raw, \@ignore_commands) || in_array(hex($cmd_raw), \@ignore_commands));

		my $cmd_assumed;
		my $cmd;
		my $broadcast = " ";

		my $time_local = local_time($time);

		if ($dst eq "LOC" || $dst eq "GLO" || $dst eq "MUL" || $dst eq "ANZV") {
			$broadcast = "B";

			$cmd_assumed = $src."_BROADCAST_".$cmd_raw;
			if ($cmd{$cmd_assumed}) {
				$cmd = $cmd{$cmd_assumed};
			} elsif ($cmd{$cmd_raw}) {
				$cmd = $src."_BROADCAST_".$cmd{$cmd_raw};
			} else { 
				$cmd = $cmd_assumed;
			};
		} elsif ($cmd{$cmd_raw} =~ /RESP/io) {
				$cmd = $src."_".$cmd{$cmd_raw};
		} else {
			$cmd_assumed = $dst."_".$cmd_raw;
			if ($cmd{$cmd_assumed}) {
				$cmd = $cmd{$cmd_assumed};
			} else {
				$cmd_assumed = $src."_".$cmd_raw;
				if ($cmd{$cmd_assumed}) {
					$cmd = $cmd{$cmd_assumed};
				} elsif ($cmd{$cmd_raw}) {
					$cmd = $dst."_".$cmd{$cmd_raw};
				} else { 
					$cmd = $dst."_".$cmd_raw;
				};
			}
		}

		next if (in_array($cmd, \@ignore_commands));

		my $self = " ";

		if ($data =~ s/\s+\[SELF\]//o) {
			$self = "*";
		}

		$data =~ s/\s*..$//;

		my $sec = ($time % (60*1000))/1000;
		my $min = int($time/(60*1000)) % 60;
		my $hour = int($time/(60*60*1000));

		my $data_parsed;
		if ($data_parsers{$cmd}) {
			my @data = hex_string_to_array($data,$len - 5);
			if (scalar(@data) > 0) {
				$data_parsed = $data_parsers{$cmd}->($src,$dst, $data, \@data, $time);
			} else {
				$data_parsed = "";
			}
		} else {
			$data_parsed = $data;
		}

		if ($config_original_line) {
			print $line."\n";
		}

		if ($config_local_time) {
			printf ("%2d:%02d:%06.3f (%s) ",$hour, $min, $sec, $time_local);
		} else {
			printf ("%2d:%02d:%06.3f ",$hour, $min, $sec);
		};

		printf ("%1s %1s %4s -> %-4s %2s %s (%s)", $self, $broadcast, $src, $dst, $cmd_raw, $cmd, $data_parsed);
		if ($config_original_data) {
			printf (" [%s]", $packet);
		};
		print "\n";

	} elsif (/^\[(\d+)\]\s+DEBUG:\s+BT:\s+([RW]):\s+'(\S+)\s*(.*)\s*'$/os) {
# BlueTooth BC127 Messages
		my $time = $1;
		my $command = $3;
		my $data = $4;
		my $src;
		my $dst;
		my $self;

		if ($2 eq 'R') {
			$src = "BT";
			$dst = "BBUS";
			$self = " ";
		} else {
			$src = "BBUS";
			$dst = "BT";
			$self = "*";
		}

		next if (in_array($src, \@ignore_devices) || in_array($dst, \@ignore_devices));
		next if (in_array($command, \@ignore_commands));

		my $time_local = local_time($time);

		my $sec = ($time % (60*1000))/1000;
		my $min = int($time/(60*1000)) % 60;
		my $hour = int($time/(60*60*1000));

		if ($config_original_line) {
			print $line."\n";
		};

		if ($config_local_time) {
			printf ("%2d:%02d:%06.3f (%s) ",$hour, $min, $sec, $time_local);
		} else {
			printf ("%2d:%02d:%06.3f ",$hour, $min, $sec);
		};

		printf ("%1s   %4s -> %-4s    %s %s\n", $self, $src, $dst, $command, $data);
	} else {
		print $line."\n" if ($config_nonparsed_lines);
	}
};
