#include "pitches.h"

const uint8_t buttonPins[] = {0, 1, 2, 3};
const uint8_t ledPins[] = {8, 7, 6, 5};
#define SPEAKER_PIN 10

const int LATCH_PIN = 18;
const int DATA_PIN = 19;
const int CLOCK_PIN = 9;

#define MAX_GAME_LENGTH 100

const int gameTones[] = {NOTE_G3, NOTE_C4, NOTE_E4, NOTE_G5};

uint8_t gameSequence[MAX_GAME_LENGTH] = {0};
uint8_t gameIndex = 0;
int currentStepInSequence = 0;

void setup() {
  Serial.begin(9600);
  for (byte i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  randomSeed(analogRead(4));
}

/* Digit table for the 7-segment display */
const uint8_t digitTable[] = {
  0b11000000,
  0b11111001,
  0b10100100,
  0b10110000,
  0b10011001,
  0b10010010,
  0b10000010,
  0b11111000,
  0b10000000,
  0b10010000,
};
const uint8_t DASH = 0b10111111;

void sendScore(uint8_t high, uint8_t low) {
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, low);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, high);
  digitalWrite(LATCH_PIN, HIGH);
}

void displayScore() {
  int high = gameIndex % 100 / 10;
  int low = gameIndex % 10;
  sendScore(high ? digitTable[high] : 0xff, digitTable[low]);
}

/**
   Lights the given LED and plays a suitable tone
*/
void lightLedAndPlayTone(byte ledIndex) {
  digitalWrite(ledPins[ledIndex], HIGH);
  tone(SPEAKER_PIN, gameTones[ledIndex]);
  delay(300);
  digitalWrite(ledPins[ledIndex], LOW);
  noTone(SPEAKER_PIN);
}

/**
   Plays the current sequence of notes that the user has to repeat
*/
void playSequence() {
  for (int i = 0; i < gameIndex; i++) {
    byte currentLed = gameSequence[i];
    lightLedAndPlayTone(currentLed);
    delay(50);
  }
}

/**
    Waits until the user pressed one of the buttons,
    and returns the index of that button
*/
byte readButtons() {
  unsigned long startTime = millis();
  unsigned long debounceDelay = 50; // Define a suitable debounce delay, 50ms as an example
  byte lastButtonState[4] = {HIGH, HIGH, HIGH, HIGH};
  byte buttonState[4] = {HIGH, HIGH, HIGH, HIGH};
  unsigned long lastDebounceTime[4] = {0, 0, 0, 0};

  while (true) {
    for (byte i = 0; i < 4; i++) {
      byte reading = digitalRead(buttonPins[i]);
      if (reading != lastButtonState[i]) {
        lastDebounceTime[i] = millis();
      }

      if ((millis() - lastDebounceTime[i]) > debounceDelay) {
        if (reading != buttonState[i]) {
          buttonState[i] = reading;
          if (buttonState[i] == LOW) {
            return i;
          }
        }
      }

      lastButtonState[i] = reading;
    }
  }
}


/**
  Play the game over sequence, and report the game score
*/
void gameOver() {
  Serial.print("Game over! your score: ");
  Serial.println(gameIndex - 1);
  gameIndex = 0;
  delay(200);

  // Play a Wah-Wah-Wah-Wah sound
  tone(SPEAKER_PIN, NOTE_DS5);
  delay(300);
  tone(SPEAKER_PIN, NOTE_D5);
  delay(300);
  tone(SPEAKER_PIN, NOTE_CS5);
  delay(300);
  for (byte i = 0; i < 10; i++) {
    for (int pitch = -10; pitch <= 10; pitch++) {
      tone(SPEAKER_PIN, NOTE_C5 + pitch);
      delay(6);
    }
  }
  noTone(SPEAKER_PIN);

  sendScore(DASH, DASH);
  delay(500);
}

/**
   Get the user's input and compare it with the expected sequence.
*/
bool checkUserSequence() {
  for (int i = 0; i < gameIndex; i++) {
    byte expectedButton = gameSequence[i];
    byte actualButton = readButtons();
    lightLedAndPlayTone(actualButton);
    if (expectedButton != actualButton) {
      return false;
    }
  }

  return true;
}

/**
   Plays a hooray sound whenever the user finishes a level
*/
void playLevelUpSound() {
  tone(SPEAKER_PIN, NOTE_E4);
  delay(150);
  tone(SPEAKER_PIN, NOTE_G4);
  delay(150);
  tone(SPEAKER_PIN, NOTE_E5);
  delay(150);
  tone(SPEAKER_PIN, NOTE_C5);
  delay(150);
  tone(SPEAKER_PIN, NOTE_D5);
  delay(150);
  tone(SPEAKER_PIN, NOTE_G5);
  delay(150);
  noTone(SPEAKER_PIN);
}

void startLedAndTone(byte ledIndex) {
  digitalWrite(ledPins[ledIndex], HIGH);
  tone(SPEAKER_PIN, gameTones[ledIndex]);
}

enum GameState {
  DISPLAY_SCORE,
  PLAY_SEQUENCE,
  LED_ON,
  LED_OFF,
  WAIT_FOR_INPUT,
  LEVEL_UP,
  GAME_OVER
};

GameState currentState = DISPLAY_SCORE;
GameState nextState = DISPLAY_SCORE;

unsigned long previousMillis = 0;
unsigned long interval = 300;

int inputIndex = 0;

void loop() {
  unsigned long currentMillis = millis();

  switch (currentState) {
    case DISPLAY_SCORE:
      displayScore();
      gameSequence[gameIndex] = random(0, 4);
      gameIndex++;
      if (gameIndex >= MAX_GAME_LENGTH) {
        gameIndex = MAX_GAME_LENGTH - 1;
      }
      nextState = PLAY_SEQUENCE;
      break;

    case PLAY_SEQUENCE:
      if (currentMillis - previousMillis >= interval) {
          previousMillis = currentMillis;
          if (currentStepInSequence < gameIndex) {
              startLedAndTone(gameSequence[currentStepInSequence]);
              nextState = LED_ON;
          } else {
              currentStepInSequence = 0;
              nextState = WAIT_FOR_INPUT;
          }
      }
      break;

    case LED_ON:
      if (currentMillis - previousMillis >= 300) {
        digitalWrite(ledPins[gameSequence[currentStepInSequence]], LOW);
        noTone(SPEAKER_PIN);
        previousMillis = currentMillis;
        nextState = LED_OFF;
      }
      break;

    case LED_OFF:
      if (currentMillis - previousMillis >= 50) {
        previousMillis = currentMillis;
        currentStepInSequence++;
        nextState = PLAY_SEQUENCE;
      }
      break;

    case WAIT_FOR_INPUT:
      if (checkUserSequence()) {
        nextState = LEVEL_UP;
      } else {
        nextState = GAME_OVER;
      }
      break;

    case LEVEL_UP:
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        playLevelUpSound();
        nextState = DISPLAY_SCORE;
      }
      break;

    case GAME_OVER:
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        gameOver();
        nextState = DISPLAY_SCORE;
      }
      break;
  }

  if (nextState != currentState) {
    currentState = nextState;
    previousMillis = currentMillis;
  }
}
