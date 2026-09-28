/**
 * @file smart_led_controller.ino
 * @brief Smart Multi-Mode LED & Status Controller (v0.4.0 - Thermal & PWM Safe Engine)
 * @course Project Management (2307476T) - Activity 1
 * @author Pranav Karande (Department of E&TC Engineering, MIT Academy of Engineering)
 *
 * Resolved Issues:
 * - Fixes #1: Replaced blocking delay() with asynchronous millis() state scheduler.
 * - Fixes #2: Configured INPUT_PULLUP and integrated 50ms software debounce filter.
 * - Fixes #3: Integrated PWM duty cycle throttling (MAX_PWM_DUTY = 150) and soft-fade
 *   transitions to prevent GPIO overcurrent and thermal junction stress.
 */

// Hardware Pin Definitions
const int LED_PIN = 9;       // Status LED (PWM capable pin OC1A)
const int BUTTON_PIN = 2;    // Mode select push button

// Global State Variables
int currentMode = 0;

// Non-Blocking Asynchronous Timing Variables (Fixes #1)
unsigned long previousBlinkMillis = 0;
bool ledOutputState = LOW;

// Software Debouncing & Edge Detection Variables (Fixes #2)
int buttonState = HIGH;
int lastButtonReading = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY_MS = 50;

// PWM Power Throttling & Soft-Fade Parameters (Fixes #3)
const int MAX_PWM_DUTY = 150;        // Max 58.8% duty cycle to cap current at ~12mA
const int MIN_PWM_DUTY = 0;
int currentBrightness = 0;
int fadeDirection = 5;               // Step increment for breathing effect
unsigned long previousFadeMillis = 0;
const unsigned long FADE_INTERVAL_MS = 25; // 25ms fade step interval

void setup() {
    Serial.begin(9600);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    // Initialize LED in safe OFF state
    analogWrite(LED_PIN, 0);
    
    Serial.println(F("[SYSTEM] Embedded LED Controller Initialized (v0.4.0 - PWM Throttled)"));
}

void loop() {
    unsigned long currentMillis = millis();

    // 1. Debounced Push Button Sampling (Fixes #2)
    int rawReading = digitalRead(BUTTON_PIN);
    if (rawReading != lastButtonReading) {
        lastDebounceTime = currentMillis;
    }

    if ((currentMillis - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
        if (rawReading != buttonState) {
            buttonState = rawReading;
            
            if (buttonState == LOW) {
                currentMode = (currentMode + 1) % 4;
                Serial.print(F("[EVENT] Mode switched to: "));
                Serial.println(currentMode);
                
                previousBlinkMillis = currentMillis;
                previousFadeMillis = currentMillis;
                
                // Safe state initialization on mode change
                if (currentMode == 0) {
                    analogWrite(LED_PIN, 0);
                } else if (currentMode == 1) {
                    analogWrite(LED_PIN, MAX_PWM_DUTY); // Clamped power
                }
            }
        }
    }
    lastButtonReading = rawReading;

    // 2. Execute Non-Blocking & PWM Safe Behavior (Fixes #1, #3)
    switch (currentMode) {
        case 0: // OFF (0% duty cycle)
            analogWrite(LED_PIN, 0);
            break;
            
        case 1: // Power-Clamped Steady ON (~58% duty cycle, 12mA max)
            analogWrite(LED_PIN, MAX_PWM_DUTY);
            break;
            
        case 2: // Heartbeat Blink (1000ms ON at clamped PWM / 1000ms OFF)
            if (currentMillis - previousBlinkMillis >= 1000) {
                previousBlinkMillis = currentMillis;
                ledOutputState = !ledOutputState;
                analogWrite(LED_PIN, ledOutputState ? MAX_PWM_DUTY : 0);
            }
            break;
            
        case 3: // Smooth Breathing / Alert Strobe with Soft PWM Modulation
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

    // 3. Serial Telemetry Broadcast (QA Notice: Issue #4 tracking pending)
    Serial.print(F("[TELEMETRY] Mode="));
    Serial.print(currentMode);
    Serial.print(F(" | PWM_Duty="));
    Serial.println(currentMode == 3 ? currentBrightness : (currentMode == 1 ? MAX_PWM_DUTY : (ledOutputState ? MAX_PWM_DUTY : 0)));
}
