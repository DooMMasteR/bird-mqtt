
/*
  ESP8266 mDNS-SD responder and query sample

  This is an example of announcing and finding services.

  Instructions:
  - Update WiFi SSID and password as necessary.
  - Flash the sketch to two ESP8266 boards
  - The last one powered on should now find the other.
*/

#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_wps.h>
#include <PubSubClient.h>
#include <Color.h>
#include <SmartLeds.h>

#define PWM_FREQ 4303 // PWM frequency in Hz
#define PWM_RES 10 // bits of PWM resolution

// segment GPIOs
#define SEG1 27
#define SEG2 25
#define SEG3 32
#define SEG4 26
#define SEG5 18
#define SEG6 33
#define SEG7 19
#define SEG8 23

#define LED_STRIP_PIN 22

const char* ssid     = "Stratum0";
const char* password = "stehtinderinfoecke";

WiFiClient espClient;
PubSubClient client(espClient);
String mqtt_hostname = "";
IPAddress mqtt_server;
SmartLed leds( LED_WS2812, 91, 22, 0, SingleBuffer );


void pwmSetupChannels(unsigned int numChannels, unsigned int frequency = PWM_FREQ, unsigned int resolution = PWM_RES){
  for(int i = 0; i < numChannels; i++){
    if(ledcSetup(i, frequency, resolution) == 0) {
      Serial.println("Error setup of pwm-channel: " + String(i) + " failed!");
    }
  }
}

void attachPwmPins(){
  ledcAttachPin(SEG1, 0);
  ledcAttachPin(SEG2, 1);
  ledcAttachPin(SEG3, 2);
  ledcAttachPin(SEG4, 3);
  ledcAttachPin(SEG5, 4);
  ledcAttachPin(SEG6, 5);
  ledcAttachPin(SEG7, 6);
  ledcAttachPin(SEG8, 7);
  for(int i = 0; i < 8; i++){
    ledcWrite(i, 0); 
  }
}

void setBrightness(int index, int brightness){
  if(index < 1 || index > 11) return;
  if(brightness < 0 || brightness >= pow(2, PWM_RES)) return;
  ledcWrite(index-1, brightness); 
}

void setLed(int index, Rgb color){
  leds[index] = color;
  leds.show();
}


void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.print("Connecting to WiFi.");
  
  pwmSetupChannels(8);
  attachPwmPins();
  setBrightness(5, 48);
  WiFi.begin(ssid, password);

  unsigned int wifi_retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
    wifi_retries++;
    if (wifi_retries > 20) {
      esp_wifi_wps_disable();
      ESP.restart();
    }
  }
  setBrightness(5, 128);
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin("birdfi")) {
    Serial.println("Error setting up MDNS responder!");
    ESP.restart();
  }
  setBrightness(5, 256);

  int n = MDNS.queryService("mqtt", "tcp");
  if (n == 0) {
    Serial.println("No MQTT service found");
    ESP.restart();
  } else {
    if(mqtt_hostname.length() == 0){ 
      mqtt_hostname = MDNS.hostname(0);
      mqtt_server = MDNS.IP(0);
      Serial.print("Found " + String(n) + " mqtt services, connecting to " + mqtt_hostname + "@" + mqtt_server + " .");
    } else {
      
    }
  }
  setBrightness(5, 512);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  client.connect("birdfi");
  unsigned int mqtt_retries = 0;
  while (!client.connected()) {
    delay(300);
    Serial.print(".");
    mqtt_retries++;
    if (mqtt_retries > 20) {
      esp_wifi_wps_disable();
      ESP.restart();
    }
  }
  setBrightness(5, 1022);
  Serial.println(" SUCCESS");
  client.subscribe("birdfi/#");
  Serial.println("Subscribed to birdfi");
  char charBuf[10];
  String(PWM_FREQ).toCharArray(charBuf, 10);
  client.publish("birdfi/pwm_freq", charBuf, true);
  String(PWM_RES).toCharArray(charBuf, 10);
  client.publish("birdfi/pwm_res", charBuf, true);
  
  for ( int i = 0; i != 91; i++ )
      leds[ i ] = Rgb(0,0,0);
  leds.show();
}

void loop() {
  if(!client.loop()){
    ESP.restart();
  }

}

void browseService(const char * service, const char * proto) {

}

void callback(char* topic, byte* payload, unsigned int length){
//  Serial.print("Message arrived [");
//  Serial.print(topic);
//  Serial.print("] ");
  String sPayload;
  for (int i = 0; i < length; i++) {
//    Serial.print((char)payload[i]);
    sPayload.concat((char)payload[i]);
  }
//  Serial.println();
  String sTopic = (String) topic;
  int firstSlashPos = sTopic.indexOf("/");
  int secondSlashPos = sTopic.indexOf("/", firstSlashPos+1);
  String action;
  if(firstSlashPos && secondSlashPos) {
    action = sTopic.substring(firstSlashPos+1, secondSlashPos);
  }
  if(action.equalsIgnoreCase("setBrightness")){
    int ledIndex = sTopic.substring(secondSlashPos+1).toInt();
    int brightness = sPayload.toInt();
//    Serial.println("Action: setBrightness, ledIndex: " + String(ledIndex) + ", brightness: " + String(brightness));
    if(ledIndex != 0){
      setBrightness(ledIndex, brightness);
    }
  } else if(action.equalsIgnoreCase("setLed")){
    int ledIndex = sTopic.substring(secondSlashPos+1).toInt();
    int commaPos= sPayload.indexOf(",");
    int r = sPayload.substring(0, commaPos).toInt();
    int lastCommaPos = commaPos;
    commaPos = sPayload.indexOf(",", lastCommaPos+1);
    int b = sPayload.substring(lastCommaPos+1, commaPos).toInt();
    lastCommaPos = commaPos;
    commaPos = sPayload.indexOf(",", lastCommaPos+1);
    int g = sPayload.substring(lastCommaPos+1, commaPos).toInt();

//    Serial.println("Action: setBrightness, ledIndex: " + String(ledIndex) + ", brightness: " + String(brightness));
    setLed(ledIndex, Rgb(r,g,b));
  }
}

