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
  recvSerialWithEndMarker();
  server.send(200, "text/html", makeHtml());
}

void handleJSON()
{
  server.send(200, "text/json", jsonPrettyString);
}

void handleTempUp()
{
  Serial.println("{\"ACTION\": 1}");
  recvSerialWithEndMarker();
  server.send(200, "text/html", makeHtml());
}
void handleTempDown()
{
  Serial.println("{\"ACTION\": 2}");
  recvSerialWithEndMarker();
  server.send(200, "text/html", makeHtml());
}

void handleLightsOn()
{
  Serial.println("{\"ACTION\": 3}");
  recvSerialWithEndMarker();
  server.send(200, "text/html", makeHtml());
}

void handleLightsOff()
{
  Serial.println("{\"ACTION\": 4}");
  recvSerialWithEndMarker();
  server.send(200, "text/html", makeHtml());
}

void handleAudioOn()
{
  Serial.println("{\"ACTION\": 5}");
  recvSerialWithEndMarker();
  server.send(200, "text/html", makeHtml());
}

void handleAudioOff()
{
  Serial.println("{\"ACTION\": 6}");
  recvSerialWithEndMarker();
  server.send(200, "text/html", makeHtml());
}

void handleTest() 
{
  server.send(200, "text/html", makeTest());
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

// {"temp1":"12","temp2":"22"}
//{"temp1":"12","temp2":"22","temp3":"33","array":[1.2,3.4],"p":[{"s":"1"},{"T":"2"}]}

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


String makeHtml() {
String html = "<html>";
html += "<p><a href=\"/\">Get Update</a></p>";
html += "<p><a href=\"/temp/up\">Temp Up</a></p>";
html += "<p><a href=\"/temp/down\">Temp Down</a></p>";
html += "<p><a href=\"/lights/on\">Lights On</a></p>";
html += "<p><a href=\"/lights/off\">Lights Off</a></p>";
html += "<p><a href=\"/audio/on\">Audio On</a></p>";
html += "<p><a href=\"/audio/off\">Audio Off</a></p>";
html += "<p><a href=\"/json\">View JSON</a></p>";
html += "<p><a href=\"/test\">Test HTML</a></p>";
html += "<p>Current Temp: ";
html += sb.averageCurrentTemp;
html += "</p><p>Desired Temp: ";
html += sb.desiredTemp;
html += "</p><p>Lights On: ";
html += sb.lightIsOn;
html += "</p><p>Audio On: ";
html += sb.audioIsOn;
html += "</p><p>Cooler On: ";
html += sb.coolerIsOn;
html += "</p><p>Global Error: ";
html += sb.globalError;
html += "</p></html>";

return html;
}
// {"DesiredTemp":15,"AverageCurrentTemp":22.2,"LightIsOn":0,"CoolerIsOn":0,"AudioIsOn":0,"GlobalError":"da error"}



String makeTest () {

String lightText = (sb.lightIsOn) ? "on" : "off";
String lightSwitch = (sb.lightIsOn) ? "off" : "on";
String lightButton = "<button type=\"button\" class=\"circ-button button-" + lightText + 
" button-sound-" + lightText + "\"><a href=\"/lights/" + lightSwitch + "\"></a></button>";

String audioText = (sb.audioIsOn) ? "on" : "off";
String coolerText = (sb.coolerIsOn) ? "on" : "off";


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
html += "h1{font-size:44px}";
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
html += " font-size: 13px;";
html += "}";
html += "";
html += ".temp-size {";
html += " font-size:28px;";
html += "}";
html += "";
html += "h1{";
html += " color:#E9F5ED!important;";
html += " font-weight:800; ";
html += " padding-left:8px;";
html += " padding-right:8px;";
html += " padding-top: 55px;";
html += "}";
html += "";
html += "h4{";
html += " font-size: 20px;";
html += " color: #E9F5ED;";
html += " padding-bottom: 10px;";
html += "}";
html += "";
html += ".temp-xlarge{";
html += " font-size:52px!important; ";
html += " color: #1D6E6C;";
html += " font-weight: 800;";
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
html += "  width: 66px;";
html += "  height: 56px;";
html += "  text-align: center;";
html += "  font-size: 52px;";
html += "  cursor: pointer;";
html += "  border-radius: 20%;";
html += "  text-decoration: none;";
html += "  text-align: center;";
html += "  margin: auto;";
html += "  line-height: 1.05;";
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
html += "  width: 88px;";
html += "  height: 88px;";
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
html += "  width: 96%;";
html += "  text-align: center;";
html += "}";
html += "";
html += ".temp-center {";
html += " padding: 0px 20px;";
html += "}";
html += "";
html += ".bgimg {";
html += "  background-repeat: no-repeat;";
html += "  background-size: cover;";
html += "  background-image: url(\"http://hadasashkenazi.design/box/header_bg.svg\");";
html += "  max-height: 400px;";
html += "  min-height: 175px;";
html += "}";
html += "";
html += ".set {";
html += " padding: 14px;";
html += "}";
html += "";
html += ".separate {";
html += " border-bottom: 2px solid; ";
html += " border-color: #E9F5ED;";
html += " max-width: 400px;";
html += " margin: auto;";
html += " padding-top: 20px;";
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
html += "         <button type=\"button\" class=\"button-temp\"><a href=\"/temp/down\">-</a></button>";
html += "         <span class=\"temp-center\"><span class=\"temp-xlarge\">";
html += sb.desiredTemp;
html += "°</span><span class=\"temp-size\">c</span></span>";
html += "         <button type=\"button\" class=\"button-temp\"><a href=\"/temp/up\">+</a></button>";
html += "         <div class=\"separate\"></div>";
html += "       </div>";
html += "       <div>";
html += lightButton;
// html += "          <button type=\"button\" class=\"circ-button button-";
// html += lightText;
// html += " button-light-";
// html += lightText; 
// html += "\"><a href=\"\"></a></button>";
html += "         <button type=\"button\" class=\"circ-button button-off button-sound-off\"><a href=\"\"></a></button>";
html += "         <span type=\"button\" class=\"circ-button button-off button-cooler-off\"></span>";
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
