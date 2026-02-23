#include <Arduino.h>
#include <EEPROM.h>
#include <MCUFRIEND_kbv.h>

MCUFRIEND_kbv tft;

static uint16_t lcd_id = 0x0000;

static int RAW_X_MIN = 280;
static int RAW_X_MAX = 840;
static int RAW_Y_MIN = 141;
static int RAW_Y_MAX = 923;
static bool MAP_SWAP_XY = false;
static bool MAP_FLIP_X = false;
static bool MAP_FLIP_Y = false;
static int TS_MINZ = 20;
static int TS_MAXZ = 1000;

struct CalPersist {
  uint32_t magic;
  uint16_t version;
  int16_t raw_x_min;
  int16_t raw_x_max;
  int16_t raw_y_min;
  int16_t raw_y_max;
  uint8_t map_swap_xy;
  uint8_t map_flip_x;
  uint8_t map_flip_y;
  uint16_t ts_minz;
  uint16_t ts_maxz;
};

static const uint32_t CAL_MAGIC = 0x4243414Cu;  // "BCAL"
static const uint16_t CAL_VERSION = 1;
static const int CAL_EEPROM_ADDR = 0;

static void draw_header(void) {
  tft.fillRect(0, 0, tft.width(), 24, 0x001F);
  tft.setTextColor(0xFFFF, 0x001F);
  tft.setTextSize(2);
  tft.setCursor(4, 4);
  tft.print("Basalt Uno R4 TFT");
}

static void draw_ready(void) {
  tft.setTextColor(0x07E0, 0x0000);
  tft.setTextSize(2);
  tft.setCursor(4, 34);
  tft.print("tft_parallel_uno ready");
  tft.setTextColor(0xFFE0, 0x0000);
  tft.setCursor(4, 54);
  tft.print("serial: help");
}

static void fill_body(uint16_t c) {
  tft.fillRect(0, 80, tft.width(), tft.height() - 80, c);
}

static void print_cal(void) {
  Serial.print("cal raw_x_min="); Serial.print(RAW_X_MIN);
  Serial.print(" raw_x_max="); Serial.print(RAW_X_MAX);
  Serial.print(" raw_y_min="); Serial.print(RAW_Y_MIN);
  Serial.print(" raw_y_max="); Serial.print(RAW_Y_MAX);
  Serial.print(" flip_x="); Serial.print(MAP_FLIP_X ? 1 : 0);
  Serial.print(" flip_y="); Serial.print(MAP_FLIP_Y ? 1 : 0);
  Serial.print(" swap_xy="); Serial.print(MAP_SWAP_XY ? 1 : 0);
  Serial.print(" minz="); Serial.print(TS_MINZ);
  Serial.print(" maxz="); Serial.println(TS_MAXZ);
}

static bool save_calibration(void) {
  CalPersist p{};
  p.magic = CAL_MAGIC;
  p.version = CAL_VERSION;
  p.raw_x_min = (int16_t)RAW_X_MIN;
  p.raw_x_max = (int16_t)RAW_X_MAX;
  p.raw_y_min = (int16_t)RAW_Y_MIN;
  p.raw_y_max = (int16_t)RAW_Y_MAX;
  p.map_swap_xy = MAP_SWAP_XY ? 1 : 0;
  p.map_flip_x = MAP_FLIP_X ? 1 : 0;
  p.map_flip_y = MAP_FLIP_Y ? 1 : 0;
  p.ts_minz = (uint16_t)TS_MINZ;
  p.ts_maxz = (uint16_t)TS_MAXZ;
  EEPROM.put(CAL_EEPROM_ADDR, p);
  return true;
}

static bool load_calibration(void) {
  CalPersist p{};
  EEPROM.get(CAL_EEPROM_ADDR, p);
  if (p.magic != CAL_MAGIC || p.version != CAL_VERSION) return false;
  if (p.raw_x_max <= p.raw_x_min || p.raw_y_max <= p.raw_y_min) return false;
  if ((p.raw_x_max - p.raw_x_min) < 8 || (p.raw_y_max - p.raw_y_min) < 8) return false;

  RAW_X_MIN = (int)p.raw_x_min;
  RAW_X_MAX = (int)p.raw_x_max;
  RAW_Y_MIN = (int)p.raw_y_min;
  RAW_Y_MAX = (int)p.raw_y_max;
  MAP_SWAP_XY = p.map_swap_xy != 0;
  MAP_FLIP_X = p.map_flip_x != 0;
  MAP_FLIP_Y = p.map_flip_y != 0;
  TS_MINZ = (int)p.ts_minz;
  TS_MAXZ = (int)p.ts_maxz;
  return true;
}

static void clear_calibration_store(void) {
  CalPersist p{};
  EEPROM.put(CAL_EEPROM_ADDR, p);
}

static void reset_calibration_defaults(void) {
  RAW_X_MIN = 280;
  RAW_X_MAX = 840;
  RAW_Y_MIN = 141;
  RAW_Y_MAX = 923;
  MAP_SWAP_XY = false;
  MAP_FLIP_X = false;
  MAP_FLIP_Y = false;
  TS_MINZ = 20;
  TS_MAXZ = 1000;
}

static void print_help(void) {
  Serial.println("commands: help, lcd id, fill red|green|blue|black|white");
  Serial.println("          cal show|reset|set x0 x1 y0 y1|save|load|clear");
}

static void handle_cmd(const String& in) {
  String cmd = in;
  cmd.trim();
  cmd.toLowerCase();
  if (cmd.length() == 0) return;

  if (cmd == "help") {
    print_help();
    return;
  }
  if (cmd == "lcd id") {
    Serial.print("lcd id: 0x");
    Serial.println(lcd_id, HEX);
    return;
  }
  if (cmd == "cal show") {
    print_cal();
    return;
  }
  if (cmd == "cal reset") {
    reset_calibration_defaults();
    Serial.println("ok: cal reset");
    print_cal();
    return;
  }
  if (cmd == "cal save") {
    if (save_calibration()) Serial.println("cal: saved");
    else Serial.println("cal: save failed");
    return;
  }
  if (cmd == "cal load") {
    if (load_calibration()) {
      Serial.println("cal: loaded");
      print_cal();
    } else {
      Serial.println("cal: not found");
    }
    return;
  }
  if (cmd == "cal clear") {
    clear_calibration_store();
    Serial.println("cal: cleared");
    return;
  }
  if (cmd.startsWith("cal set ")) {
    int x0 = 0, x1 = 0, y0 = 0, y1 = 0;
    int parsed = sscanf(cmd.c_str(), "cal set %d %d %d %d", &x0, &x1, &y0, &y1);
    if (parsed != 4 || x1 <= x0 || y1 <= y0) {
      Serial.println("err: usage cal set <raw_x_min> <raw_x_max> <raw_y_min> <raw_y_max>");
      return;
    }
    RAW_X_MIN = x0;
    RAW_X_MAX = x1;
    RAW_Y_MIN = y0;
    RAW_Y_MAX = y1;
    Serial.println("cal: set");
    print_cal();
    return;
  }
  if (cmd == "fill red") {
    fill_body(0xF800);
    Serial.println("ok: fill red");
    return;
  }
  if (cmd == "fill green") {
    fill_body(0x07E0);
    Serial.println("ok: fill green");
    return;
  }
  if (cmd == "fill blue") {
    fill_body(0x001F);
    Serial.println("ok: fill blue");
    return;
  }
  if (cmd == "fill black") {
    fill_body(0x0000);
    Serial.println("ok: fill black");
    return;
  }
  if (cmd == "fill white") {
    fill_body(0xFFFF);
    Serial.println("ok: fill white");
    return;
  }

  Serial.print("err: unknown command: ");
  Serial.println(cmd);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("boot: basalt uno r4 tft_parallel_uno");

  lcd_id = tft.readID();
  if (lcd_id == 0x0000 || lcd_id == 0xFFFF) lcd_id = 0x6814;
  tft.begin(lcd_id);
  tft.setRotation(1);
  tft.fillScreen(0x0000);
  draw_header();
  draw_ready();
  fill_body(0x0000);

  if (load_calibration()) {
    Serial.println("cal: loaded from eeprom");
  } else {
    Serial.println("cal: using defaults");
  }

  Serial.print("lcd id: 0x");
  Serial.println(lcd_id, HEX);
  print_cal();
  Serial.println("ready: help");
}

void loop() {
  static String line;
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (line.length() > 0) handle_cmd(line);
      line = "";
    } else if (line.length() < 96) {
      line += c;
    }
  }

  delay(2);
}
