#pragma once

// Subset of HID keycode definitions extracted from TinyUSB's class/hid/hid.h.
// The values follow the USB HID Usage Tables (Version 1.12, Table 12).

#ifndef TU_BIT
#define TU_BIT(n) (1u << (n))
#endif

// Alphabet keys
#ifndef HID_KEY_A
#define HID_KEY_A 0x04
#endif
#ifndef HID_KEY_B
#define HID_KEY_B 0x05
#endif
#ifndef HID_KEY_C
#define HID_KEY_C 0x06
#endif
#ifndef HID_KEY_D
#define HID_KEY_D 0x07
#endif
#ifndef HID_KEY_E
#define HID_KEY_E 0x08
#endif
#ifndef HID_KEY_F
#define HID_KEY_F 0x09
#endif
#ifndef HID_KEY_G
#define HID_KEY_G 0x0A
#endif
#ifndef HID_KEY_H
#define HID_KEY_H 0x0B
#endif
#ifndef HID_KEY_I
#define HID_KEY_I 0x0C
#endif
#ifndef HID_KEY_J
#define HID_KEY_J 0x0D
#endif
#ifndef HID_KEY_K
#define HID_KEY_K 0x0E
#endif
#ifndef HID_KEY_L
#define HID_KEY_L 0x0F
#endif
#ifndef HID_KEY_M
#define HID_KEY_M 0x10
#endif
#ifndef HID_KEY_N
#define HID_KEY_N 0x11
#endif
#ifndef HID_KEY_O
#define HID_KEY_O 0x12
#endif
#ifndef HID_KEY_P
#define HID_KEY_P 0x13
#endif
#ifndef HID_KEY_Q
#define HID_KEY_Q 0x14
#endif
#ifndef HID_KEY_R
#define HID_KEY_R 0x15
#endif
#ifndef HID_KEY_S
#define HID_KEY_S 0x16
#endif
#ifndef HID_KEY_T
#define HID_KEY_T 0x17
#endif
#ifndef HID_KEY_U
#define HID_KEY_U 0x18
#endif
#ifndef HID_KEY_V
#define HID_KEY_V 0x19
#endif
#ifndef HID_KEY_W
#define HID_KEY_W 0x1A
#endif
#ifndef HID_KEY_X
#define HID_KEY_X 0x1B
#endif
#ifndef HID_KEY_Y
#define HID_KEY_Y 0x1C
#endif
#ifndef HID_KEY_Z
#define HID_KEY_Z 0x1D
#endif

// Number row
#ifndef HID_KEY_1
#define HID_KEY_1 0x1E
#endif
#ifndef HID_KEY_2
#define HID_KEY_2 0x1F
#endif
#ifndef HID_KEY_3
#define HID_KEY_3 0x20
#endif
#ifndef HID_KEY_4
#define HID_KEY_4 0x21
#endif
#ifndef HID_KEY_5
#define HID_KEY_5 0x22
#endif
#ifndef HID_KEY_6
#define HID_KEY_6 0x23
#endif
#ifndef HID_KEY_7
#define HID_KEY_7 0x24
#endif
#ifndef HID_KEY_8
#define HID_KEY_8 0x25
#endif
#ifndef HID_KEY_9
#define HID_KEY_9 0x26
#endif
#ifndef HID_KEY_0
#define HID_KEY_0 0x27
#endif

// Control and special keys
#ifndef HID_KEY_ENTER
#define HID_KEY_ENTER 0x28
#endif
#ifndef HID_KEY_ESCAPE
#define HID_KEY_ESCAPE 0x29
#endif
#ifndef HID_KEY_BACKSPACE
#define HID_KEY_BACKSPACE 0x2A
#endif
#ifndef HID_KEY_TAB
#define HID_KEY_TAB 0x2B
#endif
#ifndef HID_KEY_SPACE
#define HID_KEY_SPACE 0x2C
#endif
#ifndef HID_KEY_SPACEBAR
#define HID_KEY_SPACEBAR HID_KEY_SPACE
#endif
#ifndef HID_KEY_MINUS
#define HID_KEY_MINUS 0x2D
#endif
#ifndef HID_KEY_EQUAL
#define HID_KEY_EQUAL 0x2E
#endif
#ifndef HID_KEY_LEFT_BRACKET
#define HID_KEY_LEFT_BRACKET 0x2F
#endif
#ifndef HID_KEY_RIGHT_BRACKET
#define HID_KEY_RIGHT_BRACKET 0x30
#endif
#ifndef HID_KEY_BACKSLASH
#define HID_KEY_BACKSLASH 0x31
#endif
#ifndef HID_KEY_EUROPE_1
#define HID_KEY_EUROPE_1 0x32
#endif
#ifndef HID_KEY_SEMICOLON
#define HID_KEY_SEMICOLON 0x33
#endif
#ifndef HID_KEY_APOSTROPHE
#define HID_KEY_APOSTROPHE 0x34
#endif
#ifndef HID_KEY_GRAVE
#define HID_KEY_GRAVE 0x35
#endif
#ifndef HID_KEY_COMMA
#define HID_KEY_COMMA 0x36
#endif
#ifndef HID_KEY_PERIOD
#define HID_KEY_PERIOD 0x37
#endif
#ifndef HID_KEY_SLASH
#define HID_KEY_SLASH 0x38
#endif
#ifndef HID_KEY_CAPS_LOCK
#define HID_KEY_CAPS_LOCK 0x39
#endif
#ifndef HID_KEY_NON_US_BACKSLASH
#define HID_KEY_NON_US_BACKSLASH 0x64
#endif

// Modifier masks
#ifndef KEYBOARD_MODIFIER_LEFTCTRL
#define KEYBOARD_MODIFIER_LEFTCTRL TU_BIT(0)
#endif
#ifndef KEYBOARD_MODIFIER_LEFTSHIFT
#define KEYBOARD_MODIFIER_LEFTSHIFT TU_BIT(1)
#endif
#ifndef KEYBOARD_MODIFIER_LEFTALT
#define KEYBOARD_MODIFIER_LEFTALT TU_BIT(2)
#endif
#ifndef KEYBOARD_MODIFIER_LEFTGUI
#define KEYBOARD_MODIFIER_LEFTGUI TU_BIT(3)
#endif
#ifndef KEYBOARD_MODIFIER_RIGHTCTRL
#define KEYBOARD_MODIFIER_RIGHTCTRL TU_BIT(4)
#endif
#ifndef KEYBOARD_MODIFIER_RIGHTSHIFT
#define KEYBOARD_MODIFIER_RIGHTSHIFT TU_BIT(5)
#endif
#ifndef KEYBOARD_MODIFIER_RIGHTALT
#define KEYBOARD_MODIFIER_RIGHTALT TU_BIT(6)
#endif
#ifndef KEYBOARD_MODIFIER_RIGHTGUI
#define KEYBOARD_MODIFIER_RIGHTGUI TU_BIT(7)
#endif

