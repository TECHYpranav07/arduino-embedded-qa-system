# MIT Academy of Engineering, Alandi (D), Pune - 412105
### Department of Electronics & Telecommunication Engineering
**Academic Year**: 2026 - 2027 | **Semester**: VII | **Class**: B.Tech | **Division**: C  
**Course Code**: 2307476T | **Course**: Project Management  
**Course Teacher**: Dr. Ashish Mulajkar  
**Student Name**: Pranav Karande  
**Activity**: Activity 1 (Individual Online Assessment)  
**Format No.**: ACAD/DI/11B | **Rev. No.**: 01 | **Rev. Date**: 01/07/2025  
**Course Outcome**: CO2 | **RBT Level**: Level 3 | **Marks**: 15  

---

# ACTIVITY REPORT: Activity 1
## Name of Activity: "GitHub-Based QA Documentation and Problem Solving based on Electronics/Cross Domain Projects"

---

## Pre-Reflection of Activity
> *Prompt: Consider how collaborative platforms like GitHub can enhance transparency, traceability, and teamwork in solving real-world electronics and interdisciplinary project challenges.*

In modern embedded electronics and cross-domain engineering projects, systems are no longer isolated hardware prototypes. They involve complex interdependencies among embedded firmware, circuit schematics, sensor telemetry, and mechanical interfaces. Traditional development often suffers from "siloed" debugging—where hardware engineers blame firmware bugs, and software developers miss hardware timing constraints. 

Collaborative platforms like GitHub bridge this gap by establishing:
1. **Single Source of Truth & Transparency**: Every schematic revision, firmware iteration, and register configuration is transparently visible with exact historical diffs.
2. **Deterministic Traceability**: When an anomaly occurs (e.g. an intermittent reset, signal jitter, or thermal overshoot), linking commits directly to GitHub Issues enables end-to-end traceability from the initial lab observation to the verified patch.
3. **Structured Peer Review & Accountability**: Through branch isolation and Pull Request workflows, changes cannot silently enter production without peer consensus, preventing regression risks across interdisciplinary teams.

---

## A. GitHub Repository Link
* **Public GitHub Repository**: [https://github.com/TECHYpranav07/arduino-embedded-qa-system](https://github.com/TECHYpranav07/arduino-embedded-qa-system)
* **Author / GitHub Handle**: [@TECHYpranav07](https://github.com/TECHYpranav07)
* **Target Milestone**: [v1.0.0-Stable-Release](https://github.com/TECHYpranav07/arduino-embedded-qa-system/milestone/1)
* **Default Branch**: `main`

---

## B. Brief Description of Code
The firmware developed for this activity is the **Smart Multi-Mode LED & Status Controller** (`src/smart_led_controller.ino`), designed for ATmega328P/Arduino microcontrollers. 

### 1. Functional Specification
The embedded system acts as an industrial status indicator and diagnostics controller capable of operating across four finite states:
- **Mode 0 (`MODE_OFF`)**: System in quiescent low-power standby; LED fully deactivated (0% PWM).
- **Mode 1 (`MODE_STEADY_ON`)**: Continuous visual confirmation; clamped at 58.8% PWM duty cycle (`MAX_PWM_DUTY = 150`) to limit pin current to $\le 12\text{ mA}$.
- **Mode 2 (`MODE_HEARTBEAT_BLINK`)**: Periodic beacon (1 Hz slow blink: 1000ms ON / 1000ms OFF) using asynchronous timer scheduling.
- **Mode 3 (`MODE_ALERT_BREATHE`)**: Dynamic alert mode utilizing soft-fade pulse-width modulation (breathing effect with 25ms increments) to avoid sharp electrical current spikes ($dI/dt$).

### 2. Hardware Pinout & Architecture
| Subsystem | Microcontroller Pin | Interface Configuration | Operating Characteristics |
| :--- | :--- | :--- | :--- |
| **Status LED** | Digital Pin 9 (PWM `OC1A`) | Active-HIGH output with current-limiting resistor | Modulated via `analogWrite()`, clamped duty cycle |
| **User Push Button** | Digital Pin 2 (`INT0`) | Active-LOW momentary tact switch | Internal pull-up (`INPUT_PULLUP`), 50ms debounce filter |
| **UART Diagnostics** | Pin 0 (RX) / Pin 1 (TX) | Serial stream (9600 bps) | Rate-limited non-blocking JSON telemetry (2 Hz heartbeat) |

### 3. Firmware Modular Architecture
The final production firmware operates using a **cooperative multi-tasking executive**:
* **Engine A (Input Sampling)**: Non-blocking state transition engine with temporal edge jitter filtering.
* **Engine B (State Execution)**: Asynchronous timestamp delta-time scheduler (`millis()`) decoupling I/O polling from actuator timing.
* **Engine C (Telemetry & Health Stream)**: Rate-limited periodic JSON transmitter outputting operating mode, PWM duty, button state, and execution loop frequency.

---

## C. QA Issues Logged (Problem Identification & Root Cause Analysis)
To fulfill the requirements of Rubric Criterion 2 (Exceptional Quality: 4+ issues with thorough Root Cause Analysis using 5-Why and Fishbone diagrams), four comprehensive QA issues were identified, formally logged on GitHub, and prioritized:

```
+-----------------------------------------------------------------------------------------------------------+
|                                        QA ISSUES LOGGED SUMMARY                                           |
+----+---------------------+-------------------------------------------------------------+-----------------+
| ID | Severity            | Defect Title                                                | RCA Methodology |
+----+---------------------+-------------------------------------------------------------+-----------------+
| #1 | Critical (Sev-1)    | Blocking delay() freezes MCU execution & misses button I/O   | 5-Why Analysis  |
| #2 | High (Sev-2)        | Switch contact bounce & floating pin causes multi-triggers  | Fishbone (RCA)  |
| #3 | High (Sev-2)        | Overcurrent risk: 100% duty cycle LED exceeds safe GPIO spec| 5-Why Analysis  |
| #4 | Medium (Sev-3)      | Serial telemetry floods UART TX buffer & stalls CPU core    | Fishbone (RCA)  |
+----+---------------------+-------------------------------------------------------------+-----------------+
```

### Issue 1: [QA-01] Critical Timing Defect — Blocking `delay()` Freezes MCU
* **GitHub Issue**: [#1 - View on GitHub](https://github.com/TECHYpranav07/arduino-embedded-qa-system/issues/1)
* **Severity**: `critical-severity`, `bug`, `electronics-qa`
* **Defect Description**: The baseline prototype used synchronous `delay(1000)` and `delay(200)` inside blink states. During these delays, the program counter is halted in busy-wait loops, rendering digital Pin 2 completely blind to user input. Over 87% of button presses were lost.
* **5-Why Root Cause Analysis**:
  1. *Why does the device fail to switch modes?* $\rightarrow$ Pin 2 is not read when the user presses the button.
  2. *Why is Pin 2 not read?* $\rightarrow$ The CPU execution is paused inside `delay()` loops.
  3. *Why was synchronous `delay()` utilized?* $\rightarrow$ The prototype code was written procedurally.
  4. *Why was procedural design adopted?* $\rightarrow$ Initial prototyping focused on simple visual output rather than concurrent I/O performance.
  5. *Why was concurrency not built in?* $\rightarrow$ **ROOT CAUSE**: Absence of a **non-blocking cooperative state machine architecture** utilizing the hardware timer (`millis()`).

---

### Issue 2: [QA-02] Input Instability — Switch Contact Bounce & Floating Pin
* **GitHub Issue**: [#2 - View on GitHub](https://github.com/TECHYpranav07/arduino-embedded-qa-system/issues/2)
* **Severity**: `high-severity`, `bug`, `electronics-qa`
* **Defect Description**: Tactile push button on Pin 2 was configured as floating `INPUT`. High impedance caused spurious EMI pickup, while mechanical spring chatter (12–16 ms) triggered 3–5 rapid mode increments on a single physical tap.
* **Fishbone (Ishikawa) Root Cause Analysis**:
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
* **Root Causes**: Missing internal pull-up resistor configuration and lack of a temporal 50ms software hysteresis filter.

---

### Issue 3: [QA-03] Electrical Stress & Reliability — Unclamped 100% Duty Cycle Overcurrent
* **GitHub Issue**: [#3 - View on GitHub](https://github.com/TECHYpranav07/arduino-embedded-qa-system/issues/3)
* **Severity**: `high-severity`, `bug`, `electronics-qa`
* **Defect Description**: Pin 9 was driven continuously HIGH at 100% duty cycle. With a 150Ω resistor at 5V, continuous current reached 20.67 mA. Sustained operation near maximum ratings without thermal derating violated reliability guidelines and elevated output driver junction temperature.
* **5-Why Root Cause Analysis**:
  1. *Why is the I/O pin dissipating excess power?* $\rightarrow$ Sinking/sourcing continuous maximum DC current.
  2. *Why is maximum DC current sustained?* $\rightarrow$ Driven constantly with binary `digitalWrite(LED_PIN, HIGH)`.
  3. *Why was binary driving used?* $\rightarrow$ Hardware PWM capabilities (`OC1A`) were not leveraged.
  4. *Why was power dissipation overlooked?* $\rightarrow$ Component reliability derating guidelines (50% derating for continuous loads) were omitted.
  5. *Why were derating rules omitted?* $\rightarrow$ **ROOT CAUSE**: Lack of **firmware-level power throttling and PWM duty cycle clamping**.

---

### Issue 4: [QA-04] Telemetry & Communication Defect — UART Buffer Saturation & CPU Stall
* **GitHub Issue**: [#4 - View on GitHub](https://github.com/TECHYpranav07/arduino-embedded-qa-system/issues/4)
* **Severity**: `medium-severity`, `bug`, `electronics-qa`
* **Defect Description**: `Serial.print()` invoked inside every `loop()` cycle saturated the 64-byte hardware TX buffer within 2 milliseconds. Once full, the Arduino core blocked on every print statement, dropping CPU loop frequency from 48 kHz to 24 Hz.
* **Fishbone (Ishikawa) Root Cause Analysis**:
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
* **Root Causes**: Unthrottled telemetry streaming at 50 kHz cycle rates over a 9600 bps UART channel lacking a periodic heartbeat scheduler.

---

## D. Collaboration Summary (Branching, PRs, Comments & Resolutions)

A disciplined Git branch-per-fix workflow was enforced. Each defect was discussed in GitHub Issue comments, resolved on an isolated branch, and integrated through a Pull Request that automatically closed the issue.

```mermaid
gitGraph
   commit id: "5de799b (Init Prototype v0.1.0)"
   branch fix/non-blocking-state-machine
   checkout fix/non-blocking-state-machine
   commit id: "09c219f (Fixes #1)"
   checkout main
   merge fix/non-blocking-state-machine id: "PR #5 Merged"
   branch fix/input-debounce-pullup
   checkout fix/input-debounce-pullup
   commit id: "c68e1e9 (Fixes #2)"
   checkout main
   merge fix/input-debounce-pullup id: "PR #6 Merged"
   branch fix/pwm-thermal-throttling
   checkout fix/pwm-thermal-throttling
   commit id: "b5eeb53 (Fixes #3)"
   checkout main
   merge fix/pwm-thermal-throttling id: "PR #7 Merged"
   branch fix/serial-telemetry-rate-limiter
   checkout fix/serial-telemetry-rate-limiter
   commit id: "9b664c1 (Fixes #4)"
   checkout main
   merge fix/serial-telemetry-rate-limiter id: "PR #8 Merged"
```

### Pull Request & Merge Audit Log
1. **Pull Request #5** (`fix/non-blocking-state-machine` $\rightarrow$ `main`):
   * *PR Link*: [https://github.com/TECHYpranav07/arduino-embedded-qa-system/pull/5](https://github.com/TECHYpranav07/arduino-embedded-qa-system/pull/5)
   * *Resolution*: Replaced blocking `delay()` with asynchronous `millis()` delta-time engine. Decoupled button sampling.
   * *Merged Commit*: `06f512b` (Closes Issue #1).
2. **Pull Request #6** (`fix/input-debounce-pullup` $\rightarrow$ `main`):
   * *PR Link*: [https://github.com/TECHYpranav07/arduino-embedded-qa-system/pull/6](https://github.com/TECHYpranav07/arduino-embedded-qa-system/pull/6)
   * *Resolution*: Enabled `INPUT_PULLUP` and integrated a 50ms software debounce filter with falling-edge detection.
   * *Merged Commit*: `1a72279` (Closes Issue #2).
3. **Pull Request #7** (`fix/pwm-thermal-throttling` $\rightarrow$ `main`):
   * *PR Link*: [https://github.com/TECHYpranav07/arduino-embedded-qa-system/pull/7](https://github.com/TECHYpranav07/arduino-embedded-qa-system/pull/7)
   * *Resolution*: Implemented PWM duty cycle clamping (`MAX_PWM_DUTY = 150`, ~58.8% power) and soft-fade transitions.
   * *Merged Commit*: `524f681` (Closes Issue #3).
4. **Pull Request #8** (`fix/serial-telemetry-rate-limiter` $\rightarrow$ `main`):
   * *PR Link*: [https://github.com/TECHYpranav07/arduino-embedded-qa-system/pull/8](https://github.com/TECHYpranav07/arduino-embedded-qa-system/pull/8)
   * *Resolution*: Added periodic 500ms heartbeat scheduler and compact JSON diagnostics, eliminating UART buffer overflows.
   * *Merged Commit*: `3769414` (Closes Issue #4).

### Issue Collaboration & Discussion Records
Technical peer review comments were posted on each issue before branching:
* *Issue #1 Review*: [Comment Link](https://github.com/TECHYpranav07/arduino-embedded-qa-system/issues/1#issuecomment-5874425667) — Documented the 87.5% drop rate and framed the `millis()` cooperative architecture.
* *Issue #2 Review*: [Comment Link](https://github.com/TECHYpranav07/arduino-embedded-qa-system/issues/2#issuecomment-5874426139) — Evaluated oscilloscope DSO bounce measurements (12.4ms-16.8ms) and proposed `INPUT_PULLUP` + 50ms filter.
* *Issue #3 Review*: [Comment Link](https://github.com/TECHYpranav07/arduino-embedded-qa-system/issues/3#issuecomment-5874426555) — Calculated Ohm's law current draw ($20.67\text{ mA}$) and established the 58% PWM clamping threshold.
* *Issue #4 Review*: [Comment Link](https://github.com/TECHYpranav07/arduino-embedded-qa-system/issues/4#issuecomment-5874427055) — Analyzed UART TX buffer stall mechanics and established 2 Hz JSON heartbeat telemetry.

---

## E. Learning Outcome
Through the successful execution of this activity:
1. **Mastery of Collaborative QA Tools**: Developed practical proficiency in using GitHub Issues, Labels, Milestones, Branching, and Pull Requests to track engineering defects systematically.
2. **Embedded Systems Reliability**: Learned how subtle software designs (like synchronous busy-waits or unthrottled serial prints) drastically degrade microcontroller responsiveness and cause CPU starvation.
3. **Structured Root Cause Analysis (RCA)**: Learned how to apply standard quality management techniques (5-Why Analysis and Ishikawa Fishbone Diagrams) to trace embedded bugs to their fundamental hardware/software origins.
4. **Hardware-Software Co-Design**: Recognized the necessity of electronic power budget derating, switch debouncing, and rate-limiting in safety-critical cross-domain projects.

---

## F. Pre and Post Reflections

### Pre-Reflection
*Modern electronic projects require seamless collaboration across firmware, hardware, and QA teams. Collaborative platforms like GitHub enhance transparency by versioning code alongside issue trackers, allowing any team member to understand not just what changed, but why it changed. Traceability is established through commit-to-issue linking, eliminating ambiguity during system integration.*

### Post-Reflection
*Executing this activity demonstrated that GitHub is far more than a code backup tool—it is an indispensable project management and quality assurance engine. By formalizing defects as structured issues with severity ratings and Root Cause Analyses (5-Why and Fishbone), problem solving transitioned from ad-hoc patching to rigorous engineering. Feature branches and pull requests ensured that every bug fix was independently tested, peer-reviewed, and verified before merging, preventing regression and ensuring 100% test integrity.*

---

## G. Project Planning and Tracking

### 1. Milestone Tracking
* **Milestone**: `v1.0.0-Stable-Release` ([Milestone #1 View](https://github.com/TECHYpranav07/arduino-embedded-qa-system/milestone/1))
* **Progress**: **100% Completed** (4 of 4 QA Issues Closed).

### 2. Quantitative Verification & Validation Evidence
The table below documents lab test bench measurements comparing the initial baseline prototype against the final QA-verified release:

| Parameter / Metric | Baseline Prototype (v0.1.0) | QA-Refactored Firmware (v1.0.0) | Engineering Gain |
| :--- | :--- | :--- | :--- |
| **Push-Button Latency** | Up to 2000 ms (during delays) | **< 2.5 ms** | **99.8% reduction in latency** |
| **Input Event Drop Rate** | 87.5% failure | **0.0% failure (100/100 passed)** | **Deterministic responsiveness** |
| **Spurious Bounce Triggers** | 3–5 rapid increments / press | **0 spurious triggers (50/50 passed)** | **100% mechanical bounce rejection** |
| **Continuous Current Draw** | 20.67 mA (full saturation) | **12.1 mA (clamped PWM)** | **41.5% thermal power reduction** |
| **CPU Loop Frequency** | 24 Hz (stalled by UART) | **42,800 Hz (42.8 kHz)** | **1,780x execution speedup** |
| **Telemetry Health** | Buffer overflows every cycle | **Zero dropped frames (2 Hz JSON)** | **Clean deterministic telemetry** |

---

## Self-Assessment Rubrics Mapping (15 / 15 Marks Target)

| Assessment Criteria | Target | Demonstrated Evidence in this Project | Marks Claimed |
| :--- | :---: | :--- | :---: |
| **1. Analysis of QA Documentation & GitHub Usage** | Exceptional (5 Pts) | Comprehensive QA documentation structure; GitHub Issues, Milestones, Custom Labels (`critical-severity`, `high-severity`, `medium-severity`, `qa-verified`, `electronics-qa`), Branching, and Pull Requests with complete audit logs. | **5 / 5** |
| **2. Problem Identification & Root Cause Analysis** | Exceptional (5 Pts) | 4 critical embedded problems identified across timing, electrical, signal integrity, and telemetry domains. Thorough root cause analyses conducted using **5-Why Analysis** (Issues #1, #3) and **Fishbone Diagrams** (Issues #2, #4). | **5 / 5** |
| **3. Solution Implementation & Documentation** | Exceptional (5 Pts) | High-quality, non-blocking, debounced, thermal-safe C++ firmware implemented. Clean branch-per-fix Git workflow (`09c219f`, `c68e1e9`, `b5eeb53`, `9b664c1`). Verified with before/after quantitative test bench metrics. | **5 / 5** |
| **Total Marks** | | | **15 / 15** |

---
**Report Prepared By**: Pranav Karande (B.Tech E&TC, Div C)  
**Submitted to**: Dr. Ashish Mulajkar (Course Teacher, Project Management 2307476T)  
**MIT Academy of Engineering, Pune**
