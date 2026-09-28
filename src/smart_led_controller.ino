/**
 * @file smart_led_controller.ino
 * @brief Smart Multi-Mode LED & Status Controller (Initial Prototype v0.1.0)
 * @course Project Management (2307476T) - Activity 1
 * @author Pranav Karande (Department of E&TC Engineering, MIT Academy of Engineering)
 *
 * Description:
 * Initial baseline prototype for multi-mode LED status indicator.
 * Modes:
 *   - Mode 0: Constant OFF
 *   - Mode 1: Constant ON
 *   - Mode 2: Slow Heartbeat Blink (1000ms)
 *   - Mode 3: Rapid Alert Blink (200ms)
 *
 * NOTE: This is the baseline prototype codebase logged for QA analysis.
 * It contains critical embedded QA issues to be resolved through GitHub Issues.
 */

// Hardware Pin Definitions
const int LED_PIN = 9;       // Status LED (PWM capable pin)
const int BUTTON_PIN = 2;    // Mode select push button

// Global State Variables
int currentMode = 0;
int lastButtonState = HIGH;

void setup() {
    // Initialize Serial Port for telemetry
    Serial.begin(9600);
    
    // Pin Configuration
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT); // QA Notice: Floating pin, lacking pull-up
    
    Serial.println(F("[SYSTEM] Embedded LED Controller Initialized (Prototype v0.1.0)"));
}

void loop() {
    // 1. Read Push Button to switch modes
    int buttonReading = digitalRead(BUTTON_PIN);
    if (buttonReading == LOW && lastButtonState == HIGH) {
        currentMode = (currentMode + 1) % 4;
        Serial.print(F("[EVENT] Mode switched to: "));
        Serial.println(currentMode);
    }
    lastButtonState = buttonReading;

    // 2. Execute Mode Behavior
    switch (currentMode) {
        case 0: // OFF
            digitalWrite(LED_PIN, LOW);
            break;
            
        case 1: // Constant ON (100% duty cycle)
            digitalWrite(LED_PIN, HIGH);
            break;
            
        case 2: // Heartbeat Blink (QA Notice: Blocking delay)
            digitalWrite(LED_PIN, HIGH);
            delay(1000);
            digitalWrite(LED_PIN, LOW);
            delay(1000);
            break;
            
        case 3: // Rapid Alert Blink (QA Notice: Blocking delay)
            digitalWrite(LED_PIN, HIGH);
            delay(200);
            digitalWrite(LED_PIN, LOW);
            delay(200);
            break;
    }

    // 3. Serial Telemetry Broadcast (QA Notice: Floods UART TX buffer every loop)
    Serial.print(F("[TELEMETRY] Mode="));
    Serial.print(currentMode);
    Serial.print(F(" | LED_State="));
    Serial.println(digitalRead(LED_PIN));
}
