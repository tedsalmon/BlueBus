#!/usr/bin/env python3
import platform
import serial.tools.list_ports as list_ports
import sys

from argparse import ArgumentParser
from datetime import date
from glob import glob
from intelhex import IntelHex
from serial import Serial, PARITY_ODD, serialutil
from struct import pack
from time import time, sleep

BLUEBUS_MAX_MEMORY_ADDR = 0xAA800
PROTOCOL_CMD_PLATFORM_REQUEST = 0x00
PROTOCOL_CMD_PLATFORM_RESPONSE = 0x01
PROTOCOL_CMD_ERASE_FLASH_REQUEST = 0x02
PROTOCOL_CMD_ERASE_FLASH_RESPONSE = 0x03
PROTOCOL_CMD_WRITE_DATA_REQUEST = 0x04
PROTOCOL_CMD_WRITE_DATA_RESPONSE_OK = 0x05
PROTOCOL_CMD_WRITE_DATA_RESPONSE_ERR = 0x06
PROTOCOL_CMD_BC127_MODE_REQUEST = 0x07
PROTOCOL_CMD_BC127_MODE_RESPONSE = 0x08
PROTOCOL_CMD_START_APP_REQUEST = 0x09
PROTOCOL_CMD_START_APP_RESPONSE = 0x0A
PROTOCOL_CMD_FIRMWARE_VERSION_REQUEST = 0x0B
PROTOCOL_CMD_FIRMWARE_VERSION_RESPONSE = 0x0C
PROTOCOL_CMD_READ_SN_REQUEST = 0x0D
PROTOCOL_CMD_READ_SN_RESPONSE = 0x0E
PROTOCOL_CMD_WRITE_SN_REQUEST = 0x0F
PROTOCOL_CMD_WRITE_SN_RESPONSE_OK = 0x10
PROTOCOL_CMD_WRITE_SN_RESPONSE_ERR = 0x11
PROTOCOL_CMD_READ_BUILD_DATE_REQUEST = 0x12
PROTOCOL_CMD_READ_BUILD_DATE_RESPONSE = 0x13
PROTOCOL_CMD_WRITE_BUILD_DATE_REQUEST = 0x14
PROTOCOL_CMD_WRITE_BUILD_DATE_RESPONSE_OK = 0x15
PROTOCOL_CMD_WRITE_BUILD_DATE_RESPONSE_ERR = 0x16
PROTOCOL_ERR_PACKET_TIMEOUT = 0xFE
PROTOCOL_BAD_PACKET_RESPONSE = 0xFF

rx_buffer = []
tx_buffer = []
last_tx = []
TIMEOUT = 1

def bitwise_not(n, width=32):
    return (1 << width) - 1 - n

class HexParser(object):
    def __init__(self, filename):
        self.memory_map = IntelHex(filename)

    def get_opcode(self, address):
        if address % 2 != 0:
            raise ValueError('Address must be even')
        addr = address << 1
        value = self.memory_map[addr]
        value += self.memory_map[addr + 1] << 8
        value += self.memory_map[addr + 2] << 16
        value += self.memory_map[addr + 3] << 24
        return value

def generate_packet(command, data):
    packet = [command, (len(data) + 3)]
    for d in data:
        packet.append(d)
    chk = 0
    for i in packet:
        if type(i) == str:
            i = ord(i)
        chk ^= i
    packet.append(chk)
    return packet

def request_bc127_mode():
    for i in generate_packet(PROTOCOL_CMD_BC127_MODE_REQUEST, [0x00]):
        tx_buffer.append(i)

def request_platform():
    for i in generate_packet(PROTOCOL_CMD_PLATFORM_REQUEST, [0x00]):
        tx_buffer.append(i)

def request_erase_flash():
    for i in generate_packet(PROTOCOL_CMD_ERASE_FLASH_REQUEST, [0x00]):
        tx_buffer.append(i)

def start_app():
    for i in generate_packet(PROTOCOL_CMD_START_APP_REQUEST, [0x00]):
        tx_buffer.append(i)

def read_build():
    for i in generate_packet(PROTOCOL_CMD_READ_BUILD_DATE_REQUEST, [0x00]):
        tx_buffer.append(i)

def read_sn():
    for i in generate_packet(PROTOCOL_CMD_READ_SN_REQUEST, [0x00]):
        tx_buffer.append(i)

def read_firmware():
    for i in generate_packet(PROTOCOL_CMD_FIRMWARE_VERSION_REQUEST, [0x00]):
        tx_buffer.append(i)

def write_build(week, year):
    for i in generate_packet(PROTOCOL_CMD_WRITE_BUILD_DATE_REQUEST, [week, year]):
        tx_buffer.append(i)

def write_sn(sn_msb, sn_lsb):
    for i in generate_packet(PROTOCOL_CMD_WRITE_SN_REQUEST, [sn_msb, sn_lsb]):
        tx_buffer.append(i)

def read_hexfile(filename):
    hp = HexParser(filename)
    address = 0x1800
    data = []
    while address < BLUEBUS_MAX_MEMORY_ADDR & bitwise_not(0x400 - 1):
        row_data = []
        has_ops = False
        addr_bytes = [b for b in pack('>I', address)]
        addr_bytes.pop(0)
        for b in addr_bytes:
            row_data.append(b)
        for addr in range(82 * 2):
            addr = addr + address
            if addr % 2 == 0:
                op_bytes = [b for b in pack('>I', hp.get_opcode(addr))]
                op_bytes.pop(0)
                for b in op_bytes:
                    row_data.append(b)
                    if b != 255:
                        has_ops = True
        if has_ops:
            data.append(row_data)
        address += 82 << 1
    return data

def send_file(data):
    pkt = generate_packet(PROTOCOL_CMD_WRITE_DATA_REQUEST, data)
    for i in pkt:
        tx_buffer.append(i)

if __name__ == '__main__':
    try:
        parser = ArgumentParser(description='Interact with the BlueBus Bootloader')
        parser.add_argument(
            '--port',
            metavar='port',
            type=str,
            help='The port (COMx) or tty (/dev/ttyACMx or /dev/ttyUSBx) to use',
        )
        parser.add_argument(
            '--firmware',
            metavar='hexfile',
            help='The path to the firmware file to upload to the device',
        )
        parser.add_argument(
            '--btmode',
            help='Switch UART to the BC127',
            action='store_true',
        )
        parser.add_argument(
            '--sn',
            help='Serial Number',
        )
        parser.add_argument(
            '--getsn',
            help='Get Serial Number',
            action='store_true',
        )
        parser.add_argument(
            '--getfw',
            help='Get Firmware Version',
            action='store_true',
        )
        parser.add_argument(
            '--getbuild',
            help='Get Build Date',
            action='store_true',
        )
        parser.add_argument(
            '--writebuild',
            help='Get Build Date',
            action='store_true',
        )
        parser.add_argument(
            '--start',
            help='Start Application',
            action='store_true',
        )
        args = parser.parse_args()
        # Automatic port selection
        ports = []
        if platform.system() == 'Linux':
            for f in glob('/dev/ttyUSB*'):
                ports.append(f)
        elif platform.system() == 'Darwin':
            for f in glob('/dev/tty.usbserial*'):
                ports.append(f)
        else:
            for port in list_ports.comports():
                try:
                    Serial(port.device)
                    ports.append(port.device)
                except serialutil.SerialException:
                    pass
        if not args.port and len(ports) == 1:
            args.port = ports.pop()
        if not args.port:
            if len(ports) == 0:
                print('Error: BlueBus Not detected')
            else:
                print('Error: Too many devices available')
            exit(0)
        serial = Serial(args.port, 115200, parity=PARITY_ODD)
        should_continue = True
        data = None
        data_len = 0
        data_idx = 0
        if args.firmware:
            data = read_hexfile(args.firmware)
            if not data:
                print('ERR: Could not read firmware file')
                exit(0)
            data_len = len(data)
        retries = 0
        start = int(time())
        has_response = False
        request_platform()
        while should_continue:
            while serial.in_waiting:
                rx_buffer.append(serial.read())
                if len(rx_buffer) >= 2:
                    if len(rx_buffer) == ord(rx_buffer[1]):
                        command = int(hex(ord(rx_buffer.pop(0))), 16)
                        _ = rx_buffer.pop(0) # Remove the length
                        xor = rx_buffer.pop() # Remove the XOR
                        if command == PROTOCOL_CMD_PLATFORM_RESPONSE:
                            string = [b.decode('ascii') for b in rx_buffer]
                            print('Got Platform: %s' % ''.join(string))
                            has_response = True
                            if args.firmware:
                                print('==== Begin Firmware Update ====')
                                print('Erasing Flash...')
                                request_erase_flash()
                            elif args.btmode:
                                print('==== Requesting BC127 Mode ====')
                                request_bc127_mode()
                            elif args.getsn:
                                read_sn()
                            elif args.getbuild:
                                read_build()
                            elif args.getfw:
                                read_firmware()
                            elif args.writebuild:
                                write_build(today[1], today[0] - 2000)
                            elif args.start:
                                start_app()
                            elif args.sn:
                                serial_number = int(args.sn)
                                msb = (serial_number >> 8) & 0xFF
                                lsb = serial_number & 0xFF
                                write_sn(msb, lsb)
                            else:
                                sys.exit(0)
                        if command == PROTOCOL_CMD_BC127_MODE_RESPONSE:
                            print('BC127 Mode Started')
                            sys.exit(0)
                        if command == PROTOCOL_CMD_ERASE_FLASH_RESPONSE:
                            send_file(data[data_idx])
                        if command == PROTOCOL_CMD_WRITE_DATA_RESPONSE_OK:
                            percent = 0
                            if data_idx > 0:
                                percent = data_len / data_idx
                            print('\rWriting Flash Block %d of %d' % (data_idx, data_len - 1))
                            data_idx += 1
                            if (data_idx < data_len):
                                send_file(data[data_idx])
                            else:
                                print ('\nDone')
                                today = date.today().isocalendar()
                                write_build(today[1], today[0] - 2000)
                        if command == PROTOCOL_CMD_READ_SN_RESPONSE:
                            serial_number = ord(rx_buffer[0]) << 8 | ord(rx_buffer[1])
                            print('SN: %d' % serial_number)
                            sys.exit(0)
                        if command == PROTOCOL_CMD_WRITE_SN_RESPONSE_OK:
                            print('Wrote SN')
                            start_app()
                        if command == PROTOCOL_CMD_WRITE_SN_RESPONSE_ERR:
                            print('Could not write SN')
                            start_app()
                        if command == PROTOCOL_CMD_READ_BUILD_DATE_RESPONSE:
                            print('Build: %d/%d' % (ord(rx_buffer[0]), ord(rx_buffer[1])))
                            sys.exit(0)
                        if command == PROTOCOL_CMD_FIRMWARE_VERSION_RESPONSE:
                            print('Firmware: %d.%d.%d' % (ord(rx_buffer[0]), ord(rx_buffer[1]), ord(rx_buffer[2])))
                            sys.exit(0)
                        if command == PROTOCOL_CMD_WRITE_BUILD_DATE_RESPONSE_OK:
                            print('Wrote Build Date')
                            if args.sn:
                                serial_number = int(args.sn)
                                msb = (serial_number >> 8) & 0xFF
                                lsb = serial_number & 0xFF
                                write_sn(msb, lsb)
                            else:
                                start_app()
                        if command == PROTOCOL_CMD_WRITE_BUILD_DATE_RESPONSE_ERR:
                            print('Could not write build date')
                            if args.sn:
                                serial_number = int(args.sn)
                                msb = (serial_number >> 8) & 0xFF
                                lsb = serial_number & 0xFF
                                write_sn(msb, lsb)
                            else:
                                start_app()
                        if command == PROTOCOL_CMD_START_APP_RESPONSE:
                            print('App Started')
                            sys.exit(0)
                        if command == PROTOCOL_ERR_PACKET_TIMEOUT:
                            print("ERR: Packet Timeout")
                            exit(1)
                        if command == PROTOCOL_BAD_PACKET_RESPONSE:
                            print("ERR: Please try again")
                            tx_buffer = list(last_tx)
                        if command == PROTOCOL_CMD_WRITE_DATA_RESPONSE_ERR:
                            print("ERR: Write Failed - Please try again")
                        rx_buffer = []
            if len(tx_buffer):
                serial.write(tx_buffer)
                last_tx = list(tx_buffer)
                tx_buffer = []
            if not has_response and int(time()) - start > TIMEOUT:
                if retries <= 5:
                    start = int(time())
                    request_platform()
                    retries += 1
                else:
                    print(
                        'ERR: Failed to get a response from the device within 5 '
                        'seconds. Is the device in bootloader mode?'
                    )
                    should_continue = False
    except KeyboardInterrupt:
        sys.exit(0)
