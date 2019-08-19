# reservoir-meter
Small project for Embedded Systems class. Allows you to monitor the water level of multiple reservoirs through a webapp.

Has a couple parts:

### Webapp
![Screenshot](readme-screenshot.png)
(todo: explanation here)

### Node MCU (ESP8266)
![Photo](readme-photo.png)
(todo: explanation here)

## Instructions
### Preparation
- Redis:
    - Set up a new redis instance.
    - Change relevant configurations in `db.js` so the app can connect to it.
- Node Webapp:
    - Install dependencies: `npm install`
    - Start dev server: `npm start`
    - The application should now be available at `localhost:3000`. Note the IP address of the server machine.
- ESP8266:
    - [Configure the Arduino IDE to support the ESP8266 board](https://github.com/esp8266/Arduino).
    - Install the [ESP8266 filesystem uploader Arduino plugin](https://github.com/esp8266/arduino-esp8266fs-plugin/).
    - Install the [library for the Ultrasonic sensor](https://github.com/filipeflop/Ultrasonic).
    - Load up the `esp8266` sketch in the Arduino IDE, change whatever you deem necessary (configuration WiFi AP name and password, input/output pin numbers...) in `esp8266.ino`.
    - Select `Tools -> ESP8266 Sketch Data Upload`. This will transfer the `index.html` in the `data` folder into the ESP8266.
    - Write the sketch to the board.

### Configuration
After all the steps above, the ESP8266 should boot and, after a few second of failing to connect to a WiFi AP, start its own. The default ssid name is `reservoirmeter` and the default password is `2907158928`. Connect to it, and then visit `http://192.168.4.1/` in your browser.

Input all the necessary values and click save. The browser should show there was an empty response. If everything went okay, reboot the ESP8266 and it should connect correctly to the WiFi AP specified in the configuration page.

### Caveats
The code for the configuration page is very primitive. As so, special characters in any of the fields won't work.
