#!/usr/bin/python
from __future__ import print_function
import sys
from argparse import ArgumentParser
from intelhex import IntelHex
from serial import Serial
from struct import pack
from time import sleep, time

PROTOCOL_CMD_PLATFORM_REQUEST = 0x00
PROTOCOL_CMD_PLATFORM_RESPONSE = 0x01
PROTOCOL_CMD_WRITE_DATA_REQUEST = 0x02
PROTOCOL_CMD_WRITE_DATA_RESPONSE_OK = 0x03
PROTOCOL_CMD_WRITE_DATA_RESPONSE_ERR = 0x04
PROTOCOL_CMD_BC127_MODE_REQUEST = 0x05
PROTOCOL_CMD_BC127_MODE_RESPONSE = 0x06
PROTOCOL_CMD_START_APP_REQUEST = 0x07
PROTOCOL_CMD_START_APP_RESPONSE = 0x08
PROTOCOL_CMD_WRITE_SN_REQUEST = 0x09
PROTOCOL_CMD_WRITE_SN_RESPONSE_OK = 0x0A
PROTOCOL_CMD_WRITE_SN_RESPONSE_ERR = 0x0B
PROTOCOL_BAD_PACKET_RESPONSE = 0xFF
rx_buffer = []
tx_buffer = []
TIMEOUT = 10

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

def start_app():
    for i in generate_packet(PROTOCOL_CMD_START_APP_REQUEST, [0x00]):
        tx_buffer.append(i)

def read_hexfile(filename):
    hp = HexParser(filename)
    address = 0
    data = []
    while address < (0x55e00 - 0x400) & bitwise_not(0x400 - 1):
        row_data = []
        addr_bytes = [ord(b) for b in pack('>I', address)]
        addr_bytes.pop(0)
        for b in addr_bytes:
            row_data.append(b)
        for addr in range(82 * 2):
            addr = addr + address
            if addr % 2 == 0:
                op_bytes = [ord(b) for b in pack('>I', hp.get_opcode(addr))]
                op_bytes.pop(0)
                for b in op_bytes:
                    row_data.append(b)
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
            required=True,
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
        args = parser.parse_args()
        serial = Serial(args.port, 115200)
        request_platform()
        should_continue = True
        data = None
        data_len = 0
        data_idx = 0
        start = int(time())
        has_response = False
        while should_continue:
            while serial.in_waiting:
                rx_buffer.append(serial.read())
                if len(rx_buffer) >= 2:
                    if len(rx_buffer) == ord(rx_buffer[1]):
                        command = int(hex(ord(rx_buffer.pop(0))), 16)
                        _ = rx_buffer.pop(0) # Remove the length
                        xor = rx_buffer.pop() # Remove the XOR
                        if command == PROTOCOL_CMD_PLATFORM_RESPONSE:
                            print('Got Platform: %s' % ''.join(rx_buffer))
                            has_response = True
                            if args.firmware:
                                print('==== Begin Firmware Update ====')
                                print(
                                    'Erasing Flash - DO NOT unplug the device'
                                )
                                data = read_hexfile(args.firmware)
                                data_len = len(data)
                                send_file(data[data_idx])
                            elif args.btmode:
                                print('==== Requesting BC127 Mode ====')
                                request_bc127_mode();
                            else:
                                sys.exit(0)
                        if command == PROTOCOL_CMD_BC127_MODE_RESPONSE:
                            print('BC127 Mode Started')
                            exit(0)
                        if command == PROTOCOL_CMD_WRITE_DATA_RESPONSE_OK:
                            if data_idx == 0:
                                print('Flash Erase Complete')
                            percent = 0
                            if data_idx > 0:
                                percent = data_len / data_idx
                            print('\rWriting Flash Block %d of %d' % (data_idx, data_len), end='\r')
                            data_idx += 1
                            if (data_idx < data_len):
                                send_file(data[data_idx])
                            else:
                                print('Write Complete')
                                print ('Starting App...')
                                start_app()
                        if command == PROTOCOL_CMD_START_APP_RESPONSE:
                            print('App Started')
                            sys.exit(0)
                        if command == PROTOCOL_BAD_PACKET_RESPONSE:
                            print("ERR: Bad Packet - Please Try again")
                        if command == PROTOCOL_CMD_WRITE_DATA_RESPONSE_ERR:
                            print("ERR: Write Failed - Please try again")
                        rx_buffer = []
            if len(tx_buffer):
                serial.write(tx_buffer)
                tx_buffer = []
            if not has_response and int(time()) - start > TIMEOUT:
                print(
                    'ERR: Failed to get a response from the device within 10 '
                    'seconds. Is the device in bootloader mode?'
                )
                should_continue = False
    except KeyboardInterrupt:
        sys.exit(0)
