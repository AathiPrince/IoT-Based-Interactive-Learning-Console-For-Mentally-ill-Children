#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <MFRC522.h>

// TFT LCD Pins
#define TFT_CS   42
#define TFT_RST  43
#define TFT_DC   44
#define TFT_MOSI 45
#define TFT_SCK  47
#define TFT_MISO 46

// Initialize TFT Display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST, TFT_MISO);

// RFID Pins
#define RST_PIN 35
#define SS_PIN 53
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Buttons
#define BTN_SELECT 4
#define BTN_LEFT   3
#define BTN_RIGHT  2


#define SPEAKER_PIN 5

// Menu Options
int selectedOption = 0;
const char *options[] = {"Numbers", "Colors"};
const int totalOptions = 2;
bool inSubmenu = false;
int currentNumber = -1;
int currentColor = -1;
unsigned long pressStartTime = 0;
const int longPressDuration = 1000;

// NFC Card Mapping (Numbers & Colors)
struct NFC_Card {
    int value;
    byte uid[4];
};

// NFC Mapping for Numbers
NFC_Card numberCards[] = {
    {1, {51, 135, 193, 21}},
    {2, {243, 201, 180, 21}},
    {3, {179, 250, 194, 21}},
    {4, {67, 46, 188, 21}},
    {5, {3, 211, 193, 21}}

};

// NFC Mapping for Colors
NFC_Card colorCards[] = {
    {ILI9341_RED,    {3, 72, 196, 21}},    
    {ILI9341_BLUE,   {131, 65, 184, 21}},  
    {ILI9341_GREEN,  {163, 94, 78, 226}},  
    {ILI9341_YELLOW, {67, 169, 196, 21}},  
    {ILI9341_ORANGE, {163, 176, 174, 21}}  
};

void setup() {
    pinMode(BTN_SELECT, INPUT);
    pinMode(BTN_LEFT, INPUT);
    pinMode(BTN_RIGHT, INPUT);

    SPI.begin();
    mfrc522.PCD_Init();
    tft.begin();
    tft.setRotation(2);  // Portrait mode
    randomSeed(analogRead(A0));  // Ensure randomness

    displayMenu();
}

void loop() {
    if (!inSubmenu) {
        handleNavigation();
    } else {
        if (selectedOption == 0) {
            handleNumbers();
        } else if (selectedOption == 1) {
            handleColors();
        }
    }
}

// ** Display Main Menu **
void displayMenu() {
    tft.fillScreen(ILI9341_WHITE);
    tft.setTextSize(4);
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(40, 50);
    tft.println("Menu:");

    for (int i = 0; i < totalOptions; i++) {
        drawMenuItem(i, (i == selectedOption));
    }
}

// ** Draw a Single Menu Item **
void drawMenuItem(int index, bool selected) {
    int yPos = 100 + (index * 40);
    tft.setCursor(10, yPos);

    if (selected) {
        tft.setTextColor(ILI9341_BLUE, ILI9341_WHITE);
        tft.print("> ");
    } else {
        tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
        tft.print("  ");
    }
    
    tft.println(options[index]);
}

// ** Handle Button Navigation **
void handleNavigation() {
    if (digitalRead(BTN_LEFT) == HIGH) {  
        int prevOption = selectedOption;
        selectedOption = (selectedOption - 1 + totalOptions) % totalOptions;
        drawMenuItem(prevOption, false);
        drawMenuItem(selectedOption, true);
        delay(100);
    }

    if (digitalRead(BTN_RIGHT) == HIGH) {  
        int prevOption = selectedOption;
        selectedOption = (selectedOption + 1) % totalOptions;
        drawMenuItem(prevOption, false);
        drawMenuItem(selectedOption, true);
        delay(100);
    }

    if (digitalRead(BTN_SELECT) == HIGH) {  
        pressStartTime = millis();
        while (digitalRead(BTN_SELECT) == HIGH);
        unsigned long pressDuration = millis() - pressStartTime;
        if (pressDuration >= longPressDuration) {
            inSubmenu = false;
            displayMenu();
        } else {
            inSubmenu = true;
            if (selectedOption == 0) {
                generateNumber();
            } else if (selectedOption == 1) {
                generateColor();
            }
        }
    }
}

// ** Generate Random Number **
void generateNumber() {
    tft.fillScreen(ILI9341_WHITE);  
    currentNumber = random(1, 6);
    tft.setTextSize(18);
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(70, 50);
    tft.print(currentNumber);
}

// ** Generate Random Color **
void generateColor() {
    tft.fillScreen(ILI9341_WHITE);
    currentColor = colorCards[random(0, 5)].value;
    tft.fillRect(40, 50, 160, 160, currentColor);
}

void handleNumbers() {
    // Check for long press on SELECT to return to main menu
    if (digitalRead(BTN_SELECT) == HIGH) {
        pressStartTime = millis();
        while (digitalRead(BTN_SELECT) == HIGH) {
            if (millis() - pressStartTime >= longPressDuration) {
                inSubmenu = false;
                displayMenu();
                return;
            }
        }
    }

    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        return;
    }

    byte scannedUID[4];
    for (int i = 0; i < 4; i++) {
        scannedUID[i] = mfrc522.uid.uidByte[i];
    }

    bool match = false;
    for (int i = 0; i < 5; i++) {
        if (numberCards[i].value == currentNumber && memcmp(scannedUID, numberCards[i].uid, 4) == 0) {
            match = true;
            break;
        }
    }

    tft.fillRect(0, 200, 240, 80, ILI9341_WHITE);
    tft.setTextSize(4);
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(10, 210);

    if (match) {
        tft.setTextColor(ILI9341_GREEN);
        tft.println("Great Job");
        playSuccessTone();
        delay(1000);
        generateNumber();
    } else {
        tft.setTextColor(ILI9341_RED);
        tft.println("Try Again");
        playErrorTone();
        delay(1000);
    }
}

void handleColors() {
    // Check for long press on SELECT to return to main menu
    if (digitalRead(BTN_SELECT) == HIGH) {
        pressStartTime = millis();
        while (digitalRead(BTN_SELECT) == HIGH) {
            if (millis() - pressStartTime >= longPressDuration) {
                inSubmenu = false;
                displayMenu();
                return;
            }
        }
    }

    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        return;
    }

    byte scannedUID[4];
    for (int i = 0; i < 4; i++) {
        scannedUID[i] = mfrc522.uid.uidByte[i];
    }

    bool match = false;
    for (int i = 0; i < 5; i++) {
        if (colorCards[i].value == currentColor && memcmp(scannedUID, colorCards[i].uid, 4) == 0) {
            match = true;
            break;
        }
    }

    tft.fillRect(0, 210, 260, 80, ILI9341_WHITE);
    tft.setTextSize(4);
    tft.setTextColor(ILI9341_BLACK);
    tft.setCursor(10, 215);

    if (match) {
        tft.println("Great Job");
        playSuccessTone();
        delay(1000);
        generateColor();
    } else {
        tft.println("Try Again");
        playErrorTone();
        delay(1000);
    }
}



void playSuccessTone() {
    int melody[] = {262, 330, 392, 523, 659, 784}; // C4, E4, G4, C5, E5, G5
    int duration[] = {300, 300, 400, 300, 300, 500}; // Smooth rhythm

    for (int i = 0; i < 6; i++) {
        tone(SPEAKER_PIN, melody[i], duration[i]);
        delay(duration[i] + 50);  // Small gap for better effect
    }
    noTone(SPEAKER_PIN);
}

void playErrorTone() {
    tone(SPEAKER_PIN, 200, 300);  // "Mm"
    delay(400);
    tone(SPEAKER_PIN, 180, 300);  // "Mm" again
    delay(400);
    noTone(SPEAKER_PIN);
}
