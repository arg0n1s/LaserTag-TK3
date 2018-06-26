#include <WiFiUdp.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <stdlib.h>
#include <string.h>

#define LED_PIN 2   // select the pin for the LED

const char *ssid = "TK3";
const char *password = "leet1337";
String inTopic = "";
String outTopic = "";
// No hardcode, this is variable
int playerID = 0;
int teamID = 0;
// Capacity computed with https://arduinojson.org/assistant/
DynamicJsonBuffer jsonBuffer(265);

WiFiUDP Udp;
WiFiClient wifiClient;
PubSubClient client(wifiClient);

IPAddress ipMulti(239, 0, 0, 57);
unsigned int portMulti = 5000;

char packetBuffer[265];

boolean mqttBrokerFound = false;

const uint8_t* parseIP(const char* stuff){
  static uint8_t parsedIP[4];
  static char ipBuff[4];
  char* buffer = ipBuff;
  unsigned int idx = 0;
  
  while(*stuff && idx < 4){
    *buffer = *stuff;
    buffer++;
    
    if(*stuff == '.' || *(stuff+1) == '\0') {
      buffer = ipBuff;
      parsedIP[idx] = (uint8_t) atoi(buffer);
      memset(&buffer[0], 0, 4*sizeof(buffer));
      idx++;
    }
    stuff++;
  }
  return parsedIP;
}

void inChannelCallback(char* topic, byte* payload, unsigned int length) {
  String topicName = String(topic);
  if(topicName.equals(inTopic)) {
    payload[length] = 0;
    // Deserialize the received JSON message
    JsonObject& root = jsonBuffer.parseObject(payload);
    if(!validateMQTTInputMessage) {
      return;
    }
    int id = root["player_id"];
    // Discard messages that are for other player
    if(id != playerID) {
      return;
    }
    
    String event = root["event"];
    int value = root["value"];
    if(event.equals("hit") && value == 1) {
      Serial.print("Blaster of player ");
      Serial.print(playerID);
      Serial.println(" hit");
      // TODO: Blaster hit target
    }
  }
}

void publishMessage(String message) {
  client.publish(outTopic, message.c_str());
  if(client.publish(topic, value.c_str())){
    Serial.println("Publish ok");
  }
  else{
    Serial.println("Publish failed.");
  }
}

String createDiscoveryMessage() {
  String message = "";
  if(WiFi.status() != WL_CONNECTED) {
    return message;
  }
  JsonObject& root = jsonBuffer.createObject();
  root["ip_address"] = WiFi.localIP().toString();
  //JsonArray& sensorChannels = root.createNestedArray("sensor_channels");
  //JsonArray& actuatorChannels = root.createNestedArray("actuator_channels");
  //actuatorChannels.add(String(actuatorTopic));
  root.printTo(message);
  return message;
  // JSON Message with following format:
    /*
    {
    "ip_address": "192.168.0.1",
    }
  */
  
}

String createMQTTMessage(String event, String value, String deviceType) {
  String message = "";
  // Clear JSON Buffer
  jsonBuffer.clear();
  JsonObject& root = jsonBuffer.createObject();
  root["player_id"] = playerID;
  root["device_type"] = deviceType;
  root["event"] = event;
  root["value"] = value;
  root.printTo(message);
  return message;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("esp32Client-Actuator")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic","This is the EX4 of TK3");
      // ... and resubscribe
      client.subscribe(actuatorTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void processAck() {
   // read the packet into packetBufffer
    int len = Udp.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = 0;
    }
    
    // Clear JSON Buffer
    jsonBuffer.clear();
    // Deserialize the received JSON message
    JsonObject& root = jsonBuffer.parseObject(packetBuffer);

    // Test if parsing succeeds.
    if (!root.success()) {
      Serial.println("parseObject() failed");
      return;
    }

    // Validate Ack
    if(!validateAck(root)) {
      return;
    }

    const String receiverIp = root["receiver_ip"];
    // Check whether this device is the receiver of the message
    if(!receiverIp.equals(WiFi.localIP().toString())) {
      return;
    }

    const String brokerIp = root["sender_ip"];
    const uint32_t mqttPort = root["mqtt_port"];
    inTopic = root["in_topic"];
    outTopic = root["out_topic"];
    playerID = root["player_id"];
    teamID = = root["team_id"];

    // Start PubSubClient
    IPAddress mqttServer(parseIP(brokerIp.c_str()));
    client.setServer(mqttServer, mqttPort);
    client.setCallback(inChannelCallback);
    mqttBrokerFound = true;
}

bool validateAck(JsonObject& root) {
  if(root.containsKey("sender_ip") && root.containtsKey("receiver_ip") && root.containsKey("mqtt_port")
     && root.containsKey("in_topic") && root.containsKey("out_topic") && root.containsKey("player_id") && root.containsKey("team_id")) {
    return true;
  }
  return false;
}

bool validateMQTTInputMessage(JsonObject& root) {
  if(root.containsKey("player_id") && root.containtsKey("event") && root.containsKey("value")) {
    return true;
  }
  return false;
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  pinMode(LED_PIN, OUTPUT);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected! With IP: ");
  delay(3000);
  Serial.println(WiFi.localIP());
  
  Udp.beginMulticast(ipMulti, portMulti);
}

void loop() {
  if(!mqttBrokerFound) {
    Udp.beginMulticastPacket();
    Serial.println("Advertise");
    String message = createDiscoveryMessage();
    Udp.write((const uint8_t*)message.c_str(), sizeof(char)*message.length()); 
    Udp.endPacket();  
    delay(1000);
  
    int packetSize = Udp.parsePacket();
    if (packetSize) {
      Serial.print("Received packet of size ");
      Serial.println(packetSize);
      Serial.print("From ");
      IPAddress remoteIp = Udp.remoteIP();
      Serial.print(remoteIp);
      Serial.print(", port ");
      Serial.println(Udp.remotePort());
  
      processAck();
    }
  
    delay(1000);
    yield(); 
  }
  else{
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
  }
}
