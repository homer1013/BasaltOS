#include <Arduino.h>
#include <MCUFRIEND_kbv.h>

MCUFRIEND_kbv tft;
uint16_t lcd_id = 0x0000;

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

static void print_help(void) {
  Serial.println("commands: help, lcd id, fill red|green|blue|black|white");
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

  Serial.print("lcd id: 0x");
  Serial.println(lcd_id, HEX);
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
}
