/**
 * @file smart_led_controller.ino
 * @brief Smart Multi-Mode LED & Status Controller (v0.3.0 - Debounced Input Engine)
 * @course Project Management (2307476T) - Activity 1
 * @author Pranav Karande (Department of E&TC Engineering, MIT Academy of Engineering)
 *
 * Resolved Issues:
 * - Fixes #1: Replaced blocking delay() with asynchronous millis() state scheduler.
 * - Fixes #2: Configured INPUT_PULLUP and integrated 50ms software debounce filter
 *   with falling-edge transition latching.
 */

// Hardware Pin Definitions
const int LED_PIN = 9;       // Status LED (PWM capable pin)
const int BUTTON_PIN = 2;    // Mode select push button

// Global State Variables
int currentMode = 0;

// Non-Blocking Asynchronous Timing Variables (Fixes #1)
unsigned long previousBlinkMillis = 0;
bool ledOutputState = LOW;

// Software Debouncing & Edge Detection Variables (Fixes #2)
int buttonState = HIGH;             // Filtered steady-state button reading
int lastButtonReading = HIGH;       // Raw reading from the previous loop iteration
unsigned long lastDebounceTime = 0; // Timestamp of the last raw reading change
const unsigned long DEBOUNCE_DELAY_MS = 50; // 50ms settling window for switch chatter

void setup() {
    Serial.begin(9600);
    pinMode(LED_PIN, OUTPUT);
    // Fixes #2: Activate internal pull-up resistor (prevents floating state)
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    Serial.println(F("[SYSTEM] Embedded LED Controller Initialized (v0.3.0 - Debounced)"));
}

void loop() {
    unsigned long currentMillis = millis();

    // 1. Debounced Push Button Sampling (Fixes #2)
    int rawReading = digitalRead(BUTTON_PIN);

    // Reset debounce timer if raw state changed (mechanical bounce detected)
    if (rawReading != lastButtonReading) {
        lastDebounceTime = currentMillis;
    }

    // If state has persisted beyond debounce delay window, validate new stable state
    if ((currentMillis - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
        // Detect falling edge (HIGH -> LOW on active-low pull-up button)
        if (rawReading != buttonState) {
            buttonState = rawReading;
            
            if (buttonState == LOW) {
                // Legitimate debounced press event
                currentMode = (currentMode + 1) % 4;
                Serial.print(F("[EVENT] Debounced Mode switched to: "));
                Serial.println(currentMode);
                
                // Synchronize output state
                previousBlinkMillis = currentMillis;
                ledOutputState = (currentMode == 1) ? HIGH : LOW;
                digitalWrite(LED_PIN, ledOutputState);
            }
        }
    }
    lastButtonReading = rawReading;

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
