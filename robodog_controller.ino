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

constexpr float M_PI_8 = M_PI / 8.0f;

// E8:9F:6D:20:30:18
uint8_t broadcastAddress[] = {0xE8, 0x9F, 0x6D, 0x20, 0x30, 0x18};

const unsigned button_mask = (1 << BUTTON_RIGHT) | (1 << BUTTON_DOWN) |
                             (1 << BUTTON_LEFT) | (1 << BUTTON_UP) |
                             (1 << BUTTON_SEL);

union
{
  struct
  {
    unsigned char right : 7;
    unsigned char down : 1;
    unsigned char left : 2;
    unsigned char up : 1;
    unsigned char sel : 4;
  } btn;
  unsigned masked_btns;
} btns;

Adafruit_seesaw ss;
Adafruit_NeoPixel pixel(1, 0, NEO_GRB + NEO_KHZ800);

struct
{
  float v_x, v_y, yaw, pitch, roll;
  bool poweroff;
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
  float fwd = ((signed)ss.analogRead(STICK_V) - 512) / 512.0f; // divide by 512
  float right = ((signed)ss.analogRead(STICK_H) - 512) / 512.0f;
  message.v_x = abs(fwd) > .02 ? 0.05f * fwd : 0.0f; // Max at .02 m/s
  message.v_y = abs(right) > .02 ? 0.05f * right : 0.0f;
  btns.masked_btns = button_mask & ~ss.digitalReadBulk(button_mask);

  message.roll = btns.btn.right  ? -M_PI_8
                 : btns.btn.left ? M_PI_8
                                 : 0.0f;
  message.pitch = btns.btn.up     ? M_PI_8
                  : btns.btn.down ? -M_PI_8
                                  : 0.0f;
  message.poweroff = btns.btn.sel;

  esp_now_send(broadcastAddress, (uint8_t *)&message, sizeof(message));

  delay(50);
}
