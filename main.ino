#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include <qrcodeoled.h>
#include <SSD1306.h>

SSD1306 display(0x3c, D2, D1); // Only change
QRcodeOled qrcode(&display);

#define TIME_ZONE +7

float h;
float t;
unsigned long lastMillis = 0;
unsigned long previousMillis = 0;
const long interval = 5000;

#define AWS_IOT_PUBLISH_TOPIC "esp8266/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp8266/sub"

time_t now;
time_t nowish = 1510592825;

WiFiClientSecure net;

BearSSL::X509List cert(AWS_CERT_CA);
BearSSL::X509List client_crt(AWS_CERT_CRT);
BearSSL::PrivateKey key(AWS_CERT_PRIVATE);

PubSubClient client(net);

const int LED1 = D5; // Thay thế D5 bằng chân GPIO của đèn LED số 1
const int LED2 = D6; // Thay thế D6 bằng chân GPIO của đèn LED số 2
const int LED3 = D7; // Thay thế D7 bằng chân GPIO của đèn LED số 3
const int LED4 = D8; // Thay thế D7 bằng chân GPIO của đèn LED số 4

void blinkLED(int pin, int times)
{
    for (int i = 0; i < times; i++)
    {
        digitalWrite(pin, HIGH);
        delay(500);
        digitalWrite(pin, LOW);
        if (i < times - 1)
        {
            delay(500);
        }
    }
}

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    // Tạo một buffer lớn đủ để chứa tin nhắn
    char message[length + 1];
    strncpy(message, (char *)payload, length);
    message[length] = '\0';
    Serial.println(message);

    // Phân tích tin nhắn JSON
    StaticJsonDocument<200> doc;
    deserializeJson(doc, message);

    // Kiểm tra nội dung của tin nhắn
    if (doc.containsKey("order"))
    {
        JsonObject order = doc["order"].as<JsonObject>();
        if (order.containsKey("bento"))
        {
            int times = order["bento"];
            blinkLED(LED1, times);
        }
        if (order.containsKey("cupcake"))
        {
            int times = order["cupcake"];
            blinkLED(LED2, times);
        }
        if (order.containsKey("chocolate"))
        {
            int times = order["chocolate"];
            blinkLED(LED3, times);
        }
        if (order.containsKey("coffee"))
        {
            int times = order["coffee"];
            blinkLED(LED4, times);
        }
    }
}

void connectNTP(void)
{
    Serial.print("Setting time using SNTP");
    configTime(TIME_ZONE * 3600, 0 * 3600, "pool.ntp.org", "time.nist.gov");
    now = time(nullptr);
    while (now < nowish)
    {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println("done!");
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print("Current time: ");
    Serial.print(asctime(&timeinfo));
}

void publishMessage()
{
    StaticJsonDocument<200> doc;
    doc["message"] = "Hello world";
    char jsonBuffer[100];
    serializeJson(doc, jsonBuffer); // print to client

    client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void connectWifi()
{
    // Kết nối WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi");
}

void connectAWSIoT()
{
    // Cấu hình cho WiFiClientSecure
    net.setTrustAnchors(&cert);
    net.setClientRSACert(&client_crt, &key);

    // Cấu hình kết nối MQTT
    client.setServer(AWS_IOT_ENDPOINT, 8883);
    client.setCallback(callback);

    // Kết nối MQTT
    while (!client.connect(THINGNAME))
    {
        Serial.print(".");
        delay(1000);
    }

    Serial.println("Connected to AWS IoT");

    // Tại đây bạn có thể đăng ký nhận tin nhắn từ một chủ đề
    client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
}

void displayQR()
{
    qrcode.init();
    qrcode.create("https://svm.datluyendevops.online/?id=ua3dXFyQwMSMzvzEC");
}

void setup()
{
    Serial.begin(115200);
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(LED4, OUTPUT);

    connectWifi();
    connectNTP();
    connectAWSIoT();
    displayQR();
}

void loop()
{
    // Đảm bảo ESP8266 vẫn kết nối với AWS IoT
    if (!client.connected())
    {
        while (!client.connect(THINGNAME))
        {
            Serial.print(".");
            delay(1000);
        }
    }
    client.loop();
    // Đây là nơi bạn có thể gửi tin nhắn đến một chủ đề
    // publishMessage();
}