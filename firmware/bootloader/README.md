# Bootloader

* Operates at 115200 baud with odd parity.

* Implements a protocol structured like so
    ```
    <Command Byte> <Length of entire message> <data (up to 252 bytes)> <XOR>
    ```
    * At least _one_ data byte is required, though it can be any value
