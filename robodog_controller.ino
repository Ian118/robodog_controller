#include <Adafruit_seesaw.h>
#include <Adafruit_NeoPixel.h>

#include <esp_now.h>
#include <WiFi.h>

#define STICK_H 3
#define STICK_V 2

#define BUTTON_RIGHT 6
#define BUTTON_DOWN 7
#define BUTTON_LEFT 9
#define BUTTON_UP 10
#define BUTTON_SEL 14

// E8:9F:6D:20:30:18
uint8_t broadcastAddress[] = {0xE8, 0x9F, 0x6D, 0x20, 0x30, 0x18};

const unsigned button_mask = (1 << BUTTON_RIGHT) | (1 << BUTTON_DOWN) |
                             (1 << BUTTON_LEFT) | (1 << BUTTON_UP) |
                             (1 << BUTTON_SEL);
unsigned btn_states;

Adafruit_seesaw ss;
Adafruit_NeoPixel pixel(1, 0, NEO_GRB + NEO_KHZ800);

struct
{
  unsigned vertical;
  unsigned horizontal;
  bool right;
  bool down;
  bool left;
  bool up;
  bool sel;
} message;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  if (status == ESP_NOW_SEND_FAIL)
    esp_deep_sleep_start();
}

void setup()
{
  // Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    ESP.restart();
  }

  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    ESP.restart();
  }

  esp_now_register_send_cb(OnDataSent);

  if (!ss.begin(0x49))
  {
    Serial.println("Error: seesaw (joystick) not found");
    ESP.restart();
  }

  ss.pinModeBulk(button_mask, INPUT_PULLUP);
  ss.setGPIOInterrupts(button_mask, 1);
}

void loop()
{
  message.horizontal = ss.analogRead(STICK_H);
  message.vertical = ss.analogRead(STICK_V);
  btn_states = ss.digitalReadBulk(button_mask);
  message.right = btn_states & (1 << BUTTON_RIGHT);
  message.down = btn_states & (1 << BUTTON_DOWN);
  message.left = btn_states & (1 << BUTTON_LEFT);
  message.up = btn_states & (1 << BUTTON_UP);
  message.sel = btn_states & (1 << BUTTON_SEL);

  esp_now_send(broadcastAddress, (uint8_t *)&message, sizeof(message));

  delay(50);
}
