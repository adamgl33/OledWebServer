#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <U8x8lib.h>

// oled setup
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/15, /* data=*/4, /* reset=*/16);

// webserver setup
const char *ssid = "linksys";
const char *password = "beliadam";

// Globals
WebServer server(80);
String jsonPrettyString = "{}";
String jsonString = "{}";
const uint16_t MAX_JSON_SIZE = 1000;
char receivedChars[MAX_JSON_SIZE]; // an array to store the received data
const uint16_t DELAY = 600;

typedef struct
{
  int8_t desiredTemp;
  float averageCurrentTemp;
  bool lightIsOn;
  bool audioIsOn;
  bool coolerIsOn;
  const char *globalError;
} StatusBuffer; 


StatusBuffer sb = {17,20.0,true,true,false,""};

void handleRoot()
{
  Serial.println("{\"ACTION\": 0}");
  waitRecvAndSendResponse();
}

void handleJSON()
{
  server.send(200, "text/json", jsonPrettyString);
}

void handleTempUp()
{
  Serial.println("{\"ACTION\": 1}");
  waitRecvAndSendResponse();
}
void handleTempDown()
{
  Serial.println("{\"ACTION\": 2}");
  waitRecvAndSendResponse();
}

void handleLightsOn()
{
  Serial.println("{\"ACTION\": 3}");
  waitRecvAndSendResponse();
}

void handleLightsOff()
{
  Serial.println("{\"ACTION\": 4}");
  waitRecvAndSendResponse();
}

void handleAudioOn()
{
  Serial.println("{\"ACTION\": 5}");
  waitRecvAndSendResponse();
}

void handleAudioOff()
{
  Serial.println("{\"ACTION\": 6}");
  waitRecvAndSendResponse();
}

void waitRecvAndSendResponse() {
  delay(DELAY);
  recvSerialWithEndMarker();
  server.send(200, "text/html", makeHtml());
}

void handleTest() 
{
  server.send(200, "text/html", makeHtml());
}
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

String IpAddress2String(const IPAddress &ipAddress)
{
  return String(ipAddress[0]) + String(".") +
         String(ipAddress[1]) + String(".") +
         String(ipAddress[2]) + String(".") +
         String(ipAddress[3]);
}

char *String2CharArray(String input, int sized)
{
  char *buf = (char *)malloc(sized);
  char temp[sized];
  input.toCharArray(temp, sized);
  strcpy(buf, temp);
  return buf;
}

void DrawLine(int column, int row, String text)
{
  // pad
  text = text + "             ";
  //convert to array
  char *charArray = String2CharArray(text, 16);
  u8x8.drawString(column, row, charArray);
  free(charArray);
}

void setup(void)
{

  // start oled
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  DrawLine(0, 0, "Starting...");

  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  //print to oled
  String ipLine = "IP: " + IpAddress2String(WiFi.localIP());
  DrawLine(0, 0, "Server on:");
  DrawLine(0, 1, ipLine);
  DrawLine(0, 2, "Network: ");
  DrawLine(0, 3, ssid);

  if (MDNS.begin("esp32"))
  {
    //Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", jsonString);
  });

  server.on("/json", handleJSON);
  server.on("/temp/up", handleTempUp);
  server.on("/temp/down", handleTempDown);
  server.on("/lights/on", handleLightsOn);
  server.on("/lights/off", handleLightsOff);
  server.on("/audio/on", handleAudioOn);
  server.on("/audio/off", handleAudioOff);
  server.on("/test", handleTest);

  server.onNotFound(handleNotFound);

  server.begin();
}

void loop(void)
{
  server.handleClient();

  recvSerialWithEndMarker();
}

// serial handler

boolean newData = false;

void recvSerialWithEndMarker()
{
  static uint16_t ndx = 0;
  char endMarker = '\n';
  char rc;

  while (Serial.available() > 0 && newData == false)
  {
    rc = Serial.read();

    if (rc != endMarker)
    {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= MAX_JSON_SIZE)
      {
        ndx = MAX_JSON_SIZE - 1;
      }
    }
    else
    {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
      // save strings
      processJSON();
    }
  }
}

void processJSON()
{
  StaticJsonBuffer<MAX_JSON_SIZE> jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(receivedChars);
  if (!root.success())
  {
    // ERROR 
    newData = false;
    return;
  }

  jsonPrettyString = "";
  root.prettyPrintTo(jsonPrettyString);
  jsonString = receivedChars;

  sb.desiredTemp = root["DesiredTemp"];
  sb.averageCurrentTemp = root["AverageCurrentTemp"];
  sb.lightIsOn = root["LightIsOn"];
  sb.coolerIsOn = root["CoolerIsOn"];
  sb.audioIsOn = root["AudioIsOn"];
  sb.globalError = root["GlobalError"];

  newData = false;
}



String makeHtml () {

String lightText = (sb.lightIsOn) ? "on" : "off";
String lightSwitch = (sb.lightIsOn) ? "off" : "on";
String lightButton = "<a href=\"/lights/" + lightSwitch + "\"><button type=\"button\" class=\"circ-button button-" +
 lightText + " button-light-" + lightText + "\"></button></a>";

String audioText = (sb.audioIsOn) ? "on" : "off";
String audioSwitch = (sb.audioIsOn) ? "off" : "on";
String audioButton = "<a href=\"/audio/" + audioSwitch + "\"><button type=\"button\" class=\"circ-button button-" +
 audioText + " button-sound-" + audioText + "\"></button></a>";


String html = "<html>";
html += "  <title>The box</title>";
html += "   <meta charset=\"UTF-8\">";
html += "   <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
html += " <head>";
html += "";
html += "   <link href=\"https://fonts.googleapis.com/css?family=Montserrat\" rel=\"stylesheet\">";
html += " </head>";
html += " <style type=\"text/css\">";
html += "   html{box-sizing:border-box}*,*:before,*:after{box-sizing:inherit}";
html += "html{-ms-text-size-adjust:100%;-webkit-text-size-adjust:100%}body{margin:0}";
html += "html,body{font-family:Verdana,sans-serif;font-size:15px;line-height:1.5}";
html += "html{overflow-x:hidden}";
html += "body, html {height: 100%}";
html += "";
html += "h1{font-size:2.75em}";
html += "h1,h2,h3,h4,h5,h6,button,span,.label {";
html += " font-family: 'Montserrat', sans-serif;";
html += " color:#1D6E6C;";
html += " text-transform: uppercase;";
html += " font-weight:400;";
html += " margin:10px 0;";
html += " text-align:center";
html += "}";
html += "";
html += ".lable {";
html += " font-size: 0.8em;";
html += "}";
html += "";
html += ".temp-size {";
html += " font-size:1.75em;";
html += "}";
html += "";
html += "h1{";
html += " color:#E9F5ED!important;";
html += " font-weight:700; ";
html += " padding-left:0.5em;";
html += " padding-right:0.5em;";
html += " padding-top: 1em;";
html += "}";
html += "";
html += "h4{";
html += " font-size: 1.25em;";
html += " color: #E9F5ED;";
html += " padding-bottom: 10px;";
html += "}";
html += "";
html += ".temp-xlarge{";
html += " font-size:3.25em!important; ";
html += " color: #1D6E6C;";
html += " font-weight: 700;";
html += "}";
html += "";
html += "article,aside,details,figcaption,figure,footer,header,main,menu,nav,section,summary{display:block}";
html += "";
html += "a{";
html += " background-color:transparent;";
html += " text-decoration: none;";
html += " -webkit-text-decoration-skip:objects;";
html += " color: inherit;";
html += "}";
html += "";
html += "a:active,a:hover{outline-width:0}abbr[title]{border-bottom:none;text-decoration:underline;text-decoration:underline dotted}";
html += "";
html += "";
html += ".button-temp {";
html += "  color:#35B993;";
html += "  background-color:#E9F5ED;";
html += "  border: none;";
html += "  padding: 0em 0.5em;";
html += "  text-align: center;";
html += "  font-size: 3.25em;";
html += "  cursor: pointer;";
html += "  border-radius: 10%;";
html += "  text-decoration: none;";
html += "  text-align: center;";
html += "  margin: auto;";
html += "  line-height: 1.1em;";
html += "}";
html += "";
html += ".button-temp:hover {";
html += "  background-color: #DEEFE4;";
html += "}";
html += "";
html += ".circ-button";
html += "{";
html += "  margin: 28px 8px 0px 8px;";
html += "  border: none;";
html += "  text-align: center;";
html += "  text-decoration: none;";
html += "  display: inline-block;";
html += "  border-radius: 50%;";
html += "  cursor: pointer;";
html += "  width: 8em;";
html += "  height: 8em;";
html += "";
html += "}";
html += ".button-off {";
html += "  background-color: #E9F5ED;";
html += "  background-position:center;";
html += "  background-repeat: no-repeat;";
html += "  background-size: 70%;";
html += "}";
html += "";
html += ".button-on {";
html += "  background-color: #77DBBE;";
html += "  background-position:center;";
html += "  background-repeat: no-repeat;";
html += "  background-size: 70%;";
html += "}";
html += "";
html += ".button-light-off {";
html += "background-image: url(\"http://hadasashkenazi.design/box/ic_light_off.svg\");";
html += "}";
html += "";
html += ".button-light-on {";
html += "background-image: url(\"http://hadasashkenazi.design/box/ic_light_on.svg\");";
html += "}";
html += "";
html += ".button-sound-off {";
html += "background-image: url(\"http://hadasashkenazi.design/box/ic_sound_off.svg\");";
html += "}";
html += "";
html += ".button-sound-on {";
html += "background-image: url(\"http://hadasashkenazi.design/box/ic_sound_on.svg\");";
html += "}";
html += "";
html += ".button-cooler-off {";
html += "background-image: url(\"http://hadasashkenazi.design/box/ic_cooler_off.svg\");";
html += "pointer-events: auto! important;";
html += "cursor: default! important;";
html += "}";
html += "";
html += ".button-cooler-on {";
html += "background-image: url(\"http://hadasashkenazi.design/box/ic_cooler_on.svg\");";
html += "}";
html += "";
html += ".center {";
html += "  margin: auto;";
html += "  text-align: center;";
html += "}";
html += "";
html += ".temp-center {";
html += " padding: 0em 1.25em;";
html += "}";
html += "";
html += ".bgimg {";
html += "  background-repeat: no-repeat;";
html += "  background-size: cover;";
html += "  background-image: url(\"http://hadasashkenazi.design/box/header_bg.svg\");";
html += "  max-height: 25em;";
html += "  min-height: 11em;";
html += "}";
html += "";
html += ".set {";
html += " padding: 0.875em;";
html += "}";
html += "";
html += ".separate {";
html += " border-bottom: 0.125em solid; ";
html += " border-color: #E9F5ED;";
html += " max-width: 400px;";
html += " margin: auto;";
html += " padding-top: 1.25em;";
html += "}";
html += "";
html += " </style>";
html += " <body>";
html += "   <div>";
html += "     <header class=\"bgimg\" w3-display-container>";
html += "       <h1>The box</h1>";
html += "       <h4>";
html += sb.averageCurrentTemp;
html += "°c</h4>";
html += "     </header>";
html += "     <div class=\"center\">";
html += "       <div class=\"label set\">Set to</div>";
html += "       <div class=\"center\">";
html += "         <a href=\"/temp/down\" class=\"button-temp\">-</a>";
html += "         <span class=\"temp-center\"><span class=\"temp-xlarge\">";
html += sb.desiredTemp;
html += "°</span><span class=\"temp-size\">c</span></span>";
html += "         <a href=\"/temp/up\" class=\"button-temp\">+</a>";
html += "         <div class=\"separate\"></div>";
html += "       </div>";
html += "       <div>";
html += lightButton;
html += audioButton;
String coolerText = (sb.coolerIsOn) ? "on" : "off";
html += "         <button type=\"button\" class=\"circ-button button-" + coolerText + " button-cooler-" + coolerText + "\"></button>";
html += "         <div class=\"separate\"></div>";
html += "       </div>";
html += "       <div class=\"label set\">";
html += "         <a href=\"/json\">Debugging</a>";
html += "       </div>";
html += "     </div>";
html += "   </div>";
html += " </body>";
html += "</html>";


return html;
}
// cat html  | sed 's/"/\\"/g' | sed 's/^/html += "/' | sed 's/$/";/'
