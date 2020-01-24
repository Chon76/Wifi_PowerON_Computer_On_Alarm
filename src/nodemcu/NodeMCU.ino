// § Midnight crisis: when the webserver gives 0:0 and the current time is 0:0

#include <ESP8266WebServer.h> // AP, webserver
#include <ESP8266WiFi.h> // connect to internet
#include <NTPClient.h> // get time from NTP server
#include <WiFiUdp.h> //  get time from NTP server
#include <Wire.h> // i2c communication


//==============================================================
//                  Web Server
//==============================================================
// Css
String main_css = "* {\n    font-family: 'Courier New', Courier, monospace;\n    text-align: center;\n}\n\n .child-one {\n    margin: auto;\n    display: inline-block;\n }\n\n .button {\n    background-color: #4CAF50;\n    border: none;\n    border-radius: 6px;\n    color: white;\n    padding: 16px 42px;\n    text-align: center;\n    text-decoration: none;\n    display: inline-block;\n    font-size: 16px;\n    margin: 4px 2px;\n    cursor: pointer;\n    width: 140px;\n  }";

String powerOnTime = "";
byte powerOnTime_hours = -1; // = 255
byte powerOnTime_minutes = -1; // = 255

// SSID and Password of your WiFi router
const char* ssid_AP = "ESP_01";
const char* password_AP = "12345678";
// AP Config IP
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// SSID and Password of Wifi to connect
const char* ssid_STA     = "netedismilyen";
const char* password_STA = "memedestroy";

ESP8266WebServer server(80); //Server on port 80

//==============================================================
//                  Current Time
//==============================================================
const int utcOffsetInSeconds = 3600;
byte currentTime_hours;
byte currentTime_minutes;
unsigned long timers[2];
// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

#define PIN D7

//==============================================================
//                  Declare Functions
//==============================================================
// Web server
void connectToInternet();
void createAP();
void handleRoot();
void handleCss();
void handleForm();
String SendHTML(String);

//Current Time
void getCurrentTime();
bool compareTime();

void debug();

//==============================================================
//                  SETUP
//==============================================================
void setup(void) {
  // Preparation
  Serial.begin(115200);
  Serial.println("");


  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN, LOW);

  Wire.begin(D1, D2); // I am the master!

  WiFi.mode(WIFI_AP_STA); // Set wifi mode to access point and station


  connectToInternet();
  createAP();

  // Display webpage
  server.on("/", handleRoot);      //Which routine to handle at root location
  server.on ("/main.css", handleCss);
  server.on("/action_page", handleForm);

  server.begin();                  //Start server

  Serial.println("");
  Serial.println("HTTP server started");

  timeClient.begin();

}
//==============================================================
//                     LOOP
//==============================================================
void loop(void) {
  server.handleClient();          //Handle client requests

  getCurrentTime();

  sendSignal();
  debug();
}

//==============================================================
//                    Web Server
//==============================================================

// Config Station
void connectToInternet() {
  WiFi.begin(ssid_STA, password_STA);
  Serial.print("Connecting");
  while ( WiFi.status() != WL_CONNECTED ) {
    digitalWrite(LED_BUILTIN, HIGH); // ==> LED:OFF
    delay(250);
    digitalWrite(LED_BUILTIN, LOW); // ==> LED:ON
    delay (250);
    Serial.print(".");
  }
  digitalWrite(LED_BUILTIN, HIGH); // ==> LED:OFF
}

// Config AP
void createAP() {
  WiFi.softAP(ssid_AP, password_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
}

void handleRoot() {
  server.send(200, "text/html", SendHTML(powerOnTime)); //Send web page
}

void handleCss() {
  server.send(200, "text/css", main_css);
}

void handleForm() {
  powerOnTime = server.arg("powerOnTime");

  powerOnTime_hours = powerOnTime.substring(0, powerOnTime.indexOf(':')).toInt();
  powerOnTime_minutes = powerOnTime.substring(powerOnTime.indexOf(':') + 1).toInt();

  // After deleting alarm to protect midnight crisis
  if (powerOnTime_hours == 0 && powerOnTime_minutes == 0) {
    powerOnTime_hours = -1; // = 255
    powerOnTime_minutes = -1; // = 255
  }

  server.send(200, "text/html", SendHTML(powerOnTime)); //Send web page
}

String SendHTML(String powerOnTime = "") {
  String ptr = "<!DOCTYPE html>\n";
  ptr += "<html lang=\"en\">\n";

  ptr += "<head>\n";
  ptr += "    <meta charset=\"UTF-8\">\n";
  ptr += "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
  ptr += "    <meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\">\n";
  ptr += "    <link rel=\"stylesheet\" type=\"text/css\" href=\"main.css\">\n";
  ptr += "    <title>Arduino Wake Up Comuputer</title>\n";
  ptr += "</head>\n";

  ptr += "<body>\n";
  ptr += "    <h1>Set Alarm</h1>\n";
  ptr += "    <div class=\"child-one\">\n";
  ptr += "        <!--<form action=\"/action_page\">-->\n";
  ptr += "        <input type=\"time\" id=\"wakeUpTime\" width=\"72px\" onchange=\"replaceButton()\">\n";
  ptr += "        <input type=\"submit\" class=\"button\" value=\"Clear\" id=\"submitButton\">\n";
  ptr += "        <!--</form>-->\n";
  ptr += "    </div>\n";


  if (powerOnTime != "") {
    ptr += "<br>Alarm is set to " + powerOnTime;
  } else {
    ptr += "<br>You have no alarm set.";
  }

  ptr += "</body>\n";

  ptr += "</html>\n";

  ptr += "<script>\n";
         ptr += "    function replaceButton() {\n";
  ptr += "        var time = document.getElementById(\"wakeUpTime\").value;\n";
  ptr += "        if (time == \"\") {\n";
         ptr += "            document.getElementById(\"submitButton\").value = \"Clear\";\n";
  ptr += "        } else {\n";
  ptr += "            document.getElementById(\"submitButton\").value = \"Set\";\n";
  ptr += "        }\n";
  ptr += "    }\n";
  ptr += "</script>";
  return ptr;
}

//==============================================================
//                     getCurrentTime
//==============================================================
// Check internet connection and calculate time considering the connection
void getCurrentTime() {
  if  (millis() - timers[0] >= 1000) {
    timers[0] = millis();
    if (WiFi.status() == WL_CONNECTED) {
      timeClient.update();
      currentTime_hours = timeClient.getHours();
      currentTime_minutes = timeClient.getMinutes();
    } else {
      if  (millis() - timers[1] >= 1000 * 60) {
        timers[1] = millis();
        currentTime_minutes++;
        if (currentTime_minutes > 60) {
          currentTime_minutes = 0;
          currentTime_hours++;
        }
        if (currentTime_hours > 24) {
          currentTime_hours = 0;
        }
      }
    }
  }
}
// Compare the curent and powerON time
bool compareTime() {
  if (powerOnTime_hours == currentTime_hours && powerOnTime_minutes == currentTime_minutes /*+ 2*/) {
    return true;
  }
  return false;
}


//==============================================================
//                     i2c
//==============================================================



//TODO: finish me
void sendSignal() {
  Wire.beginTransmission(8); /* begin with device address 8 */
  if (compareTime()) {
    Wire.write("1");
  } else {
    Wire.write("0");
  }
  Wire.endTransmission();    /* stop transmitting */
  delay(1000);
}

void debug() {
  Serial.println("======");
  Serial.print("Current time: ");
  Serial.print(currentTime_hours);
  Serial.print(":");
  Serial.println(currentTime_minutes);

  Serial.print("Alarm: ");
  Serial.print(powerOnTime_hours);
  Serial.print(":");
  Serial.println(powerOnTime_minutes);

  Serial.print("WIFI state: ");
  Serial.println(WiFi.status());
  Serial.print("Number of connected clients: ");
  Serial.println(WiFi.softAPgetStationNum());
  Serial.println("======");
  if (compareTime()) {
    Serial.println("ALARM IS ON!! RING-RING-RING!\nD7:ON");
  }
}
