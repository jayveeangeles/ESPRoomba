# ESPRoomba
Inspired by [cybrox's implementation](https://github.com/cybrox/wroomba) using ESP32 as a Roomba controller. Instead of using ESP32 as a dedicated webserver, the ESP32 is made into a CoAP server that any CoAP client can subscribe to (one possibility is to use a nodeJS server as a [CoAP client](https://github.com/mcollina/node-coap)). 

## Hardware
1. [ESP32 DevKitC](https://dl.espressif.com/doc/esp-idf/latest/get-started/get-started-devkitc.html)
2. [Switching (step-down) Regulator](https://www.pololu.com/product/2858)
3. [Bi-Directional Level Converterr](https://www.sparkfun.com/products/12009)
4. Terminal blocks
5. Switch

## Circuit Connections
- The Pololu step-down regulator has 5 pins. For this exercise, only 3 pins were used. IN is connected to the unregulated battery of the robot, GND is connected to the GND of the robot and OUT is connected to DevKitC's 5V pin.
- Instead of using a 3.3V step-down regulator, a 5V version is used instead. This is to allow the level converter to use the same 5V input to the DevKitC as reference voltage (high side). 
- A simple voltage divider can also be used to bring down the UART RX to less than 5V (ESP32 is not 5V tolerant) but a level converter is used instead.
- IO16 and IO17 are used as UART RX and TX respectively. IO18 is used to pulse the BRC pin low (not fully tested if working).
- IO16 is connected to Roomba TXD pin (through level converter) and IO17 to Roomba RXD.
- The 5V and 3.3V pins are used as reference voltages for the high and low sides of the level converter.

## CoAP Endpoints
1. /sc -> for simple commands
   * /sc?mode=safe -> safe mode
   * /sc?mode=full -> full mode
   * /sc?clean=clean -> send clean opcode
   * /sc?clean=max -> send max opcode
   * /sc?clean=dock -> send seek and dock opcode
   * /sc?clean=spot -> send spot opcode
2. /sked -> send schedule to Roomba
   * use payload option here in JSON format; days are three letter words (mon, tue, wed, thu, fri, sat, sun), hour under hr key and minute in mn key
   ``` 
   {
      mon: {
        hr: 12,
        mn: 45
      },
      fri: {
        hr: 13,
        mn: 00
      }
    }
    ```
3. /obs -> subscribe to Roomba sensors, data is sent every 15ms.

## To Do's
* Make commands handling simpler
* Add actuator handlers
* Add simple circuit diagram for connections
* Implement multicast for CoAP (?)
