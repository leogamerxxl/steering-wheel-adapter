# Steering Wheel Adapter — Renault Clio 5 → Dacia Logan I

> **Status: Ongoing — hardware calibration phase**

Wireless gateway that maps OEM Renault Clio 5 steering wheel controls to an aftermarket infotainment unit installed on a Dacia Logan I.

---

## Problem

The aftermarket head unit on the Logan I reads steering wheel button commands via two analog signal lines — **SW1** and **SW2**. These lines use a **resistor ladder network** referenced to **12V** (automotive system voltage): each button press connects a different resistor to GND, producing a unique voltage on SW1/SW2 that the head unit's ADC identifies as a specific command.

The Clio 5 steering wheel uses a different proprietary protocol — its buttons cannot be connected directly to the aftermarket unit.

---

## Solution Architecture

```
CLIO 5 STEERING WHEEL          LOGAN I — HEAD UNIT
┌─────────────────────┐        ┌──────────────────────┐
│  Physical buttons   │        │  Aftermarket HU      │
│  ┌───┐ ┌───┐ ┌───┐ │        │                      │
│  │ 1 │ │ 2 │ │ 3 │ │        │  SW1 ──── ADC input  │
│  └─┬─┘ └─┬─┘ └─┬─┘ │        │  SW2 ──── ADC input  │
│    └──────┴──────┘   │        │                      │
│         ↓            │        │  12V reference       │
│   ESP32 WROOM TX     │        │  resistor ladder     │
│   GPIO direct read   │        │                      │
│         ↓            │        │   ESP32 WROOM RX     │
│    ESP-NOW TX ───────┼────────┼──► ESP-NOW RX        │
│    2.4GHz wireless   │        │         ↓            │
└─────────────────────┘        │   NPN transistor     │
                                │   array              │
                                │         ↓            │
                                │   Calibrated         │
                                │   resistors R1..Rn   │
                                │         ↓            │
                                │   SW1 / SW2 pins ───►│
                                └──────────────────────┘
```

---

## SW1/SW2 Signal — How the Resistor Ladder Works

```
        12V (automotive)
            │
          R_ref
            │
            ├──────────── SW1 pin (ADC input on head unit)
            │
    ┌───────┼───────┐
    │       │       │
   R1      R2      R3  ...  (one per button)
    │       │       │
   SW1     SW2     SW3  (button switches)
    │       │       │
   GND     GND    GND
```

Each button pressed → unique voltage divider ratio → unique voltage on SW1/SW2 → head unit ADC identifies the command.

**Voltage levels:** referenced to 12V automotive system (measured at head unit connector with ignition on).

---

## ESP32 RX — Output Stage

The ESP32 operates at 3.3V logic and **cannot directly drive a 12V resistor ladder**. Each output channel uses an NPN transistor (BC547) to interface the 3.3V GPIO with the 12V automotive system.

```
ESP32 GPIO (3.3V)
        │
       1kΩ  (base resistor)
        │
      BC547 NPN
      Base ─────────────
      Collector ────── R_calibrated ──── SW1 pin (12V system)
      Emitter  ────── GND
```

**One transistor + one calibrated resistor per button.**

When ESP32 pulls GPIO HIGH → transistor conducts → resistor connects to SW1 → head unit reads correct voltage → executes command.

---

## Calibration — Next Step

> ⚠️ **Calibration required before resistor selection.**

Must measure actual voltages on SW1/SW2 at the head unit connector for each button press (ignition on, multimeter on DC voltage):

| Button | Expected SW1 (V) | R_calibrated (Ω) |
|--------|-----------------|-----------------|
| Volume + | — TBD — | — TBD — |
| Volume - | — TBD — | — TBD — |
| Next track | — TBD — | — TBD — |
| Prev track | — TBD — | — TBD — |
| Mute | — TBD — | — TBD — |
| Source | — TBD — | — TBD — |

**Resistor calculation:**

Once voltage targets are known:

```
V_sw = 12V × R_calibrated / (R_ref + R_calibrated)

→ R_calibrated = R_ref × V_sw / (12 - V_sw)
```

Where R_ref is the internal pull-up resistor inside the head unit (typically 1kΩ–10kΩ — measure or find in datasheet).

---

## Wiring Summary

### Transmitter — ESP32 on steering wheel

```
Button 1  → GPIO 4   (INPUT_PULLUP)
Button 2  → GPIO 5   (INPUT_PULLUP)
Button 3  → GPIO 18  (INPUT_PULLUP)
Button 4  → GPIO 19  (INPUT_PULLUP)
Button 5  → GPIO 21  (INPUT_PULLUP)
GND       → GND
12V car   → AMS1117-3.3V → ESP32 3.3V
```

### Receiver — ESP32 at head unit

```
GPIO 4  → 1kΩ → BC547 base → collector: R1_cal → SW1
GPIO 5  → 1kΩ → BC547 base → collector: R2_cal → SW1
GPIO 18 → 1kΩ → BC547 base → collector: R3_cal → SW1
GPIO 19 → 1kΩ → BC547 base → collector: R4_cal → SW2
GPIO 21 → 1kΩ → BC547 base → collector: R5_cal → SW2
All BC547 emitters → GND
12V car → AMS1117-3.3V → ESP32 3.3V
```

---



## Bill of Materials

| Component | Part | Qty | Notes |
|-----------|------|-----|-------|
| MCU transmitter | ESP32 WROOM-32 | 1 | On steering wheel |
| MCU receiver | ESP32 WROOM-32 | 1 | At head unit |
| Voltage regulator | AMS1117-3.3V | 2 | 12V → 3.3V per board |
| NPN transistor | BC547 | 5+ | One per button channel |
| Base resistor | 1kΩ | 5+ | ESP32 GPIO → BC547 base |
| Calibrated resistors | TBD after measurement | 5+ | Set correct SW1/SW2 voltage |
| Decoupling cap | 100nF | 4 | Power supply filtering |
| Bulk cap | 10µF | 2 | AMS1117 stability |

---

## Project Status

- [x] ESP-NOW communication — tested and working
- [x] Button GPIO reading on transmitter
- [x] Wireless latency verified (~1ms)
- [x] Architecture designed for SW1/SW2 resistor ladder
- [ ] SW1/SW2 voltage measurements per button — **next step**
- [ ] Resistor values calculation and selection
- [ ] Hardware assembly and testing on car
- [ ] Long-term reliability testing

---

## Author

**Cosmin Leonardo Cozaciuc**  
Electrical Engineering Student — Universitatea Politehnica București  
 · [LinkedIn](https://www.linkedin.com/in/leonardo-cozaciuc-1605901a0/)
