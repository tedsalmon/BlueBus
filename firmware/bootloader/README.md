# Bootloader

* Operates at 111000 baud for UART connections to prevent timing issues. The clock error rate is too high at 115200 with the FT232RL

* Implements a protocol structured like so
    ```
    <Command Byte> <Length of entire message> <data (up to 252 bytes)> <XOR>
    ```
    * At least _one_ data byte is required, though it can be any value

* Firmware updates via the BC127 are not fully supported yet.
