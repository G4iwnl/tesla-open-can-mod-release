#pragma once

// Shared sketch configuration for Arduino IDE and PlatformIO.
// Arduino IDE users should open RP2040CAN/RP2040CAN.ino.
// PlatformIO users should edit this file from the repository root.

// ── BOARD SELECTION ──────────────────────────────────────────────
// Uncomment ONE of the following lines to match your board:
#define DRIVER_MCP2515   // Adafruit Feather RP2040 CAN (MCP2515 over SPI)
// #define DRIVER_SAME51    // Adafruit Feather M4 CAN Express (native ATSAME51 CAN)
// #define DRIVER_TWAI // ESP32 boards with built-in TWAI (CAN) peripheral

// ── VEHICLE HARDWARE SELECTION ───────────────────────────────────
// Uncomment ONE of the following lines to match your vehicle:
// #define LEGACY // HW3-retrofit
// #define HW3 // HW3
#define HW4 // HW4

// ── BEHAVIOUR OPTIONS ────────────────────────────────────────────
// Uncomment any of the following lines:
// #define ISA_SPEED_CHIME_SUPPRESS // Suppress ISA speed chime; speed limit sign will be empty while driving
#define EMERGENCY_VEHICLE_DETECTION // Enable emergency vehicle detection
#define BYPASS_TLSSC_REQUIREMENT // Always enable FSD without requiring "Traffic Light and Stop Sign Control" toggle
// #define NAG_KILLER // Suppress Autosteer "hands on wheel" nag (CAN 880 counter+1 echo, X179 pin 2/3)
// #define ENHANCED_AUTOPILOT          // Enable UI_applyEceR79 override on HW3/HW4 and summon on HW4
