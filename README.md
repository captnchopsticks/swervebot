# swervebot
Repository for my Swervebot project (1.12.2024)
Hello! This is an example code to get you started with the swervebot project. The code lets you control 
the swervebot through any device's web browser. The Nano ESP32 hosts a web server with the web page that has
the buttons and sliders to control the robot. Anytime you press any of the buttons, an HTTP request is
sent to the Nano ESP32. The Nano ESP32 will then drive the motors according to your desired inputs.
To make this code functional, you will need to replace your ssid and password information so the Nano ESP32
can connect to your WiFi or hotspot.
This code uses the SH110X library from Adafruit to control the OLED screen. However, it is not the end of the world
if you are using the SSD1306 OLED screen, which is much more commonly used. To adapt the code, you will need to include
the appropriate Adafruit library as well as change the initialization of the Adafruit display object to accomodate
your OLED screen.
