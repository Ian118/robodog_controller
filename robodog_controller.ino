#include <Adafruit_seesaw.h>

#include <esp_now.h>
#include <WiFi.h>

#define STICK_H 3
#define STICK_V 2

// E8:9F:6D:20:30:18
uint8_t broadcastAddress[] = {0xE8, 0x9F, 0x6D, 0x20, 0x30, 0x18};

const unsigned button_mask = (1 << 6) | (1 << 7) |
                             (1 << 9) | (1 << 10) |
                             (1 << 14);

Adafruit_seesaw ss;

struct msg
{
    unsigned vertical;
    unsigned horizontal;
} message;

void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    if (!ss.begin(0x49))
    {
        Serial.println("Error: seesaw (joystick) not found");
        return;
    }

    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        while (1)
            delay(20);
    }
}

void loop()
{
    message.horizontal = ss.analogRead(STICK_H);
    message.vertical = ss.analogRead(STICK_V);

    esp_err_t outcome = esp_now_send(broadcastAddress, (uint8_t *)&message, sizeof(message));

    // if (outcome == ESP_OK)
    // {
    //     Serial.println("Mesage sent successfully!");
    // }
    // else
    // {
    //     Serial.println("Error sending the message");
    // }
    delay(50);
}
