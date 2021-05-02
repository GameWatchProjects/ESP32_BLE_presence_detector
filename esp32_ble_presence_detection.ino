/*

   This detects advertising messages of BLE devices and compares it with stored MAC addresses.
   If one matches, it sends an MQTT message to MQTT broker.


  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.

*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <WiFi.h>
#include <PubSubClient.h>

const char * ssid = "SSID";                       // Your WiFi SSID
const char * password = "PASSWORD";               // Your WiFi Passowrd
const char * mqtt_server = "192.168.x.4";         // Your MQTT broker IP address
const int mqtt_port = 1883;                       // Your MQTT Broker Port
const char * mqtt_user = "MQTT_USER";             // Your MQTT broker user
const char * mqtt_password = "MQTT_PASS";         // Your MQTT broker password
const char * mqtt_topic = "MQTT_TOPIC";           // Your MQTT broker topic with no spaces
const char * ESPHostname = "ESP-32_BLE_Scanner";  // WYour iFi & MQTT broker Client Name

unsigned long entry;

WiFiClient client;
PubSubClient MQTTclient(client);

int scanTime = 5;
const byte numBeacons = 3;                        // Edit here the amount of your beacons from the below String
String knownMAC[numBeacons] = {
  "58:9e:c6:1a:3c:13",                            // Place here your Beacons MAC's
  "58:9e:c6:1a:3b:e3",                            // Place here your Beacons MAC's
  "58:9e:c6:1a:3c:05",                            // Place here your Beacons MAC's
};

const int minRSSI = -120;                         // Edit there the minimal RSSI to found a Beacon
const byte maxNotFound = 10;                      // Edit here the attemps to found a beacon, before it was listed as not found

byte numNotFoundMAC[numBeacons];

BLEScan * pBLEScan;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {

    void onResult(BLEAdvertisedDevice advertisedDevice) {

      Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());

    }
};

void MQTTcallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];

  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  Serial.printf("Starting ESP32 %d\n");

  pinMode(2, OUTPUT);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  entry = millis();
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.hostname(ESPHostname);
    if (millis() - entry >= 15000) esp_restart();
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());

  MQTTclient.setServer(mqtt_server, mqtt_port);
  MQTTclient.setCallback(MQTTcallback);
  Serial.println("Connect to MQTT server ...");
  while (!MQTTclient.connected()) {
    Serial.print("Attempting MQTT connection ...");
    // Attempt to connect
    if (MQTTclient.connect(ESPHostname, mqtt_user, mqtt_password)) {
      Serial.println(" connected");
      // Subscribe
      MQTTclient.subscribe(mqtt_topic);

    } else {
      Serial.print("failed, rc=");
      Serial.print(MQTTclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(1000);
    }
  }

  BLEDevice::init("");

  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan -> setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan -> setActiveScan(true);
  pBLEScan -> setInterval(100);
  pBLEScan -> setWindow(99);
}

void loop() {

  Serial.println("Start scan!");
  digitalWrite(2, HIGH);

  BLEScanResults foundDevices = pBLEScan -> start(scanTime, false);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");
  digitalWrite(2, LOW);

  int count = foundDevices.getCount();

  for (int j = 0; j < count; j++) {
    BLEAdvertisedDevice d = foundDevices.getDevice(j);
    String dMAC = d.getAddress().toString().c_str();

    for (byte i = 0; i < numBeacons; i++) {
      if (dMAC == knownMAC[i]) {
        Serial.print("FOUND VALID MAC: ");
        Serial.println(dMAC);
        Serial.print("RSSI: ");
        Serial.println(d.getRSSI());

        String macTopic = String(mqtt_topic) + String("/") + String(dMAC) + String("/MAC");
        String macMsg = String(dMAC);
        MQTTclient.publish(macTopic.c_str(), macMsg.c_str());
        Serial.print("Publish MQTT MAC Topic value: ");
        Serial.println(macTopic.c_str());
        Serial.print("Publish MQTT MAC value: ");
        Serial.println(macMsg.c_str());
        
        String rssiTopic = String(mqtt_topic) + String("/") + String(dMAC) + String("/RSSI");
        String rssiMsg = String(d.getRSSI());
        MQTTclient.publish(rssiTopic.c_str(), rssiMsg.c_str());
        Serial.print("Publish MQTT RSSI Topic value: ");
        Serial.println(rssiTopic.c_str());
        Serial.print("Publish MQTT RSSI value: ");
        Serial.println(rssiMsg.c_str());
        
        String stateTopic = String(mqtt_topic) + String("/") + String(dMAC) + String("/STATE");
        String stateMsg = String("true");
        MQTTclient.publish(stateTopic.c_str(), stateMsg.c_str());
        Serial.print("Publish MQTT STATE Topic: ");
        Serial.println(stateTopic.c_str());
        Serial.print("Publish MQTT STATE value: ");
        Serial.println(stateMsg.c_str());

        if (d.getRSSI() > minRSSI) {
          numNotFoundMAC[i] = 0;
        }

        break;
      }
    }
  }

  bool deviceFound = false;

  for (byte i = 0; i < numBeacons; i++) {
    if (numNotFoundMAC[i] < maxNotFound) {
      numNotFoundMAC[i]++;
    }

    if (numNotFoundMAC[i] < maxNotFound) {
      deviceFound = true;
    } else {

      Serial.printf("NOT FOUND FOR %d TIMES OR MORE\n", maxNotFound);

    }
  }

  pBLEScan -> clearResults();
  Serial.println("Waiting for 10 seconds ...");
  delay(10000); // Delay

} 
