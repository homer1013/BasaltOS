#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>
#include "TouchScreen_kbv.h"
#define TouchScreen TouchScreen_kbv
#define TSPoint TSPoint_kbv

MCUFRIEND_kbv tft;

const int XP = 6, XM = A2, YP = A1, YM = 7;
TouchScreen ts(XP, YP, XM, YM, 300);

int TS_LEFT = 915;
int TS_RT = 142;
int TS_TOP = 281;
int TS_BOT = 827;

// Raw corner samples (rotation=1, from calibration logs)
float r_tl_x = 908, r_tl_y = 280;
float r_tr_x = 142, r_tr_y = 283;
float r_br_x = 144, r_br_y = 828;
float r_bl_x = 923, r_bl_y = 826;

const int MINPRESSURE = 120;
const int MAXPRESSURE = 1000;

int px = -1, py = -1;

void restorePins() {
  pinMode(YP, OUTPUT);
  pinMode(XM, OUTPUT);
  digitalWrite(YP, HIGH);
  digitalWrite(XM, HIGH);
}

bool readTouch(int *x, int *y, int *z) {
  TSPoint p = ts.getPoint();
  restorePins();
  *z = p.z;
  if (p.z < MINPRESSURE || p.z > MAXPRESSURE) return false;

  float rx = (float)p.x;
  float ry = (float)p.y;

  // Invert bilinear mapping from raw quadrilateral -> normalized u,v.
  float u = (ry - TS_TOP) / (float)(TS_BOT - TS_TOP);
  float v = (rx - TS_RT) / (float)(TS_LEFT - TS_RT);
  if (u < 0) u = 0; if (u > 1) u = 1;
  if (v < 0) v = 0; if (v > 1) v = 1;
  for (int i = 0; i < 4; i++) {
    float left_x = r_tl_x + (r_bl_x - r_tl_x) * v;
    float right_x = r_tr_x + (r_br_x - r_tr_x) * v;
    float left_y = r_tl_y + (r_bl_y - r_tl_y) * v;
    float right_y = r_tr_y + (r_br_y - r_tr_y) * v;

    if (right_x != left_x) u = (rx - left_x) / (right_x - left_x);
    if (u < 0) u = 0; if (u > 1) u = 1;

    float top_x = r_tl_x + (r_tr_x - r_tl_x) * u;
    float bot_x = r_bl_x + (r_br_x - r_bl_x) * u;
    float top_y = r_tl_y + (r_tr_y - r_tl_y) * u;
    float bot_y = r_bl_y + (r_br_y - r_bl_y) * u;
    (void)top_x; (void)bot_x; // only y-axis equation used for v update

    if (bot_y != top_y) v = (ry - top_y) / (bot_y - top_y);
    if (v < 0) v = 0; if (v > 1) v = 1;
  }

  int sx = (int)(u * (tft.width() - 1) + 0.5f);
  int sy = (int)(v * (tft.height() - 1) + 0.5f);
  if (sx < 0) sx = 0; if (sx >= tft.width()) sx = tft.width() - 1;
  if (sy < 0) sy = 0; if (sy >= tft.height()) sy = tft.height() - 1;
  *x = sx;
  *y = sy;
  return true;
}

void setup() {
  Serial.begin(115200);
  uint16_t id = tft.readID();
  if (id == 0xD3D3 || id == 0xFFFF || id == 0x0000 || id == 0x0101) id = 0x9486;
  tft.begin(id);
  tft.setRotation(1);
  tft.fillScreen(0x0000);
  tft.setTextColor(0xFFFF, 0x0000);
  tft.setTextSize(2);
  tft.setCursor(6, 6);
  tft.print("BASELINE PAINT");
  tft.setCursor(6, 26);
  tft.print("0x"); tft.print(id, HEX);
  tft.drawRect(0, 50, tft.width(), tft.height() - 50, 0x07E0);
  Serial.println("baseline ready");
}

void loop() {
  int x, y, z;
  bool down = readTouch(&x, &y, &z);
  if (!down) {
    px = -1;
    py = -1;
    return;
  }
  if (y < 52) return;
  if (px < 0) {
    tft.fillCircle(x, y, 2, 0xF800);
  } else {
    tft.drawLine(px, py, x, y, 0xF800);
    tft.fillCircle(x, y, 2, 0xF800);
  }
  px = x;
  py = y;
}
