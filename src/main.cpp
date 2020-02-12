
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

const char* ssid = "test";
const char* password = "cuddlycheetah";
const char* mqtt_server = "192.168.1.149";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

#define PIN_UD D2
#define PIN_INC D3
#define POT_DOWN false
#define POT_UP true
void potCmd(boolean up){
  digitalWrite(PIN_UD, up);
  digitalWrite(PIN_INC, HIGH);
  delayMicroseconds(1e3);
  digitalWrite(PIN_INC, LOW);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  for (int i=0; i<100; i++) {
    potCmd(POT_UP);
    delayMicroseconds(150);
  }
  for (int i=0; i<(char)payload[0]; i++) {
    potCmd(POT_DOWN);
    delayMicroseconds(150);
  }

  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);
  } else {
    digitalWrite(BUILTIN_LED, HIGH);
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "Schrag-Heizung";
    clientId += String(ESP.getChipId(), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),"home/heizung/schrag/alive", 2, true, "0")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("home/heizung/schrag/alive", "1");
      client.publish("home/heizung/schrag/advertise", "0");
      // ... and resubscribe
      client.subscribe("home/heizung/schrag/leistung");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void resetPoti() {
  for (int i=0; i<100; i++) {
    potCmd(POT_UP);
    delayMicroseconds(150);
  }
}

void setup() {
  pinMode(PIN_INC, OUTPUT);
  pinMode(PIN_UD, OUTPUT);
  //pinMode(A0, INPUT);
  resetPoti();
  pinMode(BUILTIN_LED, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    resetPoti();
    reconnect();
  }
  client.loop();
  digitalWrite(BUILTIN_LED, millis() % 600 > 70);

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 50, "%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("home/heizung/schrag/advertise", msg);
  }
}