#!/usr/bin/env python3
import platform
import serial.tools.list_ports as list_ports
import sys

from glob import glob
from serial import Serial, PARITY_ODD, serialutil
from time import time, sleep
from tkinter import Button, Tk, Label, Text, StringVar, HORIZONTAL, BOTTOM, SUNKEN, END, W, X, TclError
from tkinter.ttk import Progressbar
from tk_tools import SmartOptionMenu

STATE_NEW = 0
STATE_VERSION_FOUND = 1
STATE_VERSION_BT = 2
STATE_LOG_OFF = 3
STATE_APPLY_CVC = 4
STATE_CVC_ON = 5
STATE_MGAIN = 6
STATE_DONE = 7

class Application(Tk):

    TITLE = 'BlueBus cVc Tool'
    BAUDRATE = '115200'
    TIMEOUT = 10
    rx_buffer = []
    mac_id = 0x00
    serial_num = ''
    state = STATE_NEW
    start_time = 0

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
        self.apply_button = Button(
            self,
            text='Apply cVc',
            command=self.apply_cvc
        )
        self.apply_button.grid(row=3, column=0, columnspan=2, sticky='news')
        self.apply_button['state'] = 'disabled'

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

    def apply_cvc(self):
        self.set_status('Working...')
        self.serial_port.write(b'set log bt off\r')
        sleep(0.25)
        self.state = STATE_APPLY_CVC
        self.apply_button['state'] = 'disabled'
        self.update()
        found_mac = False
        if self.mac_id >= 0x0907A7 and self.mac_id <= 0x0909F9:
            found_mac = True
            self.serial_port.write(b'bt license cvc 3D6C 51DF 4AD8 FC81 0000\r')
        if self.mac_id >= 0x0E8E6A and self.mac_id <= 0x0E9265:
            found_mac = True
            self.serial_port.write(b'bt license cvc 3A6B D812 D144 FC81 0000\r')
        if self.mac_id >= 0x0E9DE8 and self.mac_id <= 0x0EA1F7:
            found_mac = True
            self.serial_port.write(b'bt license cvc 3A6B CB90 E2D6 FC81 0000\r')
        if self.mac_id >= 0x0ED710 and self.mac_id <= 0x0ED8AE:
            found_mac = True
            self.serial_port.write(b'bt license cvc 3A6B 8168 9B8F FC81 0000\r')
        if self.mac_id >= 0x0EE480 and self.mac_id <= 0x0EE8AE:
            found_mac = True
            self.serial_port.write(b'bt license cvc 3A6B B2F8 AB8F FC81 0000\r')
        if self.mac_id >= 0x0EEA60 and self.mac_id <= 0x0EEA9E:
            found_mac = True
            self.serial_port.write(b'bt license cvc 3A6B BC18 A9BF FC81 0000\r')
        if not found_mac:
            if self.mac_id >= 0x0E8E6A and self.mac_id <= 0x0EEA9E:
                found_mac = True
                self.serial_port.write(b'bt license cvc 3A6B CB90 A9BF FC81 0000\r')
            else:
                self.set_status('ERROR: License for MAC ID %s not found' % (self.mac_id))
                self.state = STATE_NEW
            return
        sleep(0.25)
        self.serial_port.write(b'bt cvc on\r')
        sleep(0.25)
        self.serial_port.write(b'bt mgain c1\r')
        self.handle_serial()

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
        self.set_status('Connecting...')
        self.serial_port.write(b'\r') # Clear the port
        sleep(0.25)
        while self.serial_port.in_waiting:
            if self.serial_port.read():
                pass
        self.serial_port.write(b'version\r') # Get the version
        self.start_time = time()
        self.handle_serial()

    def handle_serial(self):
        should_continue = True
        while should_continue and self.state != STATE_DONE:
            while self.serial_port.in_waiting:
                data = self.serial_port.read()
                if not data:
                    continue
                for i in data:
                    self.rx_buffer.append(chr(i))
                if not '\n' in self.rx_buffer:
                    continue
                tbuf = []
                for i in range(0, len(self.rx_buffer)):
                    c = self.rx_buffer.pop(0)
                    if c == '\r':
                        continue
                    if c == '\n':
                        line = ''.join(tbuf)
                        if line:
                            tbuf = []
                            if 'Serial Number: ' in line:
                                sn = line.replace('Serial Number: ', '')
                                self.set_status('Serial Number: %s' % sn)
                                self.serial_port.write(b'set log bt on\r')
                                sleep(0.25)
                                self.serial_port.write(b'bt version\r')
                                self.start_time = time()
                                self.state = STATE_VERSION_FOUND
                                should_continue = False
                            if 'Bluetooth addresses: ' in line:
                                self.start_time = time()
                                bt_data = line.split(' ')
                                mac_id = None
                                for i in bt_data:
                                    if '20FABB0' in i:
                                        mac_id = i.replace('20FABB0', '')
                                self.mac_id = int('0x%s' % mac_id, 16)
                                self.apply_button['state'] = 'normal'
                                should_continue = False
                                self.state = STATE_VERSION_BT
                            if 'log bt off' in line:
                                self.state = STATE_LOG_OFF
                            if 'bt license cvc' in line:
                                self.state = STATE_APPLY_CVC
                            if 'bt cvc on' in line:
                                self.state = STATE_CVC_ON
                            if 'bt mgain c1' in line:
                                self.state = STATE_DONE
                            if 'Command not found' in line:
                                self.apply_button['state'] = 'normal'
                                self.serial_port.write(b'\r')
                                self.set_status('Error, please try again')
                                return
                            return self.handle_serial()
                            line = None
                    else:
                        tbuf.append(c)
            try:
                self.update()
            except TclError:
                sys.exit(0)
        if (time() - self.start_time) >= self.TIMEOUT and self.state != STATE_VERSION_BT:
            self.state = STATE_NEW
            self.set_status('Timed out while waiting, please try again')
            return
        if self.state == STATE_DONE:
            self.set_status('Successfully Applied License!')
            return

    def set_status(self, text):
        self.statusbar.config(text=text)
        self.statusbar.update()

if __name__ == '__main__':
    app = Application()
