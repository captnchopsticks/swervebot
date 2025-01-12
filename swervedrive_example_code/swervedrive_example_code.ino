#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SparkFun_TB6612.h>

// Replace with your network credentials!!!!!!
const char* ssid = "ssid";
const char* password = "password";

//=========assigning pins
const int A1in1 = 2;
const int A1in2 = 3;
const int B1in1 = 4;
const int B1in2 = 5;
const int PWMA1 = 6;
const int PWMB1 = 7;
const int STBY1 = A7;

const int A2in1 = 10;
const int A2in2 = 11;
const int B2in1 = 12;
const int B2in2 = A3;
const int PWMA2 = 8;
const int PWMB2 = 9;
const int STBY2 = A6;

//these parameters are inherited from the Sparkfun Library.
const int OffsetA = 1;
const int OffsetB = 1;

Motor motorA1 = Motor(A1in1, A1in2, PWMA1, OffsetA, STBY1);//setting up motor "profiles"
Motor motorB1 = Motor(B1in1, B1in2, PWMB1, OffsetB, STBY1);
Motor motorA2 = Motor(A2in1, A2in2, PWMA2, OffsetA, STBY2);
Motor motorB2 = Motor(B2in1, B2in2, PWMB2, OffsetB, STBY2);

// variables for screen
#define i2c_Address 0x3c  //initialize with the I2C addr 0x3C Typically eBay OLED's
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET -1
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET, 400000, 100000);
//variables for eye animation from intellar.
int ref_eye_height = 40;
int ref_eye_width = 40;
int ref_space_between_eye = 10;
int ref_corner_radius = 10;
int left_eye_height = ref_eye_height;
int left_eye_width = ref_eye_width;
int left_eye_x = 32;
int left_eye_y = 32;
int right_eye_x = 32 + ref_eye_width + ref_space_between_eye;
int right_eye_y = 32;
int right_eye_height = ref_eye_height;
int right_eye_width = ref_eye_width;
static const int max_animation_index = 7;
int current_animation_index = 0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// variables for parsing and storing commands from the HTML get requests.
const char* PARAM_INPUT = "value";
String direction;
String textInput = "0";  //blank string container for now
int integerFromPC = 0;
float floatFromPC = 0.0;

//a raw string literal that stores the html code for the async web server
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Simple Swervebot Controller</title>
<style>
  html{font-family: Arial; text-align:center; color:white;}
  body {background-color:#00cc66;}
  h1 {font-size:2.3rem;}
  h2 {font-size: 2.3rem;}
  p {font-size: 1.3 rem;}
  .container {display:flex; max-width:device-width;}
  .containertwo {display:in-block; background-color:#FFF4A3;}
  .columnone {background-color:#53c68c; width:70vw;}
  .columntwo {background-color:#39ac73; width:30vw;}
  .buttonstyle {
    font-size: 1.4rem;
    padding: 1rem 2rem;
    margin: 0.2rem;
    background-color:#00994d;
    color:white;
  }
  .vertical-slider {
    user-select: none; 
    -webkit-appearance: slider-vertical;
    appearance: slider-vertical;
    width: 20px;
    height: 200px;
  }
</style>
</head>
<body>
<h1>Simple Swervebot Controller</h1>
<p>This is an example web server that lets you send GET requests to the Nano ESP32.</p> 
<p>FW and BW moves the entire swervebot forward and backward respectively.</p>
<p>CCW and CW turns the orientation of the swerve modules, allowing you to strafe in any direction.</p>
<p>CCW orbit and CW orbit drives the swervebot in a orbital path. The orbit slider sets how tight the orbit is. Smaller setting = greater orbit radius.</p>
<div class="container">
  <div class="columnone">
    <h2>Direction</h2>
    <button class="buttonstyle" onmousedown="controller(1, 'forward')" onmouseup="controller(0, 'brake')" ontouchstart="controller(1, 'forward')">FW</button>
    <br><br>
    <button class="buttonstyle" onmousedown="controller(1, 'all')" onmouseup="controller(0, 'brake')" ontouchstart="controller(1, 'all')">CCW</button>
    <button class="buttonstyle" onmousedown="controller(-1, 'all')" onmouseup="controller(0, 'brake')" ontouchstart="controller(-1, 'all')">CW</button>
    <br><br>
    <button class="buttonstyle" onmousedown="controller(-1, 'forward')" onmouseup="controller(0, 'brake')" ontouchstart="controller(-1, 'forward')">BW</button>
    <br><br>
    <button class="buttonstyle" onmousedown="controller(-1, 'orbit')" onmouseup="controller(0, 'brake')" ontouchstart="controller(-1, 'orbit')">CCW ORBIT</button>
    <button class="buttonstyle" onmousedown="controller(1, 'orbit')" onmouseup="controller(0, 'brake')" ontouchstart="controller(1, 'orbit')">CW ORBIT</button>
  </div>
  <div class="columntwo">
    <h2>Speed</h2>
    <p><span id="vsliderplaceholder">%VERTSLIDEPLACEHOLDER%</span></p>
    <input type="range" class="vertical-slider" min="0" max="255" value="0" orient="vertical" id="Vslider" onchange = "controller(0, 'brake')">
    <h2>Orbit</h2>
    <p><span id="hsliderplaceholder">%HORIZSLIDEPLACEHOLDER%</span></p>
    <input type="range" min="0" max="1" id="Hslider" onchange = "controller(0, 'brake')" step="0.05">
    <br><br>
  </div>
</div>
<script>
function controller(offset, direction) {
  if (offset != 0) {
    var speed = document.getElementById("Vslider").value * offset;
    var orbit = document.getElementById("Hslider").value;
    document.getElementById("vsliderplaceholder").innerHTML = direction + "," + speed;
    console.log(speed);
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/controller?value=" + direction + "," + speed + "," + orbit, true);
    xhr.send();
  }
  else {
    document.getElementById("vsliderplaceholder").innerHTML = "brake, slider position: " + document.getElementById("Vslider").value;
    document.getElementById("hsliderplaceholder").innerHTML = document.getElementById("Hslider").value
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/controller?value=" + direction + "," + offset + ",0" , true);
    xhr.send();
  }
}
</script>
</body>
</html>
)rawliteral";

String processor(const String& var) {
  //Serial.println(var);
  if (var == "ESPRESPONSE") {
    return textInput;
  }
  return String();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // print ESP Local IP Address. sometimes this doesn't show up.
  // in that case, copy the following line into void loop to record the IP address.
  Serial.println(WiFi.localIP());

  // route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  /* this section of code is for the ESP32 to handle GET requests from the clients
  (i.e. your computer/phone/device's browser), parses the string and stores the info
  into the textInput, integerFromPC, and floatFromPC variables in the parseRequest function,
  and drives the motors accordingly in the interpretRequest function.
  */
  server.on("/controller", HTTP_GET, [](AsyncWebServerRequest* request) {
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      textInput = request->getParam(PARAM_INPUT)->value();
    }
    else {
      textInput = "No message sent";
    }
    Serial.println(textInput);
    parseRequest();
    interpretRequest();
    request->send(200, "text/plain", "OK");
  });

  // Start server
  server.begin();

  //code to initialize the eyes
  display.begin(i2c_Address, true);
  display.clearDisplay();
  display.display();
  delay(500);
  sleep();
  delay(500);
  wakeup();
  delay(1000);
}

void loop() {
  // put your main code here, to run repeatedly:

  launch_animation_with_index(current_animation_index++);
  if (current_animation_index > max_animation_index) {
    current_animation_index = 0;
  }
}

void parseRequest() {
  int firstComma = textInput.indexOf(',');
  int secondComma = textInput.indexOf(',', firstComma + 1);
  // Extract the message, integer, and float parts from `textInput`
  integerFromPC = textInput.substring(firstComma + 1, secondComma).toInt();
  floatFromPC = textInput.substring(secondComma + 1).toFloat();
}

void interpretRequest() {
  char command = textInput[0];
  switch (command) {
    case 'f':
      direction = "Moving forward";
      forward(motorA1, motorA2, integerFromPC);
      break;
    case 'a':
      direction = "All motors moving";
      forward(motorA1, motorA2, integerFromPC);
      forward(motorB1, motorB2, integerFromPC);
      break;
    case 'b':
      direction = "Braking";
      brake(motorA1, motorA2);
      brake(motorB1, motorB2);
      delay(50);
      break;
    case 'o':
      direction = "Orbiting";
      forward(motorA1, motorA2, integerFromPC);
      forward(motorB1, motorB2, (integerFromPC * floatFromPC));
      break;
    default:
      direction = "Unknown input";
      break;
  }
  if (command == 'o') {
    Serial.println(direction + " at speed of " + String(integerFromPC) + " with an orbit scale of " + String(floatFromPC));
  }
  else {
    Serial.println(direction + " at speed of " + String(integerFromPC) + ".");
  }
}

//the credit for the rest of the code goes to intellar.ca. much thanks to him!
void draw_eyes(bool update = true) {
  display.clearDisplay();
  //draw from center
  int x = int(left_eye_x - left_eye_width / 2);
  int y = int(left_eye_y - left_eye_height / 2);
  display.fillRoundRect(x, y, left_eye_width, left_eye_height, ref_corner_radius, SH110X_WHITE);
  x = int(right_eye_x - right_eye_width / 2);
  y = int(right_eye_y - right_eye_height / 2);
  display.fillRoundRect(x, y, right_eye_width, right_eye_height, ref_corner_radius, SH110X_WHITE);
  if (update) {
    display.display();
  }
}
void center_eyes(bool update = true) {
  //move eyes to the center of the display, defined by SCREEN_WIDTH, SCREEN_HEIGHT
  left_eye_height = ref_eye_height;
  left_eye_width = ref_eye_width;
  right_eye_height = ref_eye_height;
  right_eye_width = ref_eye_width;

  left_eye_x = SCREEN_WIDTH / 2 - ref_eye_width / 2 - ref_space_between_eye / 2;
  left_eye_y = SCREEN_HEIGHT / 2;
  right_eye_x = SCREEN_WIDTH / 2 + ref_eye_width / 2 + ref_space_between_eye / 2;
  right_eye_y = SCREEN_HEIGHT / 2;

  draw_eyes(update);
}
void blink(int speed = 12) {
  draw_eyes();

  for (int i = 0; i < 3; i++) {
    left_eye_height = left_eye_height - speed;
    right_eye_height = right_eye_height - speed;
    draw_eyes();
    delay(1);
  }
  for (int i = 0; i < 3; i++) {
    left_eye_height = left_eye_height + speed;
    right_eye_height = right_eye_height + speed;

    draw_eyes();
    delay(1);
  }
}
void sleep() {
  left_eye_height = 2;
  right_eye_height = 2;
  draw_eyes(true);
}
void wakeup() {
  sleep();
  for (int h = 0; h <= ref_eye_height; h += 2) {
    left_eye_height = h;
    right_eye_height = h;
    draw_eyes(true);
  }
}
void happy_eye() {
  center_eyes(false);
  //draw inverted triangle over eye lower part
  int offset = ref_eye_height / 2;
  for (int i = 0; i < 10; i++) {
    display.fillTriangle(left_eye_x - left_eye_width / 2 - 1, left_eye_y + offset, left_eye_x + left_eye_width / 2 + 1, left_eye_y + 5 + offset, left_eye_x - left_eye_width / 2 - 1, left_eye_y + left_eye_height + offset, SH110X_BLACK);
    //display.fillRect(left_eye_x-left_eye_width/2-1, left_eye_y+5, left_eye_width+1, 20,SH110X_BLACK);

    display.fillTriangle(right_eye_x + right_eye_width / 2 + 1, right_eye_y + offset, right_eye_x - left_eye_width / 2 - 1, right_eye_y + 5 + offset, right_eye_x + right_eye_width / 2 + 1, right_eye_y + right_eye_height + offset, SH110X_BLACK);
    //display.fillRect(right_eye_x-right_eye_width/2-1, right_eye_y+5, right_eye_width+1, 20,SH110X_BLACK);
    offset -= 2;
    display.display();
    delay(1);
  }
  display.display();
  delay(1000);
}
void move_right_big_eye() {
  move_big_eye(1);
}
void move_left_big_eye() {
  move_big_eye(-1);
}
void move_big_eye(int direction) {
  //direction == -1 :  move left
  //direction == 1 :  move right

  int direction_oversize = 1;
  int direction_movement_amplitude = 2;
  int blink_amplitude = 5;

  for (int i = 0; i < 3; i++) {
    left_eye_x += direction_movement_amplitude * direction;
    right_eye_x += direction_movement_amplitude * direction;
    right_eye_height -= blink_amplitude;
    left_eye_height -= blink_amplitude;
    if (direction > 0) {
      right_eye_height += direction_oversize;
      right_eye_width += direction_oversize;
    } else {
      left_eye_height += direction_oversize;
      left_eye_width += direction_oversize;
    }

    draw_eyes();
    delay(1);
  }
  for (int i = 0; i < 3; i++) {
    left_eye_x += direction_movement_amplitude * direction;
    right_eye_x += direction_movement_amplitude * direction;
    right_eye_height += blink_amplitude;
    left_eye_height += blink_amplitude;
    if (direction > 0) {
      right_eye_height += direction_oversize;
      right_eye_width += direction_oversize;
    } else {
      left_eye_height += direction_oversize;
      left_eye_width += direction_oversize;
    }
    draw_eyes();
    delay(1);
  }

  delay(1000);

  for (int i = 0; i < 3; i++) {
    left_eye_x -= direction_movement_amplitude * direction;
    right_eye_x -= direction_movement_amplitude * direction;
    right_eye_height -= blink_amplitude;
    left_eye_height -= blink_amplitude;
    if (direction > 0) {
      right_eye_height -= direction_oversize;
      right_eye_width -= direction_oversize;
    } else {
      left_eye_height -= direction_oversize;
      left_eye_width -= direction_oversize;
    }
    draw_eyes();
    delay(1);
  }
  for (int i = 0; i < 3; i++) {
    left_eye_x -= direction_movement_amplitude * direction;
    right_eye_x -= direction_movement_amplitude * direction;
    right_eye_height += blink_amplitude;
    left_eye_height += blink_amplitude;
    if (direction > 0) {
      right_eye_height -= direction_oversize;
      right_eye_width -= direction_oversize;
    } else {
      left_eye_height -= direction_oversize;
      left_eye_width -= direction_oversize;
    }
    draw_eyes();
    delay(1);
  }

  center_eyes();
}

void launch_animation_with_index(int animation_index) {

  switch (animation_index) {
    case 0:
      blink(12);
      break;
    case 1:
      center_eyes(true);
      break;
    case 2:
      move_right_big_eye();
      break;
    case 3:
      move_left_big_eye();
      break;
    case 4:
      blink(10);
      break;
    case 5:
      blink(10);
      break;
    case 6:
      happy_eye();
      break;
    case 7:
      sleep();
      break;
  }
}
