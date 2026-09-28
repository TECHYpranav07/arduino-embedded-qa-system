/**
 * @file smart_led_controller.ino
 * @brief Smart Multi-Mode LED & Status Controller (v0.2.0 - Non-Blocking Engine)
 * @course Project Management (2307476T) - Activity 1
 * @author Pranav Karande (Department of E&TC Engineering, MIT Academy of Engineering)
 *
 * Resolved Issue:
 * - Fixes #1: Replaced blocking delay() with asynchronous millis() state scheduler.
 *   I/O polling cycle is now decoupled from LED timing.
 */

// Hardware Pin Definitions
const int LED_PIN = 9;       // Status LED (PWM capable pin)
const int BUTTON_PIN = 2;    // Mode select push button

// Global State Variables
int currentMode = 0;
int lastButtonState = HIGH;

// Non-Blocking Asynchronous Timing Variables (Fixes #1)
unsigned long previousBlinkMillis = 0;
bool ledOutputState = LOW;

void setup() {
    Serial.begin(9600);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT); // QA Notice: Issue #2 tracking pending
    Serial.println(F("[SYSTEM] Embedded LED Controller Initialized (v0.2.0 - Non-Blocking)"));
}

void loop() {
    unsigned long currentMillis = millis();

    // 1. Read Push Button to switch modes (Now responsive immediately)
    int buttonReading = digitalRead(BUTTON_PIN);
    if (buttonReading == LOW && lastButtonState == HIGH) {
        currentMode = (currentMode + 1) % 4;
        Serial.print(F("[EVENT] Mode switched to: "));
        Serial.println(currentMode);
        
        // Reset blink phase upon mode switch
        previousBlinkMillis = currentMillis;
        ledOutputState = (currentMode == 1) ? HIGH : LOW;
        digitalWrite(LED_PIN, ledOutputState);
    }
    lastButtonState = buttonReading;

    // 2. Execute Non-Blocking Mode Behavior (Fixes #1)
    switch (currentMode) {
        case 0: // OFF
            digitalWrite(LED_PIN, LOW);
            break;
            
        case 1: // Constant ON
            digitalWrite(LED_PIN, HIGH);
            break;
            
        case 2: // Heartbeat Blink (1000ms ON / 1000ms OFF asynchronous)
            if (currentMillis - previousBlinkMillis >= 1000) {
                previousBlinkMillis = currentMillis;
                ledOutputState = !ledOutputState;
                digitalWrite(LED_PIN, ledOutputState);
            }
            break;
            
        case 3: // Rapid Alert Blink (200ms ON / 200ms OFF asynchronous)
            if (currentMillis - previousBlinkMillis >= 200) {
                previousBlinkMillis = currentMillis;
                ledOutputState = !ledOutputState;
                digitalWrite(LED_PIN, ledOutputState);
            }
            break;
    }

    // 3. Serial Telemetry Broadcast (QA Notice: Issue #4 tracking pending)
    Serial.print(F("[TELEMETRY] Mode="));
    Serial.print(currentMode);
    Serial.print(F(" | LED_State="));
    Serial.println(digitalRead(LED_PIN));
}
