#!/usr/bin/env python
import platform
import serial.tools.list_ports as list_ports

from glob import glob
from intelhex import IntelHex, HexRecordError
from serial import Serial, PARITY_ODD, serialutil
from struct import pack
from time import time, sleep
from tkinter import Button, Tk, Label, Text, StringVar, HORIZONTAL, BOTTOM, SUNKEN, END, W, X
from tkinter.ttk import Progressbar
from tkinter.filedialog import askopenfilename
from tk_tools import SmartOptionMenu

BLUEBUS_PLATFORMS = [
    'BLUEBUS_BOOTLOADER_1_3',
    'BLUEBUS_BOOTLOADER_1_4'
]

# Protocol definition
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
PROTOCOL_BAD_PACKET_RESPONSE = 0xFF

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

def read_hexfile(filename):
    hp = HexParser(filename)
    address = 0x1800
    data = []
    while address < (0x55e00 - 0x400) & bitwise_not(0x400 - 1):
        row_data = []
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
        data.append(row_data)
        address += 82 << 1
    return data

class Application(Tk):

    TITLE = 'BlueBus Firmware Tool'
    BAUDRATE = '115200'
    TIMEOUT = 10
    tx_buffer = []
    firmware_version = ''
    serial_num = ''
    build = ''

    def __init__(self):
        super().__init__()
        self.hex_file_path = ''
        self.port_name = None
        self.serial_port = None
        self.hex_data = None
        self.hex_data_idx = 0
        self.title(self.TITLE)
        self.geometry('400x150')
        self.rowconfigure(0, weight = 1)
        self.columnconfigure(1, weight = 1)
        self.page_title = Label(
            self,
            text=self.TITLE,
            font=('Helvetica', 14)
        )
        self.page_title.grid(row=0, column=0, columnspan=2, sticky='news')
        self.serial_port_menu = SmartOptionMenu(
            self,
            self.get_ports(),
            callback=self.select_serial_port
        )
        self.serial_port_menu.grid(row=1, column=0, sticky='ew')

        self.select_hex_button = Button(
            self,
            text='Select firmware file...',
            command=self.select_hex_file
        )
        self.select_hex_button.grid(row=2, column=0, columnspan=2, sticky='news')
        self.select_hex_button['state'] = 'disabled'

        self.flash_button = Button(
            self,
            text='Flash Firmware',
            command=self.flash_firmware
        )
        self.flash_button.grid(row=3, column=0, columnspan=2, sticky='news')
        self.flash_button['state'] = 'disabled'

        self.progress = Progressbar(self, orient=HORIZONTAL, length=100, mode='determinate')
        self.progress.grid(row=4, column=0, columnspan=2, sticky='news')
        self.statusbar = Label(
            self,
            text='Not Connected',
            bd=1,
            relief=SUNKEN,
            anchor=W
        )
        self.statusbar.grid(row=5, column=0, columnspan=2, sticky='we')
        self.after(1000, self.update_ports)
        self.mainloop()

    def get_ports(self):
        ports = ['Select Device...']
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
        return ports
    
    def update_ports(self):
        option = self.serial_port_menu.option_menu
        menu = option.children['menu']
        menu.delete(0, END)
        for port in self.get_ports():
            menu.add_command(
                label=port,
                command=lambda p=port: self.serial_port_menu.set(p)
            )
        self.after(1000, self.update_ports)

    def request_bc127_mode(self):
        for i in generate_packet(PROTOCOL_CMD_BC127_MODE_REQUEST, [0x00]):
            self.tx_buffer.append(i)
    
    def request_erase_flash(self):
        for i in generate_packet(PROTOCOL_CMD_ERASE_FLASH_REQUEST, [0x00]):
            self.tx_buffer.append(i)

    def request_flash_write(self):
        data = self.hex_data[self.hex_data_idx]
        pkt = generate_packet(PROTOCOL_CMD_WRITE_DATA_REQUEST, data)
        for i in pkt:
            self.tx_buffer.append(i)

    def request_platform(self):
        for i in generate_packet(PROTOCOL_CMD_PLATFORM_REQUEST, [0x00]):
            self.tx_buffer.append(i)

    def request_firmware_version(self):
        for i in generate_packet(PROTOCOL_CMD_FIRMWARE_VERSION_REQUEST, [0x00]):
            self.tx_buffer.append(i)

    def request_serial_number(self):
        for i in generate_packet(PROTOCOL_CMD_READ_SN_REQUEST, [0x00]):
            self.tx_buffer.append(i)

    def request_build_date(self):
        for i in generate_packet(PROTOCOL_CMD_READ_BUILD_DATE_REQUEST, [0x00]):
            self.tx_buffer.append(i)
    
    def request_start_app(self):
        for i in generate_packet(PROTOCOL_CMD_START_APP_REQUEST, [0x00]):
            self.tx_buffer.append(i)

    def select_hex_file(self):
        self.hex_file_path = askopenfilename(
            title='Select Firmware...',
            filetypes=[('hex', '*.hex')]
        )
        self.flash_button['state'] = 'disable'
        if not self.hex_file_path:
            self.set_status('ERROR: Could not find firmware file')
            return
        try:
            self.hex_data = read_hexfile(self.hex_file_path)
        except HexRecordError:
            self.set_status('ERROR: Invalid / Corrupt Firmware File')
            return
        if not self.hex_data:
            self.set_status('ERROR: Could Not Read Firmware File')
            return
        self.flash_button['state'] = 'normal'
        self.set_status('Ready to Flash')

    def select_serial_port(self, port_name):
        port_name = self.serial_port_menu.get()
        self.port_name = port_name.split('-')[0].strip()
        if self.port_name == 'Select Device...':
            return
        # Reset Info
        self.firmware_version = ''
        self.serial_num = ''
        self.build = ''
        # Reload the device and verify
        self.serial_port = Serial(port_name, self.BAUDRATE)
        self.serial_port.write(b'bootloader\r\n')
        sleep(0.1) # Wait for data to reach unit before closing the ports
        self.serial_port.close()
        self.set_status('Connecting...')
        # Allow the bootloader to spool
        sleep(1)
        # Re-open the serial port
        self.serial_port = Serial(port_name, self.BAUDRATE, parity=PARITY_ODD)
        self.serial_port.write(b'\r\n') # Clear the port
        sleep(0.25)
        while self.serial_port.in_waiting:
            if self.serial_port.read():
                pass
        self.request_platform()
        self.handle_serial()

    def flash_firmware(self):
        self.flash_button['state'] = 'disable'
        self.select_hex_button['state'] = 'disable'
        self.set_status('Erasing Flash...')
        self.request_erase_flash()
        self.handle_serial()

    def handle_serial(self):
        start = int(time())
        has_response = False
        rx_buffer = []

        while not has_response:
            while self.serial_port.in_waiting:
                rx_buffer.append(self.serial_port.read())
                if len(rx_buffer) >= 2:
                    if len(rx_buffer) == ord(rx_buffer[1]):
                        command = int(hex(ord(rx_buffer.pop(0))), 16)
                        _ = rx_buffer.pop(0) # Remove the length
                        xor = rx_buffer.pop() # Remove the XOR
                        if command == PROTOCOL_CMD_PLATFORM_RESPONSE:
                            has_response = True
                            string = [b.decode('ascii') for b in rx_buffer]
                            if ''.join(string) in BLUEBUS_PLATFORMS:
                                self.select_hex_button['state'] = 'normal'
                                # Request FW Version
                                self.request_firmware_version()
                                self.handle_serial()
                            else:
                                self.set_status('ERROR: Unsupported device')
                        if command == PROTOCOL_CMD_FIRMWARE_VERSION_RESPONSE:
                            self.firmware_version = '%d.%d.%d' % (
                                ord(rx_buffer[0]),
                                ord(rx_buffer[1]),
                                ord(rx_buffer[2])
                            )
                            self.request_serial_number()
                            has_response = True
                            self.handle_serial()
                        if command == PROTOCOL_CMD_READ_SN_RESPONSE:
                            self.serial = (ord(rx_buffer[0]) << 8) | ord(rx_buffer[1])
                            self.set_status(
                                'Current Firmware: %s / Unit Serial Number: %d' % (
                                    self.firmware_version,
                                    self.serial
                                )
                            )
                            has_response = True
                        if command == PROTOCOL_CMD_ERASE_FLASH_RESPONSE:
                            self.set_status('Writing Flash: 0%')
                            self.request_flash_write()
                        if command == PROTOCOL_CMD_WRITE_DATA_RESPONSE_OK:
                            data_size = len(self.hex_data)
                            self.hex_data_idx += 1
                            progress = (self.hex_data_idx / data_size) * 100
                            self.progress['value'] = progress
                            self.set_status('Writing Flash: %d%%' % progress)
                            if self.hex_data_idx < data_size:
                                self.request_flash_write()
                            else:
                                self.request_start_app()
                        if command == PROTOCOL_CMD_START_APP_RESPONSE:
                            self.set_status('Flash Complete')
                            self.flash_button['state'] = 'disable'
                            self.select_hex_button['state'] = 'disable'
                            self.progress['value'] = 0
                            has_response = True
                        if command == PROTOCOL_BAD_PACKET_RESPONSE:
                            self.set_status('ERROR: Protocol Error -- Try again')
                        if command == PROTOCOL_CMD_WRITE_DATA_RESPONSE_ERR:
                            self.set_status('ERROR: Write Failed - try again')
                            has_response = True
                        rx_buffer = []
                        start = int(time())
            if len(self.tx_buffer) > 0:
                self.serial_port.write(self.tx_buffer)
                self.tx_buffer = []
            if not has_response and int(time()) - start > self.TIMEOUT:
                self.set_status(
                    'ERROR: Failed to get a response within 5 seconds'
                )
                has_response = True
            self.update()

    def set_status(self, text):
        self.statusbar.config(text=text)
        self.statusbar.update()

if __name__ == '__main__':
    app = Application()
