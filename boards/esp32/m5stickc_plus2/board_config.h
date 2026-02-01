#pragma once

// M5StickC Plus2 board-specific helpers.
// Keep this file small: the canonical pin map is in board.json -> generated basalt_config.h

// Power hold: set HIGH to keep device powered when not on USB.
#define BASALT_PWR_HOLD_PIN 4

// Display (ST7789V2)
#define BASALT_TFT_CS_PIN   5
#define BASALT_TFT_DC_PIN   14
#define BASALT_TFT_RST_PIN  12
#define BASALT_TFT_MOSI_PIN 15
#define BASALT_TFT_SCLK_PIN 13
#define BASALT_TFT_BL_PIN   27

// Onboard peripherals
#define BASALT_BUZZER_PIN   2
#define BASALT_IR_LED_PIN   19

// Buttons (input-only pins for A/B/C)
#define BASALT_BUTTON_A_PIN 37
#define BASALT_BUTTON_B_PIN 39
#define BASALT_BUTTON_C_PIN 35

// Battery sense (ADC)
#define BASALT_BAT_ADC_PIN  38
