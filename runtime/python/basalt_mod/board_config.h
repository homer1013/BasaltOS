#pragma once

#include "basalt_config.h"

// Default LED config (derived from board pins when available).
#ifndef BASALT_LED_R_PIN
#ifdef BASALT_PIN_LED_R
#define BASALT_LED_R_PIN BASALT_PIN_LED_R
#elif defined(BASALT_PIN_LED) && !defined(BASALT_LED_MODE_SINGLE)
// Single-color boards map generic LED to all channels for compatibility.
#define BASALT_LED_R_PIN BASALT_PIN_LED
#else
#define BASALT_LED_R_PIN -1
#endif
#endif

#ifndef BASALT_LED_G_PIN
#ifdef BASALT_PIN_LED_G
#define BASALT_LED_G_PIN BASALT_PIN_LED_G
#elif defined(BASALT_PIN_LED)
// Single-color boards default to GREEN channel semantics.
#define BASALT_LED_G_PIN BASALT_PIN_LED
#else
#define BASALT_LED_G_PIN -1
#endif
#endif

#ifndef BASALT_LED_B_PIN
#ifdef BASALT_PIN_LED_B
#define BASALT_LED_B_PIN BASALT_PIN_LED_B
#elif defined(BASALT_PIN_LED) && !defined(BASALT_LED_MODE_SINGLE)
// Single-color boards map generic LED to all channels for compatibility.
#define BASALT_LED_B_PIN BASALT_PIN_LED
#else
#define BASALT_LED_B_PIN -1
#endif
#endif

#ifndef BASALT_LED_ACTIVE_LOW
#if defined(BASALT_BOARD_CYD)
// CYD onboard status LED is active-low.
#define BASALT_LED_ACTIVE_LOW 1
#else
#define BASALT_LED_ACTIVE_LOW 0
#endif
#endif
