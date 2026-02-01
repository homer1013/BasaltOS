#pragma once

#include "basalt_config.h"

// Default LED config (derived from board pins when available).
#ifndef BASALT_LED_R_PIN
#ifdef BASALT_PIN_LED_R
#define BASALT_LED_R_PIN BASALT_PIN_LED_R
#else
#define BASALT_LED_R_PIN -1
#endif
#endif

#ifndef BASALT_LED_G_PIN
#ifdef BASALT_PIN_LED_G
#define BASALT_LED_G_PIN BASALT_PIN_LED_G
#else
#define BASALT_LED_G_PIN -1
#endif
#endif

#ifndef BASALT_LED_B_PIN
#ifdef BASALT_PIN_LED_B
#define BASALT_LED_B_PIN BASALT_PIN_LED_B
#else
#define BASALT_LED_B_PIN -1
#endif
#endif

#ifndef BASALT_LED_ACTIVE_LOW
#define BASALT_LED_ACTIVE_LOW 0
#endif
