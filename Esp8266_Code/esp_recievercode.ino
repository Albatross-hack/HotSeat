#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#define MQTT_HOST "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_DEVICEID "d:hwu:esp8266:myesp"
#define MQTT_USER "" // no need for authentication, for now
#define MQTT_TOKEN "" // no need for authentication, for now
#define MQTT_TOPIC "espnode/evt/button/fmt/json"
#define MQTT_TOPIC_DISPLAY "espnode/cmd/display/fmt/json"
#define NEOPIXEL_TYPE NEO_RGB + NEO_KHZ800
#define RGB_PIN 5
// Structure example to receive data
// Must match the sender structure
// Add WiFi connection information
char ssid[] = "POCO F4";     //  your network SSID (name)
char pass[] = "setuppoco@123"; // your network password
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(1, RGB_PIN, NEOPIXEL_TYPE);
// MQTT objects
void callback(char* topic, byte* payload, unsigned int length);
WiFiClient wifiClient;
PubSubClient mqtt(MQTT_HOST, MQTT_PORT, callback, wifiClient);

typedef struct  {
    char a[32];
    int b;
 
} struct_message;

// Create a struct_message called myData
struct_message myData;

// Create a structure to hold the readings from each board
struct_message board1;
struct_message board2;
int id =0;
int delayamt=1;
int mytime = millis();
char str[100];
// Create an array with all the structures
struct_message boardsStruct[2] = {board1, board2};
volatile boolean haveReading1 = false;
volatile boolean haveReading2 = false;
int heartBeat;
// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  if(strcmp(myData.a,"BOARD2")==0){id=0;haveReading1 = true;}
  if(strcmp(myData.a,"BOARD3")==0){id=1;haveReading2 = true;}
  boardsStruct[id].b = myData.b;
  Serial.print("Bytes received: ");
  Serial.println(len);  
  Serial.print("Board Id: ");
  Serial.println(myData.a);
  Serial.print(haveReading1);
  Serial.println(haveReading2);


}

// variables to hold data
StaticJsonDocument<100> jsonDoc;
JsonObject payload = jsonDoc.to<JsonObject>();
JsonObject status = payload.createNestedObject("d");
static char msg[100];

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  memcpy(&str, payload, sizeof(delayamt));
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] : ");
  
  payload[length] = 0; // ensure valid content is zero terminated so can treat as c-string
  Serial.println((char *)payload);

 
  sscanf(str, "%d", &delayamt);
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.setTimeout(2000);
  while (!Serial) { }
  Serial.println();
  Serial.println("Iot Project Initialization");
  WiFi.mode(WIFI_STA); 

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    ESP.restart();
    return;
  }
  Serial.println("ESP_NOW_ACTIVATED!");
  pixel.setPixelColor(0, 255, 0, 0); 
  pixel.show();  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {

  if (haveReading1 && haveReading2){
    esp_now_deinit();
    haveReading1=false;
    haveReading2=false;
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }   
  while (!mqtt.connected()) {
  Serial.print("Attempting MQTT connection...");
  // Attempt to connect
    pixel.setPixelColor(0, 0, 255, 0); // 255,0
  pixel.show(); 
  if (mqtt.connect(MQTT_DEVICEID, MQTT_USER, MQTT_TOKEN)) {
    Serial.println("MQTT Connected");
    mqtt.subscribe(MQTT_TOPIC_DISPLAY);
    mqtt.loop();
  } else {
    Serial.println("MQTT Failed to connect!");
    delay(5000);
    ESP.reset();
  }
  } 

  status["BOARD2"] = boardsStruct[0].b;
  status["BOARD3"] = boardsStruct[1].b;
  serializeJson(jsonDoc, msg,100);
  Serial.println(msg);
  if (!mqtt.publish(MQTT_TOPIC, msg)) {
      Serial.println("MQTT Publish failed");
  }
  pixel.setPixelColor(0, 0, 0, 255); // 255,0
  pixel.show(); 
    // Pause - but keep polling MQTT for incoming messages
  for (int i = 0; i < 7; i++) {
    mqtt.loop();
    delay(100);
  }

  mqtt.disconnect();
  Serial.println("Active time is(ms) : ");
  float mytime = millis();
  Serial.println(mytime);
  float sleepdelay = (delayamt*1000);
  Serial.println("sleep time is(ms) : ");
  Serial.println(sleepdelay);
  Serial.println("Total Duty Cycle :");    //Find the total Duty Cycle of the Esp Reciever module
  double DC;
  DC= (mytime/(mytime+sleepdelay));
  Serial.println(DC);
  delay((delayamt*1000)-700);
  ESP.restart();   
  }




}
