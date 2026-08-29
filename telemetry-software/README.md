# Status LED
The red onboard LED indicates ESP32 is powered. 
Another onboard RGB LED indicates sensor status:
- A green led that's always on: both GPS and IMU booted and connection established over i2c. 
- A purple led that's always on: IMU failed to initialise
- A yellow led that's always on: GPS failed to initialise
A green, flashing LED on the GPS sensor itself indicates it has successully obtained a fix. 


# Detailed status logs, programming and logged data extraction
The software is written using ESP-IDF. It is recommended to install ESP-IDF. Either start from the [VSCode extension](https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/installation.html) or go to [ESP-IDF github](https://github.com/espressif/esp-idf) directly. 

## A few setup steps before building and compiling: 
- `source ${IDF_PATH}/export.sh`
- If the above doesn't work, do `source /your-path-to-esp-idf-root/export.sh`
- `set-target esp32s3`
- `idf.py menuconfig` -> select `Serial Flasher config` and set `Flash size` to 8 MB.


## Build, compile and monitor
for a quick check (nothing important already logged in flash and not retreived)
- `idf.py build flash monitor` auto detects your port and flashes the board. Attaching a new monitor (without `no-reset`) results in a reset of the chip. A bunch of status logs will be printed, it actually tells you which usb-serial port is being used, the status of SD card/flash FAT logging system, IMU and GPS. 


## Access logged data 
A new datalog file is opened and written to every time the board reboots/resets. It also moves to a new file once a logging session exceeds one hour. 
- `idf.py -p /your-usb-serial-port monitor --no-reset` to find out which usb serial port is in use by the esp32, run `ls /dev/tty.* /dev/cu.*` (macOS) or `[System.IO.Ports.SerialPort]::GetPortNames()` (Windows powershell)
- This command attaches a moniotr to the serial port but does NOT reset/reboot the chip. 
- You should then see a console. type `help` for the short list of commands available, including listing files logged and their sizes, stream file content, removing a file etc. 
- To extract data onto your local PC directory, press `control+]` to exit monitor. Then in terminal, run `python tools/pull_log.py --port /YOUR-USB-SERIAL-PORT --file DATALOG-TO-BE-EXTRACTED.txt --out ~/YOUR-DESTINATION-DIRECTORY/YOUR-FILENAME.txt`

- **EXTRACT DATA ONTO YOUR LOCAL PC DIRECTORY FREQUENTLY**. Unfortunately data logged in flash FAT system is not as safely protected from power brown-outs and loss-of-power. This is because if a flash sector erase is triggered and there is a power-off after the erase but before the sector could be written to, the data temporarily copied into RAM would be lost. This would result in invlaid data. It's quite unlikely happen for normal power resets/reboot/monitor but DO NOT FLASH new program if there is meaningful logs not yet extracted. 

- **CLEAN UP AFTER YOU"VE EXTRACTED EVERYTHING** 
A `idf.py erase-flash` would do. You need to reflash after that. 





