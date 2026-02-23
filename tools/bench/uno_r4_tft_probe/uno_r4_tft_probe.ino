#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>

MCUFRIEND_kbv tft;

static uint16_t probe_id() {
  uint16_t id = tft.readID();
  // Some shields return dummy IDs until bus timing is stable.
  if (id == 0xD3D3 || id == 0xFFFF || id == 0x0000 || id == 0x0101) {
    id = 0x9486;
  }
  return id;
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}

  uint16_t id = probe_id();
  tft.begin(id);
  tft.setRotation(1);

  tft.fillScreen(0x0000);
  tft.setTextColor(0xFFFF);
  tft.setTextSize(2);
  tft.setCursor(20, 20);
  tft.print("UNO R4 TFT OK");
  tft.setCursor(20, 50);
  tft.print("ID: 0x");
  tft.print(id, HEX);

  Serial.print("LCD ID = 0x");
  Serial.println(id, HEX);
}

void loop() {}
