#include "braille_dictionary.h"
#include "braille_cell.h"

// -------------------- PIN ALLOCATION --------------------

// Motor 0 — Cell 1 Left
const int EN_PIN[4]  = { D5, 18, 25, 15 };
const int IN1_PIN[4] = { D6, 19, 33,  4 };
const int IN2_PIN[4] = { D7, 21, 32, 16 };

// Encoders
const int ENC_A[4]   = { D2, 22, 13, 17 };
const int ENC_B[4]   = { D3, 26, 14,  5 };

// Motor index labels for readability
// 0 = Cell 1 Left
// 1 = Cell 1 Right
// 2 = Cell 2 Left
// 3 = Cell 2 Right

// -------------------- MOTOR CALIBRATION --------------------
long POS8_COUNTS = 3000;
const int ENCODER_SIGN = -1;

// -------------------- TUNING --------------------
const int GO_PWM         = 35;
const int TOLERANCE      = 8;
const int MOVE_TIMEOUT   = 8000;
const int BRAKE_MS       = 80;
const int BACKOFF_COUNTS = 20;
const int SLOW_PWM       = 35;
const int SLOW_ZONE      = 25;
const int STABLE_MS      = 120;
const int STABLE_DELAY   = 20;

// -------------------- ENCODER --------------------
volatile long rawEncoderCount[4] = {0, 0, 0, 0};
volatile uint8_t lastEncoded[4]  = {0, 0, 0, 0};

const int8_t quadTable[16] = {
    0, -1,  1,  0,
    1,  0,  0, -1,
   -1,  0,  0,  1,
    0,  1, -1,  0
};

void IRAM_ATTR updateEncoderN(int n) {
    uint8_t a = digitalRead(ENC_A[n]);
    uint8_t b = digitalRead(ENC_B[n]);
    uint8_t encoded = (a << 1) | b;
    uint8_t sum = (lastEncoded[n] << 2) | encoded;
    rawEncoderCount[n] += quadTable[sum];
    lastEncoded[n] = encoded;
}

void IRAM_ATTR updateEncoder0() { updateEncoderN(0); }
void IRAM_ATTR updateEncoder1() { updateEncoderN(1); }
void IRAM_ATTR updateEncoder2() { updateEncoderN(2); }
void IRAM_ATTR updateEncoder3() { updateEncoderN(3); }

long getEncoderCount(int n) {
    noInterrupts();
    long c = rawEncoderCount[n];
    interrupts();
    return c * ENCODER_SIGN;
    
}


class BrailleQueue {
    private:
        static const int cap = 2;
        BrailleCell* arr[cap];
        int count;

    public:
        BrailleQueue() : count(0) {
            for (int i = 0; i < cap; i++) arr[i] = nullptr;
        }

        int getCount() const { return count; }
        void setCount(int i) { count = i; }
        int getCap() { return cap; }

        BrailleCell getIndex(int displayPos) const {
            int filled = (count < cap) ? count : cap;
            int oldest = (count - filled) % cap;
            BrailleCell* cell = arr[(oldest + displayPos) % cap];
            if (cell == nullptr) return BrailleCell();
            return *cell;
        }

        bool isEmpty() const { return count == 0; }
        void increaseCount() { count += 1; }
        void decreaseCount() { count -= 1; }

        void forward(char c) {
            int slot = count % cap;
            if (arr[slot] == nullptr) arr[slot] = new BrailleCell();
            arr[slot]->setLetter(c);
            count++;
        }

        void backward(char c) {
            count--;
            int slot = count % cap;
            arr[slot]->setLetter(c);
        }

        void empty() {
            this->count = 0;
            for (int i = 0; i < cap; i++) {
                delete arr[i];
                arr[i] = nullptr;
            }
        }

        void printOut() {
            for (int i = 0; i < cap; i++) {
                if (arr[i] != nullptr) {
                    Serial.print(arr[i]->getLetter());
                    Serial.print(" | ");
                }
            }
            Serial.println(" ");
        }

        void addLine(String input) {
            if (input.length() > cap) {
                Serial.println("addLine Error - too long");
                return;
            }
            for (int i = 0; i < input.length(); i++) {
                if (arr[i] == nullptr) arr[i] = new BrailleCell();
                arr[i]->setLetter(input[i]);
            }
        }
};

// -------------------- MOTOR CONTROL --------------------
void setMotor(int pwm, int dir, int n) {
    pwm = constrain(pwm, 0, 255);
    if (dir > 0) {
        digitalWrite(IN1_PIN[n], LOW);
        digitalWrite(IN2_PIN[n], HIGH);
    } else if (dir < 0) {
        digitalWrite(IN1_PIN[n], HIGH);
        digitalWrite(IN2_PIN[n], LOW);
    } else {
        digitalWrite(IN1_PIN[n], LOW);
        digitalWrite(IN2_PIN[n], LOW);
        pwm = 0;
    }
    analogWrite(EN_PIN[n], pwm);
}

void stopMotor(int n) {
    digitalWrite(IN1_PIN[n], LOW);
    digitalWrite(IN2_PIN[n], LOW);
    analogWrite(EN_PIN[n], 0);
}

void holdBrake(int n) {
    digitalWrite(IN1_PIN[n], HIGH);
    digitalWrite(IN2_PIN[n], HIGH);
    analogWrite(EN_PIN[n], 255);
}

void brakeAndHold(int n) {
    holdBrake(n);
    delay(BRAKE_MS);
    holdBrake(n);
}

void releaseBrake(int n) {
    stopMotor(n);
}

// -------------------- MOVEMENT --------------------
void moveToTargetDirect(long target, int n) {
    unsigned long startTime = millis();
    unsigned long stableStart = 0;

    while (true) {
        long current = getEncoderCount(n);
        long error = target - current;
        long absError = abs(error);

        Serial.print("Motor "); Serial.print(n);
        Serial.print("  Current: "); Serial.print(current);
        Serial.print("  Target: "); Serial.print(target);
        Serial.print("  Error: "); Serial.println(error);

        if (absError <= TOLERANCE) {
            brakeAndHold(n);
            if (stableStart == 0) stableStart = millis();
            if (millis() - stableStart >= STABLE_MS) {
                Serial.print("Motor "); Serial.print(n); Serial.println(" reached target");
                return;
            }
            delay(STABLE_DELAY);
        } else {
            stableStart = 0;
            int pwm = (absError <= SLOW_ZONE) ? SLOW_PWM : GO_PWM;
            if (error > 0) setMotor(pwm,  1, n);
            else           setMotor(pwm, -1, n);
            delay(8);
        }

        if (millis() - startTime > MOVE_TIMEOUT) {
            brakeAndHold(n);
            Serial.print("Motor "); Serial.print(n); Serial.println(" timeout");
            return;
        }
    }
}

void moveToAbsolutePosition(long target, int n) {
    long current = getEncoderCount(n);

    if (current > target) {
        long backoffTarget = target - BACKOFF_COUNTS;
        if (backoffTarget < 0) backoffTarget = 0;
        Serial.print("Motor "); Serial.print(n);
        Serial.print(" backing off to "); Serial.println(backoffTarget);
        releaseBrake(n);
        moveToTargetDirect(backoffTarget, n);
    }

    Serial.print("Motor "); Serial.print(n);
    Serial.print(" final approach to "); Serial.println(target);
    releaseBrake(n);
    moveToTargetDirect(target, n);
    Serial.print("Motor "); Serial.print(n); Serial.println(" holding position");
}

void goToPosition(int pos, int n) {
    if (pos < 1 || pos > 8) return;
    long target = (POS8_COUNTS * pos) / 8;
    Serial.print("Motor "); Serial.print(n);
    Serial.print(" going to position "); Serial.print(pos);
    Serial.print("  target = "); Serial.println(target);
    moveToAbsolutePosition(target, n);
}

// -------------------- ZERO --------------------
void setZeroAll() {
    noInterrupts();
    for (int i = 0; i < 4; i++) rawEncoderCount[i] = 0;
    interrupts();
    Serial.println("All encoders zeroed");
}

// -------------------- ANGLES --------------------
// Index 0-7 maps 3-bit dot column pattern to motor position 1-8
const int angles[8] = {1, 2, 3, 4, 5, 6, 7, 8};

// -------------------- POTENTIOMETER & BUTTON --------------------
const int potPin   = A3;
const int buttonPin = 12;

int potVal   = 0;
int speed    = 0;
int oldSpeed = 0;
const int maxSpeed = 255;

int delayTime = 1250;
unsigned long previousMillis = 0;

byte lastButtonState = HIGH;
bool isPaused = false;
unsigned long debounceDuration = 50;
unsigned long lastTimeButtonChanged = 0;

int changeReadingSpeed(int speedInput) {
    int givenSpeed = abs(250 - speedInput);
    return map(givenSpeed, 0, 250, 1000, 5000);
}

void readPotentiometer() {
    oldSpeed = speed;
    potVal = analogRead(potPin);
    speed = constrain(map(potVal, 0, 720, 0, maxSpeed), 0, maxSpeed);
    if (oldSpeed != speed) {
        Serial.print("Potentiometer: "); Serial.println(potVal);
        Serial.print("New Speed: "); Serial.println(speed);
        delayTime = changeReadingSpeed(speed);
    }
}

void readPauseButton() {
    if (millis() - lastTimeButtonChanged > debounceDuration) {
        byte buttonState = digitalRead(buttonPin);
        if (buttonState != lastButtonState) {
            lastTimeButtonChanged = millis();
            lastButtonState = buttonState;
            if (buttonState == LOW) {
                isPaused = !isPaused;
                Serial.print("Paused: ");
                Serial.println(isPaused ? "YES" : "NO");
            }
        }
    }
}

// -------------------- BRAILLE DISPLAY --------------------
void printPattern(std::vector<std::vector<int>> pattern) {
    for (int row = 0; row < pattern.size(); row++) {
        for (int col = 0; col < pattern[row].size(); col++) {
            Serial.print(pattern[row][col]);
            Serial.print(" | ");
        }
        Serial.println();
    }
}

// -------------------- OBJECTS --------------------
BrailleCell* braille;
BrailleQueue* queue;
String input = "";
int charIndex = 0;
bool processing = false;

// -------------------- SETUP --------------------
void setup() {
    Serial.begin(115200);
    pinMode(buttonPin, INPUT_PULLUP);

    for (int i = 0; i < 4; i++) {
        pinMode(EN_PIN[i],  OUTPUT);
        pinMode(IN1_PIN[i], OUTPUT);
        pinMode(IN2_PIN[i], OUTPUT);
        pinMode(ENC_A[i], INPUT_PULLUP);
        pinMode(ENC_B[i], INPUT_PULLUP);
        stopMotor(i);

        uint8_t a = digitalRead(ENC_A[i]);
        uint8_t b = digitalRead(ENC_B[i]);
        lastEncoded[i] = (a << 1) | b;
    }

    attachInterrupt(digitalPinToInterrupt(ENC_A[0]), updateEncoder0, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_A[1]), updateEncoder1, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_A[2]), updateEncoder2, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_A[3]), updateEncoder3, CHANGE);

    setZeroAll();

    braille = new BrailleCell();
    queue   = new BrailleQueue();

    while (!Serial);
    Serial.println("Enter a word:");
}

// -------------------- MAIN LOOP --------------------
void loop() {
    readPauseButton();
    readPotentiometer();

    int cap = queue->getCap();
    unsigned long currentMillis = millis();

    // INPUT SECTION
    if (Serial.available() > 0 && !processing) {
        input = Serial.readStringUntil('\n');
        input.trim();
        Serial.println("You entered: " + input);
        charIndex = 0;
        processing = true;
    }

    // PROCESSING SECTION
    if (processing) {
        if (!isPaused) {
            if (currentMillis - previousMillis >= delayTime) {
                previousMillis = currentMillis;

                if (charIndex < input.length()) {
                    String chunk;

                    if (::isUpper(input[charIndex])) {
                        chunk = input.substring(charIndex, charIndex + 1);
                        charIndex += 1;
                    } else {
                        chunk = input.substring(charIndex, charIndex + cap);
                        charIndex += cap;
                    }

                    Serial.print("Chunk: ");
                    Serial.println(chunk);

                    queue->empty();
                    queue->addLine(chunk);

                    std::vector<std::vector<int>> pattern1 = queue->getIndex(0).getPattern();
                    std::vector<std::vector<int>> pattern2 = queue->getIndex(1).getPattern();

                    if (queue->getIndex(0).isUpperCase()) {
                        // Capital indicator on cell 1, uppercase letter on cell 2
                        std::vector<std::vector<int>> capIndicator = {{0,0}, {0,0}, {0,1}};

                        int c1L = braille->getCase(angles, 'L', capIndicator);
                        int c1R = braille->getCase(angles, 'R', capIndicator);
                        int c2L = braille->getCase(angles, 'L', pattern1);
                        int c2R = braille->getCase(angles, 'R', pattern1);

                        goToPosition(c1L, 0); // Cell 1 Left
                        goToPosition(c1R, 1); // Cell 1 Right
                        goToPosition(c2L, 2); // Cell 2 Left
                        goToPosition(c2R, 3); // Cell 2 Right

                        Serial.println("Cell 1 (capital indicator):");
                        printPattern(capIndicator);
                        Serial.println("-----------");
                        Serial.println("Cell 2:");
                        printPattern(pattern1);

                    } else {
                        int c1L = braille->getCase(angles, 'L', pattern1);
                        int c1R = braille->getCase(angles, 'R', pattern1);
                        int c2L = braille->getCase(angles, 'L', pattern2);
                        int c2R = braille->getCase(angles, 'R', pattern2);

                        goToPosition(c1L, 0); // Cell 1 Left
                        goToPosition(c1R, 1); // Cell 1 Right
                        goToPosition(c2L, 2); // Cell 2 Left
                        goToPosition(c2R, 3); // Cell 2 Right

                        Serial.println("Cell 1:");
                        printPattern(pattern1);
                        Serial.println("-----------");
                        Serial.println("Cell 2:");
                        printPattern(pattern2);
                    }

                } else {
                    processing = false;
                    charIndex = 0;
                    Serial.println("Enter a word:");
                }
            }
        } else {
            readPauseButton();
        }
    }
}
