#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#define WIFI_SSID "iPhone"
#define WIFI_PASS "no u don't"

#define MQTT_SERVER "arenardcpp.duckdns.org"
#define MQTT_PORT 1883

#define PLAYER "X"

const char* TOPIC_MOVE   = "ttt/game1/move";
const char* TOPIC_STATE  = "ttt/game1/state";
const char* TOPIC_STATUS = "ttt/game1/status";
const char* TOPIC_RESET  = "ttt/game1/reset";

WiFiClient espClient;
PubSubClient mqtt(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// 4x4 keypad layout
const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {14, 27, 26, 25};
byte colPins[COLS] = {13, 32, 35, 34};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

String boardState = "";
String statusMsg = "";

void showLCD() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(statusMsg.substring(0, 16));

  lcd.setCursor(0, 1);
  lcd.print(boardState.substring(0, 16));
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg = "";

  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  if (String(topic) == TOPIC_STATE) {
    boardState = msg;
    Serial.println("Board: " + boardState);
  }

  if (String(topic) == TOPIC_STATUS) {
    statusMsg = msg;
    Serial.println("Status: " + statusMsg);
  }

  showLCD();
}

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
}

void connectMQTT() {
  while (!mqtt.connected()) {
    String clientId = "ESP32-TTT-" + String(random(0xffff), HEX);

    if (mqtt.connect(clientId.c_str())) {
      Serial.println("MQTT connected");

      mqtt.subscribe(TOPIC_STATE);
      mqtt.subscribe(TOPIC_STATUS);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MQTT connected");
    } else {
      delay(3000);
    }
  }
}

void sendMove(char key) {
  if (key < '1' || key > '9') return;

  String msg = String(PLAYER) + " " + String(key);
  mqtt.publish(TOPIC_MOVE, msg.c_str());

  Serial.println("Sent move: " + msg);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Move sent:");
  lcd.setCursor(0, 1);
  lcd.print(msg);
}

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Tic Tac Toe");

  connectWiFi();

  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  connectMQTT();

  Serial.println("Press keypad 1-9 for move.");
  Serial.println("Press * to reset.");
}

void loop() {
  if (!mqtt.connected()) {
    connectMQTT();
  }

  mqtt.loop();

  char key = keypad.getKey();

  if (key) {
    Serial.print("Key pressed: ");
    Serial.println(key);

    if (key >= '1' && key <= '9') {
      sendMove(key);
    }

    if (key == '*') {
      mqtt.publish(TOPIC_RESET, "reset");
      Serial.println("Reset sent");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Reset sent");
    }
  }
}