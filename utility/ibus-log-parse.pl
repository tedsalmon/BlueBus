#!env perl
use strict;

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
	"E7" => "ANZ",	# Displays Multicast
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
	"37" => "DISPLAY_RADIO_MENU",
	"38" => "REQUEST",
	"39" => "RESPONSE",
	"3B" => "BTN_PRESS",
	"40" => "OBC_INPUT",
	"41" => "OBC_CONTROL",
	"42" => "OBC_REMOTE_CONTROL",
	"45" => "SCREEN_MODE_SET",
	"46" => "SCREEN_MODE_UPDATE",
	"47" => "SOFT_BUTTON",
	"48" => "BUTTON",
	"49" => "DIAL_KNOB",
	"4A" => "LED_TAPE_CTRL",
	"4E" => "TV_STATUS",
	"4F" => "MONITOR_CONTROL",
	"53" => "LCM_REQ_REDUNDANT_DATA",
	"54" => "LCM_RESP_REDUNDANT_DATA",
	"59" => "RLS_STATUS",
	"5A" => "LCM_INDICATORS_REQ",
	"5A" => "LCM_INDICATORS_RESP",
	"5C" => "INSTRUMENT_BACKLIGHTING",
	"60" => "WRITE_INDEX",
	"61" => "WRITE_INDEX_TMC",
	"62" => "WRITE_ZONE",
	"63" => "WRITE_STATIC",
	"74" => "IMMOBILISER_STATUS",
	"7A" => "DOORS_FLAPS_STATUS_RESP",
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


while (<>) {
	my $line = $_;
	if (/^\[(\d+)\]\s+DEBUG:\s+IBus:\s+RX\[(\d+)\]:\s+?(..)\s+..\s+(..)\s+(..)\s*(.*?)[\s\r\n]+$/osi) {	
#		print $line;
		my $time = $1;
		my $len = $2;
		my $src = $bus{$3} || "0x".$3;
		my $dst = $bus{$4} || "0x".$4;
		my $cmd_raw = $5;
		my $data = $6;

		my $cmd_assumed;
		my $cmd;

		if ($dst eq "LOC" || $dst eq "GLO" || $dst eq "MUL" || $dst eq "ANZ") {
			$cmd_assumed = $src."_BROADCAST_".$cmd_raw;
			if ($cmd{$cmd_assumed}) {
				$cmd = $cmd{$cmd_assumed};
			} elsif ($cmd{$cmd_raw}) {
				$cmd = $src."_BROADCAST_".$cmd{$cmd_raw};
			} else { 
				$cmd = $cmd_assumed;
			};
		} elsif ($cmd{$cmd_raw} =~ /RESP/i) {
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

		my $self = " ";

		if ($data =~ s/\s+\[SELF\]//o) {
			$self = "*";
		}

		$data =~ s/\s*..$//;

		my $sec = ($time % (60*1000))/1000;
		my $min = int($time/(60*1000)) % 60;
		my $hour = int($time/(60*60*1000));

		printf ("%3d:%02d:%06.3f %1s %4s -> %-4s %2s %s (%s)\n", $hour, $min, $sec, $self, $src, $dst, $cmd_raw, $cmd, $data);

	} else {
#		print;
	}
};