#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Thông tin WiFi
const char* ssid = "Ko biet";
const char* password = "44444444";

// Thông tin MQTT
const char* mqtt_server = "192.168.0.104";
const char* mqtt_user = "NguyenGiaKhien";
const char* mqtt_password = "b21dccn459";

// Định nghĩa các topic MQTT
const char* mqtt_sensor_topic = "sensor/data";
// Topics cho các thiết bị thường
const char* mqtt_led_control = "device/control/led";
const char* mqtt_fan_control = "device/control/fan";
const char* mqtt_laptop_control = "device/control/laptop";
// Topics cho LED1 và LED2
const char* mqtt_led1_control = "led/control/led1";
const char* mqtt_led2_control = "led/control/led2";

// Khởi tạo các đối tượng
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(D5, DHT11);

// Định nghĩa chân GPIO cho các thiết bị
#define LED_PIN D1    // LED thường
#define FAN_PIN D0    // Quạt
#define LAPTOP_PIN D2 // Laptop
#define LED1_PIN D6   // LED1
#define LED2_PIN D7   // LED2

// Biến theo dõi thời gian
unsigned long lastMsg = 0;
const long interval = 10000; // Gửi dữ liệu mỗi 10 giây

void setup() {
  Serial.begin(115200);
  
  // Khởi tạo các chân output
  pinMode(LED_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(LAPTOP_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  
  // Tắt tất cả thiết bị khi khởi động
  digitalWrite(LED_PIN, LOW);
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(LAPTOP_PIN, LOW);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  
  // Khởi tạo các kết nối
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1884);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Gửi dữ liệu cảm biến định kỳ
  unsigned long now = millis();
  if (now - lastMsg > interval) {
    lastMsg = now;
    sendSensorData();
  }
}

void setup_wifi() {
  delay(10);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      
      // Subscribe vào tất cả các topic điều khiển
      client.subscribe(mqtt_led_control);
      client.subscribe(mqtt_fan_control);
      client.subscribe(mqtt_laptop_control);
      client.subscribe(mqtt_led1_control);
      client.subscribe(mqtt_led2_control);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  // Xử lý điều khiển các thiết bị thường
  if (String(topic) == mqtt_led_control) {
    digitalWrite(LED_PIN, message == "on" ? HIGH : LOW);
  }
  else if (String(topic) == mqtt_fan_control) {
    digitalWrite(FAN_PIN, message == "on" ? HIGH : LOW);
  }
  else if (String(topic) == mqtt_laptop_control) {
    digitalWrite(LAPTOP_PIN, message == "on" ? HIGH : LOW);
  }
  // Xử lý điều khiển LED1 và LED2
  else if (String(topic) == mqtt_led1_control) {
    digitalWrite(LED1_PIN, message == "on" ? HIGH : LOW);
  }
  else if (String(topic) == mqtt_led2_control) {
    digitalWrite(LED2_PIN, message == "on" ? HIGH : LOW);
  }
}

void sendSensorData() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int lightValue = analogRead(A0);
  int dustValue = random(50, 500); // Giả lập giá trị bụi

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Tạo chuỗi JSON
  String payload = "{";
  payload += "\"temperature\":" + String(temperature) + ",";
  payload += "\"humidity\":" + String(humidity) + ",";
  payload += "\"light\":" + String(lightValue) + ",";
  payload += "\"dust\":" + String(dustValue);
  payload += "}";

  // Gửi dữ liệu
  client.publish(mqtt_sensor_topic, payload.c_str());
  Serial.println("Sent: " + payload);
}