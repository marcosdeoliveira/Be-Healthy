/*
*** Uploaded and tested at a NODEMCU v0.1 Board with ESP-12
*** At arduino software choose: NodeMCU 1.0(ESP-12E Module) at 80MHz, 115200, 4M (3M SPIFFS)
*** 192.168.4.1 --> IP pra SetUP
***
*/

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>            //Library for DNS Server on ESP
#include <ESP8266WebServer.h>     //Library for an ESP WebServer, generally built in!
/*
*** The WiFiManager Library, is the one that enables the ESP8266 to when on connect
*** to a previously known WiFi, or ask the user to connect the system to his wifi.
*/
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <WiFiClient.h>           //Makes the ESP Connect to a WiFi
#include <ESP8266mDNS.h>          //Creates an mDNS for the ESP on your local network
#include <ADXL345.h>

ESP8266WebServer server(80);      //Starts the Web Server on port 80 of the ESP8266
ADXL345 accelerometer;


//Strings that will receive the website code
String webpage = "";
String webpage_aux = "";
int refresh_delay = 1000; // 2000 ms => 2s
int sensor_standing_time=2;
int sensor_seated_time=1;
int fpitch=0;
int last_fpitch=0;
unsigned int long timer=0;
unsigned int long standing_time = 0;
unsigned int long seated_time = 0;
unsigned int long last_calc_time = 0;
unsigned int long standing_time_hs = 0;
unsigned int long standing_time_min = 0;
unsigned int long standing_time_sec = 0;
unsigned int long seated_time_hs = 0;
unsigned int long seated_time_min = 0;
unsigned int long seated_time_sec = 0;


#define countof(a) (sizeof(a) / sizeof(a[0]))

//In this void, the system deals with access to server's root
void handleRoot() {
  server.send(200, "text/html", webpage_aux); //Sending the HTML to the user with code 200
}

//This void handles not found links on the server
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message); // Sending an ERROR 404 and a message
}

/*
*** This fuction is called on every interrupt to increment the counter
*** On setup we describe the webPage and set connectivity
*/

void setup(void) {
/*
*** This is the HTML code for the webPage that displays the sensor data on the server
*/
webpage+="<!DOCTYPE html>";
webpage+="<html>";
webpage+="<style>";
webpage+="body{background-color:#9B26Af;width: 99%;height: 99%;}";
webpage+="h1{color:#607D8B; font-family:Arial, Helvetica, sans-serif; text-align:center; font-size:200%;}";
webpage+="h2{color:#78909C;font-family:Arial, Helvetica, sans-serif;text-align:center;font-size:100%;}";
webpage+="#headTitle{ color:#ECEFF1; font-family:Verdana; text-align:center; font-size:300%;}";
webpage+="#head {";
webpage+=" background-color: #691A99;";
webpage+=" max-width: 100%;";
webpage+=" margin-left:auto;";
webpage+=" margin-right:auto;";
webpage+=" padding-top: 1em;  ";
webpage+=" padding-bottom: 1em;";
webpage+="  }";
webpage+="";
webpage+="";
webpage+="#standing {";
webpage+=" background-color: #68EFAD;";
webpage+=" margin-top: 3%;";
webpage+=" border-radius: 2em;";
webpage+=" padding-top: 1%;";
webpage+=" padding-bottom: 1%;";
webpage+=" border-bottom-color: #7B1FA2;";
webpage+=" border-bottom-style: inset;";
webpage+=" border-bottom-width: 12px;";
webpage+=" border-left-color: #7B1FA2;";
webpage+=" border-left-style: solid;";
webpage+=" max-width: 60%;";
webpage+=" margin-left:auto;";
webpage+=" margin-right:auto;";
webpage+="}";
webpage+="";
webpage+="</style>";
webpage+=" <head>";
webpage+=" <title>Be Healthy | sedentary monitor</title> ";
webpage+=" </head>";
webpage+=" <body>";
webpage+=" <div>";
webpage+=" <div id = \"head\">";

  //Begins Serial and WiFiManager
  Serial.begin(115200);
    // Initialize ADXL345
  Serial.println("Initialize ADXL345");

  if (!accelerometer.begin())
  {
    Serial.println("Could not find a valid ADXL345 sensor, check wiring!");
    delay(500);
  }

  // Set measurement range
  // +/-  2G: ADXL345_RANGE_2G
  // +/-  4G: ADXL345_RANGE_4G
  // +/-  8G: ADXL345_RANGE_8G
  // +/- 16G: ADXL345_RANGE_16G
  accelerometer.setRange(ADXL345_RANGE_16G);

  WiFiManager wifiManager;  

  //sets timeout until configuration portal gets turned off
  wifiManager.setTimeout(180);

  //reset saved settings
  //wifiManager.resetSettings();

  if (!wifiManager.autoConnect("Be Healthy", "87654321")) {    //fetches ssid and pass from eeprom and tries to connect
    Serial.println("failed to connect and hit timeout");  //if it does not connect it starts an access point with the specified name
    delay(3000);                                          //here  "GetUp"
                                                          //and goes into a blocking loop awaiting configuration
    ESP.reset();                                          //reset and try again, or maybe put it to deep sleep
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //Prints the IP address at the serial connection for debug
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //Starts the mDNS server as GOUT
  if (MDNS.begin("GetUp")) {
    Serial.println("MDNS responder started");
  }

  //If someone connects on root, run handle root function
  server.on("/", handleRoot);

  //On NotFound files at the server, runs handleNotFound function
  server.onNotFound(handleNotFound);

  //Begins Server
  server.begin();
  Serial.println("HTTP server started");

}

/* 
*** At the loop updates the webpage with flow data everytime the user connects
*** and count flow every second.
*/

void loop() {

  // The trick used here was finishing the webpage at the loop with the sensor data.
  webpage_aux = webpage;
  webpage_aux+=" <h1 id=\"headTitle\">Be Healthy</h1>";
  webpage_aux+=" </div >";
  webpage_aux+="  <section id=\"standing\">";
  webpage_aux+="<meta http-equiv=\"refresh\" content=\"";
  webpage_aux+= refresh_delay/1000; //converts to seconds
  webpage_aux+= "\">";
  webpage_aux+="  <h1>Standing:</h1>";
  webpage_aux+="<h2>";
  webpage_aux+= standing_time_hs;
  webpage_aux+=":";
  webpage_aux+= standing_time_min ;
  webpage_aux+=":";
  webpage_aux+= standing_time_sec;
  webpage_aux+=" </h2>";
  webpage_aux+="  </section>";
  webpage_aux+="  <section id=\"standing\">";
  webpage_aux+="  <h1>Seated:</h1>";
  webpage_aux+= "<h2>";
  webpage_aux+= seated_time_hs;
  webpage_aux+= ":";
  webpage_aux+= seated_time_min;
  webpage_aux+= ":";
  webpage_aux+= seated_time_sec;
  webpage_aux+= " </h2>";
  webpage_aux+="  </section>";
  webpage_aux+="  </div>";
  webpage_aux+="  </body>";
  webpage_aux+="</html>";

  //Run refresh every 'refresh_delay' ms
  if(millis()-timer>1000){
    refresh();
    timer = millis();
  }

  //Send code 200 (OK) and the hole webPage
  server.send(200, "text/html", webpage_aux);

  //Handles the client connection
  server.handleClient();

}

void refresh(){
  sensor_refresh();
  calc_time();
}

void sensor_refresh(){
  Vector norm = accelerometer.readNormalize();
  Vector filtered = accelerometer.lowPassFilter(norm, 0.5);
  // Calculate Pitch & Roll (Low Pass Filter)
  fpitch = -(atan2(filtered.XAxis, sqrt(filtered.YAxis*filtered.YAxis + filtered.ZAxis*filtered.ZAxis))*180.0)/M_PI;
}

void calc_time(){
  if (abs(fpitch)>50){
      standing_time+= millis() - last_calc_time;
  }else{
    seated_time+= millis() - last_calc_time;
  }
  last_calc_time = millis();
  
  standing_time_sec = msToSeconds(standing_time);
  if(standing_time_sec>59){
    standing_time=0;
    standing_time_sec=0;
    standing_time_min++;
  }
  if(standing_time_min>59){
    standing_time_min=0;
    standing_time_hs++;
  }
  (standing_time_hs>11)?standing_time_hs=0:standing_time_hs=standing_time_hs;

seated_time_sec = msToSeconds(seated_time);
  if(seated_time_sec>59){
    seated_time=0;
    seated_time_sec=0;
    seated_time_min++;
  }
  if(seated_time_min>59){
    seated_time_min=0;
    seated_time_hs++;
  }
  (seated_time_hs>11)?seated_time_hs=0:seated_time_hs=seated_time_hs;

}

int msToSeconds(int msInput){
  if(msInput>1000){
    return(msInput/1000);
  }else{
    return(0);
  }
}
