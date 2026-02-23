#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>
#include "TouchScreen_kbv.h"
#define TouchScreen TouchScreen_kbv
#define TSPoint TSPoint_kbv

MCUFRIEND_kbv tft;

// Try multiple common UNO shield touch wirings.
// map0: typical 3.5" mcufriend-style
// map1/map2/map3: common alternates seen on 2.4"/2.8" shields
TouchScreen ts0(8, A3, A2, 9, 300);
TouchScreen ts1(6, A1, A2, 7, 300);
TouchScreen ts2(7, A1, A2, 6, 300);
TouchScreen ts3(6, A2, A1, 7, 300);

struct TouchMapProfile {
  const char *name;
  TouchScreen *ts;
  int yp;
  int xm;
};

TouchMapProfile touch_profiles[] = {
  {"map0(8,A3,A2,9)", &ts0, A3, A2},
  {"map1(6,A1,A2,7)", &ts1, A1, A2},
  {"map2(7,A1,A2,6)", &ts2, A1, A2},
  {"map3(6,A2,A1,7)", &ts3, A2, A1},
};

int touch_map_idx = 1;

// Raw axis bounds for conversion:
// rawTouchX = p.x, rawTouchY = p.y
static int RAW_X_MIN = 280;
static int RAW_X_MAX = 840;
static bool MAP_SWAP_XY = false;
static int RAW_Y_MIN = 141;
static int RAW_Y_MAX = 923;
static bool MAP_FLIP_X = false;
static bool MAP_FLIP_Y = false;
static int TS_MINZ = 20;
static int TS_MAXZ = 1000;
static const int CAL_MINZ = 80;
static int MAP_OUT_X_MIN = 0;
static int MAP_OUT_X_MAX = 479;
static int MAP_OUT_Y_MIN = 60;
static int MAP_OUT_Y_MAX = 319;
static bool MAP_USE_LUT = true;

static bool MAP_AFFINE = false;
// Raw corner samples TL, TR, BR, BL for bilinear raw->screen mapping.
static float R_TL_X = 280, R_TL_Y = 908;
static float R_TR_X = 283, R_TR_Y = 142;
static float R_BR_X = 828, R_BR_Y = 144;
static float R_BL_X = 826, R_BL_Y = 923;

static const int LUT_N = 4;
static const int LUT_RX[LUT_N][LUT_N] = {
  {289, 271, 291, 287},
  {447, 420, 434, 438},
  {607, 585, 583, 611},
  {808, 720, 716, 808}
};
static const int LUT_RY[LUT_N][LUT_N] = {
  {911, 663, 407, 142},
  {924, 675, 410, 146},
  {927, 667, 405, 147},
  {902, 681, 411, 153}
};
static const int LUT_SX[LUT_N] = {20, 166, 313, 460};
static const int LUT_SY[LUT_N] = {70, 146, 223, 300};

String line;
bool paint_mode = false;
bool touchwatch = false;
uint16_t paint_color = 0xFFFF;
uint16_t lcd_id = 0;
unsigned long last_touch_log_ms = 0;

enum CalState {
  CAL_OFF = 0,
  CAL_TL = 1,
  CAL_TR = 2,
  CAL_BR = 3,
  CAL_BL = 4
};
CalState cal_state = CAL_OFF;
int cal_raw_x[4] = {280, 283, 828, 826};  // TL, TR, BR, BL
int cal_raw_y[4] = {908, 142, 144, 923};  // TL, TR, BR, BL
unsigned long cal_last_capture_ms = 0;
unsigned long cal_last_log_ms = 0;

bool flappy_mode = false;
bool flappy_game_over = false;
float bird_y = 120.0f;
float bird_v = 0.0f;
int bird_x = 64;
int pipe_x = 0;
int pipe_w = 34;
int gap_y = 120;
int gap_h = 95;
int score = 0;
bool pipe_passed = false;
unsigned long flappy_last_ms = 0;
bool paint_has_prev = false;
int paint_prev_x = 0;
int paint_prev_y = 0;
int stable_press_count = 0;

int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

int mapClamp(int v, int inMin, int inMax, int outMin, int outMax) {
  v = clampi(v, inMin, inMax);
  long num = (long)(v - inMin) * (outMax - outMin);
  long den = (long)(inMax - inMin);
  if (den == 0) return outMin;
  return outMin + (int)(num / den);
}

void recomputeAffineFromCalCorners() {
  MAP_AFFINE = false;
  R_TL_X = (float)cal_raw_x[0]; R_TL_Y = (float)cal_raw_y[0];
  R_TR_X = (float)cal_raw_x[1]; R_TR_Y = (float)cal_raw_y[1];
  R_BR_X = (float)cal_raw_x[2]; R_BR_Y = (float)cal_raw_y[2];
  R_BL_X = (float)cal_raw_x[3]; R_BL_Y = (float)cal_raw_y[3];
}

void restoreTftPins() {
  // map1 wiring (XP=6, YP=A1, XM=A2, YM=7).
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  digitalWrite(A1, HIGH);
  digitalWrite(A2, HIGH);
}

TSPoint sampleTouch() {
  TSPoint p = ts1.getPoint();
  restoreTftPins();
  return p;
}

int rawTouchX(const TSPoint &p) { return p.x; }
int rawTouchY(const TSPoint &p) { return p.y; }
int logicalRawX(const TSPoint &p) { return MAP_SWAP_XY ? rawTouchY(p) : rawTouchX(p); }
int logicalRawY(const TSPoint &p) { return MAP_SWAP_XY ? rawTouchX(p) : rawTouchY(p); }

float bilerp(float q00, float q10, float q01, float q11, float u, float v) {
  float a = q00 + (q10 - q00) * u;
  float b = q01 + (q11 - q01) * u;
  return a + (b - a) * v;
}

bool solveCellUV(
  float rx, float ry,
  int r0x, int r1x, int r2x, int r3x,
  int r0y, int r1y, int r2y, int r3y,
  float *u_out, float *v_out, float *err_out
) {
  float u = 0.5f;
  float v = 0.5f;
  for (int i = 0; i < 7; i++) {
    float px = bilerp((float)r0x, (float)r1x, (float)r2x, (float)r3x, u, v);
    float py = bilerp((float)r0y, (float)r1y, (float)r2y, (float)r3y, u, v);

    float ex = px - rx;
    float ey = py - ry;

    float dxdu = (float)(r1x - r0x) + ((float)(r3x - r2x) - (float)(r1x - r0x)) * v;
    float dxdv = (float)(r2x - r0x) + ((float)(r3x - r1x) - (float)(r2x - r0x)) * u;
    float dydu = (float)(r1y - r0y) + ((float)(r3y - r2y) - (float)(r1y - r0y)) * v;
    float dydv = (float)(r2y - r0y) + ((float)(r3y - r1y) - (float)(r2y - r0y)) * u;

    float det = dxdu * dydv - dxdv * dydu;
    if (det > -1e-6f && det < 1e-6f) break;

    float du = (-ex * dydv + ey * dxdv) / det;
    float dv = (-ey * dxdu + ex * dydu) / det;

    u += du;
    v += dv;
  }

  float px = bilerp((float)r0x, (float)r1x, (float)r2x, (float)r3x, u, v);
  float py = bilerp((float)r0y, (float)r1y, (float)r2y, (float)r3y, u, v);
  float ex = px - rx;
  float ey = py - ry;
  float err = sqrtf(ex * ex + ey * ey);

  *u_out = u;
  *v_out = v;
  *err_out = err;
  return true;
}

bool mapRawToScreenLut(int rawx, int rawy, int *sx, int *sy) {
  float rx = (float)rawx;
  float ry = (float)rawy;

  float best_u = 0.0f;
  float best_v = 0.0f;
  int best_row = -1;
  int best_col = -1;
  float best_score = 1e9f;

  for (int row = 0; row < LUT_N - 1; row++) {
    for (int col = 0; col < LUT_N - 1; col++) {
      int r0x = LUT_RX[row][col];
      int r1x = LUT_RX[row][col + 1];
      int r2x = LUT_RX[row + 1][col];
      int r3x = LUT_RX[row + 1][col + 1];
      int r0y = LUT_RY[row][col];
      int r1y = LUT_RY[row][col + 1];
      int r2y = LUT_RY[row + 1][col];
      int r3y = LUT_RY[row + 1][col + 1];

      float u = 0.0f, v = 0.0f, err = 0.0f;
      if (!solveCellUV(rx, ry, r0x, r1x, r2x, r3x, r0y, r1y, r2y, r3y, &u, &v, &err)) {
        continue;
      }
      float oob = 0.0f;
      if (u < 0.0f) oob += -u;
      if (u > 1.0f) oob += u - 1.0f;
      if (v < 0.0f) oob += -v;
      if (v > 1.0f) oob += v - 1.0f;
      float score = err + oob * 200.0f;
      if (score < best_score) {
        best_score = score;
        best_u = u;
        best_v = v;
        best_row = row;
        best_col = col;
      }
    }
  }

  if (best_row < 0 || best_col < 0) return false;

  if (best_u < 0.0f) best_u = 0.0f;
  if (best_u > 1.0f) best_u = 1.0f;
  if (best_v < 0.0f) best_v = 0.0f;
  if (best_v > 1.0f) best_v = 1.0f;

  int s00x = LUT_SX[best_col];
  int s10x = LUT_SX[best_col + 1];
  int s00y = LUT_SY[best_row];
  int s01y = LUT_SY[best_row + 1];

  float xf = (float)s00x + (float)(s10x - s00x) * best_u;
  float yf = (float)s00y + (float)(s01y - s00y) * best_v;

  *sx = clampi((int)(xf + 0.5f), MAP_OUT_X_MIN, MAP_OUT_X_MAX);
  *sy = clampi((int)(yf + 0.5f), MAP_OUT_Y_MIN, MAP_OUT_Y_MAX);
  return true;
}

void mapRawToScreen(int rawx, int rawy, int *sx, int *sy) {
  if (MAP_USE_LUT) {
    if (mapRawToScreenLut(rawx, rawy, sx, sy)) return;
  }
  // Bilinear inverse: raw quadrilateral -> normalized u,v -> screen xy.
  float rx = (float)rawx;
  float ry = (float)rawy;
  float u = (rx - RAW_X_MIN) / (float)(RAW_X_MAX - RAW_X_MIN);
  float v = (ry - RAW_Y_MIN) / (float)(RAW_Y_MAX - RAW_Y_MIN);
  if (u < 0) u = 0; if (u > 1) u = 1;
  if (v < 0) v = 0; if (v > 1) v = 1;
  for (int i = 0; i < 4; i++) {
    float left_x = R_TL_X + (R_BL_X - R_TL_X) * v;
    float right_x = R_TR_X + (R_BR_X - R_TR_X) * v;
    if (right_x != left_x) u = (rx - left_x) / (right_x - left_x);
    if (u < 0) u = 0; if (u > 1) u = 1;

    float top_y = R_TL_Y + (R_TR_Y - R_TL_Y) * u;
    float bot_y = R_BL_Y + (R_BR_Y - R_BL_Y) * u;
    if (bot_y != top_y) v = (ry - top_y) / (bot_y - top_y);
    if (v < 0) v = 0; if (v > 1) v = 1;
  }
  int x = MAP_OUT_X_MIN + (int)(u * (MAP_OUT_X_MAX - MAP_OUT_X_MIN) + 0.5f);
  int y = MAP_OUT_Y_MIN + (int)(v * (MAP_OUT_Y_MAX - MAP_OUT_Y_MIN) + 0.5f);
  *sx = clampi(x, MAP_OUT_X_MIN, MAP_OUT_X_MAX);
  *sy = clampi(y, MAP_OUT_Y_MIN, MAP_OUT_Y_MAX);
}

void drawBanner() {
  tft.fillScreen(0x0000);
  tft.fillRect(0, 0, tft.width(), 22, 0x001F);
  tft.setTextColor(0xFFFF, 0x001F);
  tft.setTextSize(2);
  tft.setCursor(4, 4);
  tft.print("Basalt UNO R4 bench");

  tft.setTextColor(0x07E0, 0x0000);
  tft.setCursor(4, 28);
  tft.print("shell + paint + cal + flappy");

  tft.setTextColor(0xFFE0, 0x0000);
  tft.setCursor(4, 46);
  tft.print("Use serial: help");
}

void shellHelp() {
  Serial.println("commands:");
  Serial.println("  help");
  Serial.println("  cls");
  Serial.println("  id");
  Serial.println("  paint on|off");
  Serial.println("  color white|red|green|blue|yellow");
  Serial.println("  touch");
  Serial.println("  touchwatch on|off");
  Serial.println("  touchmap <0..3>");
  Serial.println("  touchscan");
  Serial.println("  z <minz>");
  Serial.println("  zmax <maxz>");
  Serial.println("  cal start|show|reset");
  Serial.println("  lut on|off");
  Serial.println("  flipx on|off");
  Serial.println("  flipy on|off");
  Serial.println("  swapxy on|off");
  Serial.println("  flappy on|off");
  Serial.println("  fill white|black|red|green|blue|yellow");
}

void printTouchSample(const TSPoint &p) {
  Serial.print("touch raw x=");
  Serial.print(rawTouchX(p));
  Serial.print(" y=");
  Serial.print(rawTouchY(p));
  Serial.print(" z=");
  Serial.print(p.z);
  Serial.println(" map=1 map1(6,A1,A2,7)");
}

bool touchPressureValid(const TSPoint &p) {
  // Resistive readings outside this range are typically floating/noise on this shield.
  return (p.z >= 0 && p.z <= 1200);
}

void printCal() {
  Serial.print("cal raw_x_min="); Serial.print(RAW_X_MIN);
  Serial.print(" raw_x_max="); Serial.print(RAW_X_MAX);
  Serial.print(" flip_x="); Serial.print(MAP_FLIP_X ? 1 : 0);
  Serial.print(" swap_xy="); Serial.print(MAP_SWAP_XY ? 1 : 0);
  Serial.print(" raw_y_min="); Serial.print(RAW_Y_MIN);
  Serial.print(" raw_y_max="); Serial.print(RAW_Y_MAX);
  Serial.print(" flip_y="); Serial.print(MAP_FLIP_Y ? 1 : 0);
  Serial.print(" out_x_min="); Serial.print(MAP_OUT_X_MIN);
  Serial.print(" out_x_max="); Serial.print(MAP_OUT_X_MAX);
  Serial.print(" out_y_min="); Serial.print(MAP_OUT_Y_MIN);
  Serial.print(" out_y_max="); Serial.print(MAP_OUT_Y_MAX);
  Serial.print(" minz="); Serial.print(TS_MINZ);
  Serial.print(" maxz="); Serial.println(TS_MAXZ);
  Serial.print(" lut="); Serial.println(MAP_USE_LUT ? 1 : 0);
}

void calTargetForIndex(int idx, int *tx, int *ty) {
  const int mx = 14;
  const int top_ui = 60;
  const int my_top = top_ui + 10;
  const int my_bot = 12;
  int x = mx;
  int y = my_top;
  if (idx == 1) { x = tft.width() - mx; y = my_top; }       // TR
  else if (idx == 2) { x = tft.width() - mx; y = tft.height() - my_bot; } // BR
  else if (idx == 3) { x = mx; y = tft.height() - my_bot; } // BL
  *tx = x;
  *ty = y;
}

void drawCalTarget(CalState s) {
  drawBanner();
  int idx = (int)s - 1;
  int tx = 0, ty = 0;
  calTargetForIndex(idx, &tx, &ty);

  tft.drawLine(tx - 10, ty, tx + 10, ty, 0xFFFF);
  tft.drawLine(tx, ty - 10, tx, ty + 10, 0xFFFF);
  tft.drawCircle(tx, ty, 14, 0xF800);

  tft.setTextColor(0xFFFF, 0x0000);
  tft.setTextSize(2);
  tft.setCursor(10, 74);
  if (s == CAL_TL) tft.print("Touch TOP-LEFT");
  if (s == CAL_TR) tft.print("Touch TOP-RIGHT");
  if (s == CAL_BR) tft.print("Touch BOTTOM-RIGHT");
  if (s == CAL_BL) tft.print("Touch BOTTOM-LEFT");
}

void calStart() {
  cal_state = CAL_TL;
  cal_last_capture_ms = 0;
  cal_last_log_ms = 0;
  drawCalTarget(cal_state);
  Serial.println("cal: start (touch each target; light touch is OK)");
  Serial.print("cal: capture z threshold="); Serial.println(CAL_MINZ);
}

void calFinish() {
  int left_x = (cal_raw_x[0] + cal_raw_x[3]) / 2;
  int right_x = (cal_raw_x[1] + cal_raw_x[2]) / 2;
  int top_y = (cal_raw_y[0] + cal_raw_y[1]) / 2;
  int bottom_y = (cal_raw_y[2] + cal_raw_y[3]) / 2;

  MAP_FLIP_X = (left_x > right_x);
  MAP_FLIP_Y = (top_y > bottom_y);

  RAW_X_MIN = (left_x < right_x) ? left_x : right_x;
  RAW_X_MAX = (left_x < right_x) ? right_x : left_x;
  RAW_Y_MIN = (top_y < bottom_y) ? top_y : bottom_y;
  RAW_Y_MAX = (top_y < bottom_y) ? bottom_y : top_y;

  // Map captured raw corners to the full drawable pane.
  // Cal targets are inset for usability, but output should cover full paint area.
  MAP_OUT_X_MIN = 0;
  MAP_OUT_X_MAX = tft.width() - 1;
  MAP_OUT_Y_MIN = 60;
  MAP_OUT_Y_MAX = tft.height() - 1;
  recomputeAffineFromCalCorners();

  cal_state = CAL_OFF;
  drawBanner();
  Serial.println("cal: complete");
  printCal();
}

void maybeCaptureCalibration(const TSPoint &p) {
  if (cal_state == CAL_OFF) return;
  if (!touchPressureValid(p)) return;
  if (p.z <= CAL_MINZ) return;
  if (millis() - cal_last_capture_ms < 120) return;

  int idx = (int)cal_state - 1;
  cal_raw_x[idx] = logicalRawX(p);
  cal_raw_y[idx] = logicalRawY(p);
  cal_last_capture_ms = millis();

  Serial.print("cal: captured ");
  Serial.print(idx);
  Serial.print(" rawx=");
  Serial.print(cal_raw_x[idx]);
  Serial.print(" rawy=");
  Serial.println(cal_raw_y[idx]);

  if (cal_state == CAL_TL) cal_state = CAL_TR;
  else if (cal_state == CAL_TR) cal_state = CAL_BR;
  else if (cal_state == CAL_BR) cal_state = CAL_BL;
  else cal_state = CAL_OFF;

  if (cal_state == CAL_OFF) {
    calFinish();
  } else {
    drawCalTarget(cal_state);
  }
}

void flappySpawnPipe() {
  pipe_x = tft.width() + 10;
  gap_y = random(70, tft.height() - 70 - gap_h);
  pipe_passed = false;
}

void flappyReset() {
  bird_y = (float)(tft.height() / 2);
  bird_v = 0.0f;
  score = 0;
  flappy_game_over = false;
  flappySpawnPipe();
  flappy_last_ms = 0;
}

void flappyDraw() {
  tft.fillScreen(0x0010);
  tft.fillRect(0, 0, tft.width(), 22, 0x0000);
  tft.setTextColor(0xFFFF, 0x0000);
  tft.setTextSize(2);
  tft.setCursor(4, 4);
  tft.print("Flappy score:");
  tft.print(score);

  int ground_h = 10;
  tft.fillRect(0, tft.height() - ground_h, tft.width(), ground_h, 0x07E0);

  int top_h = gap_y;
  int bot_y = gap_y + gap_h;
  int bot_h = tft.height() - ground_h - bot_y;
  if (pipe_x + pipe_w > 0 && pipe_x < tft.width()) {
    tft.fillRect(pipe_x, 22, pipe_w, top_h - 22, 0x07E0);
    tft.fillRect(pipe_x, bot_y, pipe_w, bot_h, 0x07E0);
  }

  tft.fillCircle(bird_x, (int)bird_y, 8, 0xFFE0);

  if (flappy_game_over) {
    tft.fillRect(35, tft.height() / 2 - 22, tft.width() - 70, 44, 0x7800);
    tft.setTextColor(0xFFFF, 0x7800);
    tft.setCursor(48, tft.height() / 2 - 6);
    tft.print("GAME OVER");
  }
}

void flappyUpdate(bool pressed, bool just_pressed) {
  unsigned long now = millis();
  if (flappy_last_ms == 0) flappy_last_ms = now;
  if (now - flappy_last_ms < 33) return;
  flappy_last_ms = now;

  if (flappy_game_over) {
    if (just_pressed) flappyReset();
    flappyDraw();
    return;
  }

  if (just_pressed) bird_v = -4.2f;

  bird_v += 0.24f;
  if (bird_v > 5.5f) bird_v = 5.5f;
  bird_y += bird_v;

  pipe_x -= 3;
  if (!pipe_passed && (pipe_x + pipe_w) < bird_x) {
    score++;
    pipe_passed = true;
  }
  if (pipe_x + pipe_w < 0) {
    flappySpawnPipe();
  }

  int ground_y = tft.height() - 10;
  bool hit_ground = ((int)bird_y + 8 >= ground_y);
  bool hit_ceiling = ((int)bird_y - 8 <= 22);

  bool in_pipe_x = (bird_x + 8 >= pipe_x) && (bird_x - 8 <= pipe_x + pipe_w);
  bool in_gap = ((int)bird_y - 8 >= gap_y) && ((int)bird_y + 8 <= gap_y + gap_h);
  bool hit_pipe = in_pipe_x && !in_gap;

  if (hit_ground || hit_ceiling || hit_pipe) {
    flappy_game_over = true;
  }

  flappyDraw();
}

void handleCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();
  if (cmd.length() == 0) return;

  if (cmd == "help") {
    shellHelp();
    return;
  }
  if (cmd == "cls") {
    flappy_mode = false;
    drawBanner();
    Serial.println("ok: screen cleared");
    return;
  }
  if (cmd == "id") {
    Serial.print("lcd id: 0x");
    Serial.println(lcd_id, HEX);
    return;
  }
  if (cmd == "paint on") {
    flappy_mode = false;
    paint_mode = true;
    Serial.println("ok: paint on");
    return;
  }
  if (cmd == "paint off") {
    paint_mode = false;
    Serial.println("ok: paint off");
    return;
  }
  if (cmd == "touch") {
    TSPoint p = sampleTouch();
    printTouchSample(p);
    return;
  }
  if (cmd == "touchscan") {
    TSPoint p = sampleTouch();
    printTouchSample(p);
    Serial.println("note: map locked to map1(6,A1,A2,7)");
    return;
  }
  if (cmd.startsWith("touchmap ")) {
    Serial.println("ok: touchmap locked to map1(6,A1,A2,7)");
    return;
  }
  if (cmd == "touchwatch on") {
    touchwatch = true;
    Serial.println("ok: touchwatch on");
    return;
  }
  if (cmd == "touchwatch off") {
    touchwatch = false;
    Serial.println("ok: touchwatch off");
    return;
  }
  if (cmd.startsWith("z ")) {
    int v = cmd.substring(2).toInt();
    TS_MINZ = clampi(v, 1, 1023);
    if (TS_MAXZ <= TS_MINZ) TS_MAXZ = TS_MINZ + 1;
    Serial.print("ok: minz=");
    Serial.println(TS_MINZ);
    return;
  }
  if (cmd.startsWith("zmax ")) {
    int v = cmd.substring(5).toInt();
    TS_MAXZ = clampi(v, TS_MINZ + 1, 1200);
    Serial.print("ok: maxz=");
    Serial.println(TS_MAXZ);
    return;
  }
  if (cmd == "cal start") {
    flappy_mode = false;
    paint_mode = false;
    calStart();
    return;
  }
  if (cmd == "cal show") {
    printCal();
    return;
  }
  if (cmd == "cal reset") {
    RAW_X_MIN = 280; RAW_X_MAX = 840;
    RAW_Y_MIN = 141; RAW_Y_MAX = 923;
    MAP_SWAP_XY = false;
    MAP_FLIP_X = false; MAP_FLIP_Y = false;
    MAP_OUT_X_MIN = 0; MAP_OUT_X_MAX = tft.width() - 1;
    MAP_OUT_Y_MIN = 60; MAP_OUT_Y_MAX = tft.height() - 1;
    recomputeAffineFromCalCorners();
    Serial.println("cal: reset to defaults");
    printCal();
    return;
  }
  if (cmd == "flipx on") { MAP_FLIP_X = true; Serial.println("ok: flipx on"); return; }
  if (cmd == "flipx off") { MAP_FLIP_X = false; Serial.println("ok: flipx off"); return; }
  if (cmd == "flipy on") { MAP_FLIP_Y = true; Serial.println("ok: flipy on"); return; }
  if (cmd == "flipy off") { MAP_FLIP_Y = false; Serial.println("ok: flipy off"); return; }
  if (cmd == "swapxy on") { MAP_SWAP_XY = true; Serial.println("ok: swapxy on"); return; }
  if (cmd == "swapxy off") { MAP_SWAP_XY = false; Serial.println("ok: swapxy off"); return; }
  if (cmd == "lut on") { MAP_USE_LUT = true; Serial.println("ok: lut on"); return; }
  if (cmd == "lut off") { MAP_USE_LUT = false; Serial.println("ok: lut off"); return; }
  if (cmd == "flappy on") {
    paint_mode = false;
    flappy_mode = true;
    flappyReset();
    flappyDraw();
    Serial.println("ok: flappy on (touch to flap)");
    return;
  }
  if (cmd == "flappy off") {
    flappy_mode = false;
    drawBanner();
    Serial.println("ok: flappy off");
    return;
  }
  if (cmd == "fill white") { tft.fillRect(0, 60, tft.width(), tft.height() - 60, 0xFFFF); Serial.println("ok: fill white"); return; }
  if (cmd == "fill black") { tft.fillRect(0, 60, tft.width(), tft.height() - 60, 0x0000); Serial.println("ok: fill black"); return; }
  if (cmd == "fill red")   { tft.fillRect(0, 60, tft.width(), tft.height() - 60, 0xF800); Serial.println("ok: fill red"); return; }
  if (cmd == "fill green") { tft.fillRect(0, 60, tft.width(), tft.height() - 60, 0x07E0); Serial.println("ok: fill green"); return; }
  if (cmd == "fill blue")  { tft.fillRect(0, 60, tft.width(), tft.height() - 60, 0x001F); Serial.println("ok: fill blue"); return; }
  if (cmd == "fill yellow"){ tft.fillRect(0, 60, tft.width(), tft.height() - 60, 0xFFE0); Serial.println("ok: fill yellow"); return; }
  if (cmd == "color white") { paint_color = 0xFFFF; Serial.println("ok: color white"); return; }
  if (cmd == "color red")   { paint_color = 0xF800; Serial.println("ok: color red"); return; }
  if (cmd == "color green") { paint_color = 0x07E0; Serial.println("ok: color green"); return; }
  if (cmd == "color blue")  { paint_color = 0x001F; Serial.println("ok: color blue"); return; }
  if (cmd == "color yellow"){ paint_color = 0xFFE0; Serial.println("ok: color yellow"); return; }

  Serial.print("err: unknown command: ");
  Serial.println(cmd);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}

  lcd_id = tft.readID();
  if (lcd_id == 0xD3D3 || lcd_id == 0xFFFF || lcd_id == 0x0000 || lcd_id == 0x0101) {
    lcd_id = 0x9486;
  }

  tft.begin(lcd_id);
  tft.setRotation(1);
  MAP_OUT_X_MIN = 0;
  MAP_OUT_X_MAX = tft.width() - 1;
  MAP_OUT_Y_MIN = 60;
  MAP_OUT_Y_MAX = tft.height() - 1;
  recomputeAffineFromCalCorners();
  randomSeed(micros());
  drawBanner();

  Serial.println("Basalt UNO R4 shell ready. Type 'help'.");
  Serial.print("LCD ID = 0x");
  Serial.println(lcd_id, HEX);
  Serial.print("Touch map default: ");
  Serial.print(touch_map_idx);
  Serial.print(" ");
  Serial.println(touch_profiles[touch_map_idx].name);
  Serial.println("Tip: cal start, then paint on, then flappy on");
}

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      handleCommand(line);
      line = "";
    } else {
      if (line.length() < 120) line += c;
    }
  }

  TSPoint p = sampleTouch();
  bool pressed = (p.z > TS_MINZ) && (p.z < TS_MAXZ);
  if (!touchPressureValid(p)) pressed = false;

  int rx = logicalRawX(p);
  int ry = logicalRawY(p);
  // Temporary: do not reject by raw bounds while we validate mapper on-device.
  if (touchwatch && (millis() - last_touch_log_ms) >= 80) {
    last_touch_log_ms = millis();
    printTouchSample(p);
  }

  maybeCaptureCalibration(p);
  if (cal_state != CAL_OFF && (millis() - cal_last_log_ms) >= 120) {
    cal_last_log_ms = millis();
    Serial.print("cal: waiting z=");
    Serial.print(p.z);
    Serial.print(" rawx=");
    Serial.print(logicalRawX(p));
    Serial.print(" rawy=");
    Serial.println(logicalRawY(p));
  }
  if (cal_state != CAL_OFF) return;

  static bool prev_pressed = false;
  bool just_pressed = pressed && !prev_pressed;
  prev_pressed = pressed;

  if (flappy_mode) {
    flappyUpdate(pressed, just_pressed);
    return;
  }

  if (!paint_mode) {
    paint_has_prev = false;
    stable_press_count = 0;
    return;
  }
  if (!pressed) {
    paint_has_prev = false;
    stable_press_count = 0;
    return;
  }

  int x = 0;
  int y = 0;
  mapRawToScreen(rx, ry, &x, &y);
  if (y > 60) {
    if (stable_press_count < 2) {
      stable_press_count++;
      return;
    }
    if (!paint_has_prev) {
      tft.fillCircle(x, y, 3, paint_color);
      paint_has_prev = true;
      paint_prev_x = x;
      paint_prev_y = y;
    } else {
      tft.drawLine(paint_prev_x, paint_prev_y, x, y, paint_color);
      tft.fillCircle(x, y, 2, paint_color);
      paint_prev_x = x;
      paint_prev_y = y;
    }
  }
}
