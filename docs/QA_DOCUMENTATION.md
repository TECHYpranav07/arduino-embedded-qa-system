# Quality Assurance (QA) & Issue Tracking Documentation

**Course**: Project Management (`2307476T`)  
**Department**: Electronics & Telecommunication Engineering, MIT Academy of Engineering, Pune  
**Project**: Smart Multi-Mode LED & Status Controller for Embedded Systems  
**Author**: Pranav Karande (B.Tech E&TC, Div C)  
**Target Milestone**: `v1.0.0-Stable-Release`

---

## 1. Executive Summary & QA Lifecycle

In embedded systems and cross-domain electronics projects, software and hardware boundary defects represent a high percentage of field failures. This QA documentation details the systematic identification, root cause analysis, branch-based resolution, and test validation of 4 major engineering defects across timing, electrical, signal integrity, and telemetry domains.

```
+-----------------------------------------------------------------------------------+
|                              GITHUB QA WORKFLOW                                   |
|                                                                                   |
|  [Lab QA Audit] -> [Log Issue & Severity] -> [Root Cause Analysis (5-Why/Fishbone)] |
|                                                    |                              |
|  [Auto-Close Issue] <- [PR Review & Merge] <- [Branch & Fix] <- [Test Evidence]  |
+-----------------------------------------------------------------------------------+
```

---

## 2. QA Traceability & Resolution Matrix

| Issue ID | Severity | Component | Problem Summary | RCA Tool | Resolution Strategy | Status | PR / Commit |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **[QA-01](#qa-01)** | `CRITICAL` | Core / State Machine | Blocking `delay()` freezes MCU execution & drops button presses | **5-Why Analysis** | Asynchronous `millis()` delta-time engine | **RESOLVED** | PR #5 (`09c219f`) |
| **[QA-02](#qa-02)** | `HIGH` | GPIO / Input Driver | Switch contact bounce & floating pin causes spurious triggers | **Fishbone Diagram** | `INPUT_PULLUP` + 50ms software debounce filter | **RESOLVED** | PR #6 (`c68e1e9`) |
| **[QA-03](#qa-03)** | `HIGH` | Power / Output Driver | Unclamped 100% duty cycle LED driving risks GPIO thermal overload | **5-Why Analysis** | PWM duty cycle clamping (`MAX_PWM=150`) & soft-fading | **RESOLVED** | PR #7 (`b5eeb53`) |
| **[QA-04](#qa-04)** | `MEDIUM` | Telemetry / UART | Unregulated `Serial.print()` in `loop()` floods TX ring buffer | **Fishbone Diagram** | Non-blocking 500ms periodic heartbeat scheduler | **RESOLVED** | PR #8 (`9b664c1`) |

---

## 3. Deep-Dive Defect Analysis & Root Cause Analyses

### QA-01: Blocking `delay()` Freezes MCU Execution

* **GitHub Issue Link**: [#1 - Critical Timing Defect](https://github.com/TECHYpranav07/arduino-embedded-qa-system/issues/1)
* **Severity**: Critical (Severity-1)
* **Symptoms**: When running Mode 2 (1000ms blink) or Mode 3 (200ms blink), button inputs on Pin 2 are completely ignored unless held down continuously across the boundary of the delay.
* **Failure Rate**: 87.5% of short taps dropped in baseline test bench.

#### 5-Why Root Cause Analysis:
1. *Why does the device fail to register button clicks?*  
   Because digital Pin 2 is not read during the LED flashing cycles.
2. *Why is Pin 2 not read?*  
   Because the CPU execution pointer is halted inside synchronous `delay(1000)` busy-wait loops.
3. *Why was synchronous `delay()` utilized?*  
   Because the initial prototype was drafted procedurally without asynchronous multi-tasking consideration.
4. *Why was procedural design adopted?*  
   Because early firmware prototypes focused solely on visual verification rather than concurrent I/O performance.
5. *Why was concurrency not built in?*  
   **ROOT CAUSE**: Absence of a **non-blocking cooperative state machine architecture** utilizing the hardware timer (`millis()`).

#### Solution & Verification:
- Migrated timing logic from synchronous delay to cooperative timestamp comparison:
  ```cpp
  if (currentMillis - previousBlinkMillis >= 1000) {
      previousBlinkMillis = currentMillis;
      ledOutputState = !ledOutputState;
  }
  ```
- **Test Evidence**: Input response latency reduced from ~2000 ms to **< 2.5 ms**. In 100 consecutive button tap tests, 100% of presses were registered.

---

### QA-02: Switch Contact Bounce & Floating Input Pin

* **GitHub Issue Link**: [#2 - Input Instability Defect](https://github.com/TECHYpranav07/arduino-embedded-qa-system/issues/2)
* **Severity**: High (Severity-2)
* **Symptoms**: Tapping the mechanical button once causes multiple rapid mode jumps (e.g. jumping from Mode 0 straight to Mode 3).

#### Fishbone (Ishikawa) Root Cause Analysis:
```
                       CAUSE                                              EFFECT
 ---------------------------------------------------+
 Hardware / Components        Firmware / Software   |
   * Mechanical spring           * No debounce      |
     chatter (5-20ms)              filter window    |
   * Floating GPIO pin           * Polling raw pin  |
     (missing pull-up)             instantaneously  |
          \                             /           |
           \                           /            |
            +-------------------------+             +---> SPURIOUS MULTI-TRIGGERING
           /                           \            |     AND UNSTABLE MODE JUMPS
          /                             \           |
 Environment / Assembly       Process / Design      |
   * High EMI pickup             * Direct edge      |
     on breadboard wire            detection        |
   * Lack of bypass cap            without time     |
     across switch contacts        hysteresis       |
 ---------------------------------------------------+
```
* **Root Causes**:
  1. *Hardware Level*: Digital Pin 2 configured as floating `INPUT` with high impedance (>100MΩ) picking up 50Hz environmental noise.
  2. *Software Level*: Absence of a temporal debounce filter rejecting physical contact bouncing (12-16ms duration).

#### Solution & Verification:
- Activated internal pull-up: `pinMode(BUTTON_PIN, INPUT_PULLUP)`.
- Implemented software debounce filter with `DEBOUNCE_DELAY_MS = 50` and falling-edge detection (`HIGH -> LOW`).
- **Test Evidence**: Digital storage oscilloscope (DSO) confirmed all chatter spikes within 15ms window were completely rejected. 50 consecutive button presses produced exactly 50 single-step mode transitions.

---

### QA-03: Overcurrent Risk & Thermal Junction Overload

* **GitHub Issue Link**: [#3 - Hardware Stress & Reliability Defect](https://github.com/TECHYpranav07/arduino-embedded-qa-system/issues/3)
* **Severity**: High (Severity-2)
* **Symptoms**: Continuous full 100% duty cycle illumination in Mode 1 draws sustained 20.67 mA, leading to localized heating on the ATmega328P output driver.

#### 5-Why Root Cause Analysis:
1. *Why does the microcontroller pin run hot during extended operations?*  
   Because it is sourcing maximum saturation DC current continuously.
2. *Why is maximum DC current sustained?*  
   Because Pin 9 is driven constantly HIGH using binary `digitalWrite(LED_PIN, HIGH)`.
3. *Why was binary driving used on an LED pin?*  
   Because PWM modulation capability on hardware timer pin `OC1A` was neglected in the basic driver.
4. *Why was power dissipation overlooked?*  
   Because component reliability derating guidelines (50% derating for continuous operation) were not enforced during initial design.
5. *Why were derating rules omitted?*  
   **ROOT CAUSE**: Lack of **firmware-level power throttling and PWM duty cycle clamping**.

#### Solution & Verification:
- Replaced binary `digitalWrite` with PWM modulation: `analogWrite(LED_PIN, MAX_PWM_DUTY)`.
- Enforced hard limit: `MAX_PWM_DUTY = 150` (~58.8% duty cycle).
- Added soft-fading transitions in Mode 3 (breathing rate = 25ms steps).
- **Test Evidence**: Continuous DC current dropped from **20.67 mA to 12.1 mA** (41.5% reduction), keeping pin junction temperature within 2°C of ambient.

---

### QA-04: Telemetry Baud Rate Mismatch & UART Buffer Flood

* **GitHub Issue Link**: [#4 - Telemetry & Communication Defect](https://github.com/TECHYpranav07/arduino-embedded-qa-system/issues/4)
* **Severity**: Medium (Severity-3)
* **Symptoms**: Executing unthrottled `Serial.print()` in every `loop()` cycle saturated the 64-byte hardware TX buffer, dropping loop execution frequency from 48 kHz to 24 Hz.

#### Fishbone (Ishikawa) Root Cause Analysis:
```
                       CAUSE                                              EFFECT
 ---------------------------------------------------+
 Hardware / UART Specs         Firmware / Software   |
   * Fixed 64-byte ring          * Unthrottled calls |
     buffer in hardware            inside loop()     |
   * 9600 baud bandwidth         * No timestamp      |
     limitation (960 B/s)          rate limiter      |
          \                             /           |
           \                           /            |
            +-------------------------+             +---> UART BUFFER SATURATION &
           /                           \            |     CPU CORE EXECUTION STALL
          /                             \           |
 Host Diagnostics              Protocol Design      |
   * Serial monitor UI           * Repetitive raw    |
     freezes from flood            string logging    |
   * Missing backpressure        * Lack of periodic  |
     handling mechanism            heartbeat model   |
 ---------------------------------------------------+
```
* **Root Causes**:
  1. Serial data emitted at loop iteration rate (~20 microseconds) while UART baud rate (9600 bps) can only transmit 1 byte per 1040 microseconds.
  2. Arduino `HardwareSerial::write()` reverts to blocking busy-wait when the buffer is full.

#### Solution & Verification:
- Implemented non-blocking periodic scheduler: `TELEMETRY_INTERVAL_MS = 500` (2 Hz heartbeat).
- Formatted output as compact JSON: `{"ts":42500,"mode":2,"pwm":150,"btn":0,"loops":21450}`.
- **Test Evidence**: CPU loop frequency rebounded from **24 Hz to > 42,000 Hz**, with zero UART frame drops or console freezing over 30 minutes of logging.

---

## 4. Test Bench Verification & Comparative Metrics

| Operational Metric | Baseline Prototype (v0.1.0) | QA-Refactored Firmware (v1.0.0) | Engineering Improvement |
| :--- | :--- | :--- | :--- |
| **Input Response Latency** | 1000 ms – 2000 ms | **< 2.5 ms** | **99.8% Faster** |
| **Button Input Drop Rate** | 87.5% failure | **0.0% failure (100/100 passed)** | **Complete Elimination** |
| **Spurious Bounce Triggers** | 3 – 5 extra triggers / press | **0 extra triggers (50/50 passed)** | **100% Noise Rejection** |
| **Peak LED Current Draw** | 20.67 mA (full saturation) | **12.1 mA (throttled PWM)** | **41.5% Power Reduction** |
| **Loop Execution Frequency** | 24 Hz (stalled by UART) | **42,800 Hz (42.8 kHz)** | **178,200% Speedup** |
| **UART Buffer Overflows** | Constant (every loop) | **Zero (0 buffer overruns)** | **100% Telemetry Integrity** |

---

## 5. Conclusion & Verification Sign-Off
All 4 QA issues have been completely resolved, peer-reviewed via pull requests, merged into branch `main`, and tagged under Milestone `v1.0.0-Stable-Release`.
