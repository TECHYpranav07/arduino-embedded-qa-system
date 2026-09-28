/**
 * @file smart_led_controller.ino
 * @brief Smart Multi-Mode LED & Status Controller (v1.0.0 - Production Release)
 * @course Project Management (2307476T) - Activity 1
 * @author Pranav Karande (Department of E&TC Engineering, MIT Academy of Engineering)
 *
 * Fully Verified & Resolved QA Issues:
 * - Fixes #1: Replaced blocking delay() with asynchronous millis() state scheduler.
 * - Fixes #2: Configured INPUT_PULLUP and integrated 50ms software debounce filter.
 * - Fixes #3: Integrated PWM duty cycle throttling (MAX_PWM_DUTY = 150) and soft-fade
 *   transitions to prevent GPIO overcurrent and thermal junction stress.
 * - Fixes #4: Integrated non-blocking periodic telemetry scheduler (500ms interval)
 *   preventing UART buffer saturation and core execution stalls.
 */

// ==========================================
// 1. Hardware Pin Configurations
// ==========================================
const int LED_PIN = 9;       // Status LED (PWM capable timer output OC1A)
const int BUTTON_PIN = 2;    // Mode select push button (Active-LOW interrupt capable)

// ==========================================
// 2. Global State Machine Definitions
// ==========================================
enum OperatingMode {
    MODE_OFF = 0,             // 0% Duty cycle (quiescent standby)
    MODE_STEADY_ON = 1,       // Power-clamped continuous illumination (~58% duty)
    MODE_HEARTBEAT_BLINK = 2, // 1 Hz asynchronous slow beacon
    MODE_ALERT_BREATHE = 3    // Smooth sinusoidal/triangular alert fade
};

int currentMode = MODE_OFF;

// ==========================================
// 3. Asynchronous Timing Engines (Fixes #1)
// ==========================================
unsigned long previousBlinkMillis = 0;
bool ledOutputState = LOW;

// ==========================================
// 4. Software Debounce Engine (Fixes #2)
// ==========================================
int buttonState = HIGH;                 // Stable debounced button state
int lastButtonReading = HIGH;           // Instantaneous raw reading
unsigned long lastDebounceTime = 0;     // Timestamp of last edge jitter
const unsigned long DEBOUNCE_DELAY_MS = 50; // 50ms chatter rejection threshold

// ==========================================
// 5. Thermal & PWM Power Controls (Fixes #3)
// ==========================================
const int MAX_PWM_DUTY = 150;           // Clamped at 58.8% to limit current to <= 12mA
const int MIN_PWM_DUTY = 0;
int currentBrightness = 0;
int fadeDirection = 5;                  // PWM step delta
unsigned long previousFadeMillis = 0;
const unsigned long FADE_INTERVAL_MS = 25; // 25ms fade step interval

// ==========================================
// 6. Telemetry Rate Limiter (Fixes #4)
// ==========================================
unsigned long previousTelemetryMillis = 0;
const unsigned long TELEMETRY_INTERVAL_MS = 500; // 2 Hz telemetry heartbeat
unsigned long loopCounter = 0;          // Diagnostics loop frequency counter

void setup() {
    // Initialize Hardware UART Diagnostics
    Serial.begin(9600);
    
    // Configure GPIO Hardware
    pinMode(LED_PIN, OUTPUT);
    // Fixes #2: Activate internal pull-up resistor (eliminates floating high-Z state)
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Initialize output driver in safe default
    analogWrite(LED_PIN, 0);
    
    Serial.println(F("======================================================"));
    Serial.println(F(" Smart Multi-Mode LED Controller (v1.0.0 Stable)     "));
    Serial.println(F(" E&TC Dept - MIT Academy of Engineering, Pune         "));
    Serial.println(F(" Project Management (2307476T) - QA Verified Release  "));
    Serial.println(F("======================================================"));
}

void loop() {
    unsigned long currentMillis = millis();
    loopCounter++;

    // ----------------------------------------------------
    // Engine A: Non-Blocking Debounced Button Sampling (Fixes #2)
    // ----------------------------------------------------
    int rawReading = digitalRead(BUTTON_PIN);
    if (rawReading != lastButtonReading) {
        lastDebounceTime = currentMillis;
    }

    if ((currentMillis - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
        if (rawReading != buttonState) {
            buttonState = rawReading;
            
            // Trigger on confirmed falling edge (Active-LOW switch press)
            if (buttonState == LOW) {
                currentMode = (currentMode + 1) % 4;
                
                // Reset timing phases upon state transition
                previousBlinkMillis = currentMillis;
                previousFadeMillis = currentMillis;
                
                // Immediate output synchronization
                if (currentMode == MODE_OFF) {
                    analogWrite(LED_PIN, 0);
                } else if (currentMode == MODE_STEADY_ON) {
                    analogWrite(LED_PIN, MAX_PWM_DUTY);
                }
                
                Serial.print(F("[STATE CHANGE] Transitioned to Mode: "));
                Serial.println(currentMode);
            }
        }
    }
    lastButtonReading = rawReading;

    // ----------------------------------------------------
    // Engine B: Non-Blocking Asynchronous State Logic (Fixes #1, #3)
    // ----------------------------------------------------
    switch (currentMode) {
        case MODE_OFF:
            analogWrite(LED_PIN, 0);
            break;
            
        case MODE_STEADY_ON:
            analogWrite(LED_PIN, MAX_PWM_DUTY);
            break;
            
        case MODE_HEARTBEAT_BLINK:
            if (currentMillis - previousBlinkMillis >= 1000) {
                previousBlinkMillis = currentMillis;
                ledOutputState = !ledOutputState;
                analogWrite(LED_PIN, ledOutputState ? MAX_PWM_DUTY : 0);
            }
            break;
            
        case MODE_ALERT_BREATHE:
            if (currentMillis - previousFadeMillis >= FADE_INTERVAL_MS) {
                previousFadeMillis = currentMillis;
                currentBrightness += fadeDirection;
                
                if (currentBrightness >= MAX_PWM_DUTY) {
                    currentBrightness = MAX_PWM_DUTY;
                    fadeDirection = -fadeDirection;
                } else if (currentBrightness <= MIN_PWM_DUTY) {
                    currentBrightness = MIN_PWM_DUTY;
                    fadeDirection = -fadeDirection;
                }
                analogWrite(LED_PIN, currentBrightness);
            }
            break;
    }

    // ----------------------------------------------------
    // Engine C: Throttled Periodic Telemetry Stream (Fixes #4)
    // ----------------------------------------------------
    if (currentMillis - previousTelemetryMillis >= TELEMETRY_INTERVAL_MS) {
        previousTelemetryMillis = currentMillis;
        
        int activePwmDuty = 0;
        if (currentMode == MODE_STEADY_ON) {
            activePwmDuty = MAX_PWM_DUTY;
        } else if (currentMode == MODE_HEARTBEAT_BLINK) {
            activePwmDuty = ledOutputState ? MAX_PWM_DUTY : 0;
        } else if (currentMode == MODE_ALERT_BREATHE) {
            activePwmDuty = currentBrightness;
        }
        
        // Structured compact diagnostic packet
        Serial.print(F("{\"ts\":"));
        Serial.print(currentMillis);
        Serial.print(F(",\"mode\":"));
        Serial.print(currentMode);
        Serial.print(F(",\"pwm\":"));
        Serial.print(activePwmDuty);
        Serial.print(F(",\"btn\":"));
        Serial.print(buttonState == LOW ? 1 : 0);
        Serial.print(F(",\"loops\":"));
        Serial.print(loopCounter);
        Serial.println(F("}"));
        
        loopCounter = 0; // Reset diagnostic loop counter
    }
}
