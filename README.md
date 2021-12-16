# ESP32 TCP/IP Echo server

![DevKitC](./img/esp-devkits-banner3.jpg "DevKitC")

|Used board|ESP Module|Used platform|
|---|---|---|
|ESP32_DevKitc_V4|ESP32-WROOM-32D|Mongoose OS|

### Requirements

- Listen port : 11122
- Max. active connections : 5
- Message terminated with CR/LF (ASCII encoding, line based)
- Response with the requested message
- API 
  - Count active connections
  - Messages count for every active connection

# Setup development environment

### Install MOS tool
```
sudo add-apt-repository ppa:mongoose-os/mos
sudo apt-get update
sudo apt-get install mos
mos
```

### Build and flash the project
```
mos build --local --verbose
mos flash
```

