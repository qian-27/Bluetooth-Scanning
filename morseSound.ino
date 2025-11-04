#include <BLEDevice.h> 
#include <BLEUtils.h> 
#include <BLEScan.h> 
#include <BLEAdvertisedDevice.h> 
#include <LiquidCrystal_I2C.h> 
#include <Wire.h> 

// --- PINNI MÄÄRITYKSET --- 
const int buttonPin = 17;   // GPIO17 
const int buzzerPin = 12;   // GPIO12 - kaiutin 

// --- BLE & LCD --- 
bool lastState = HIGH; 
int scanTime = 10; 
BLEScan *pBLEScan; 
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// --- LASKURIT --- 
int serviceDataCount = 0; 
int serviceDataCountText = 0; 
int prevServiceDataCount = 0; 
bool showFirstScreen = true; 

// --- MORSE-KOODIN MÄÄRITYKSET --- 
const char* morseDigits[] = { 
  "-----", // 0 
  ".----", // 1 
  "..---", // 2 
  "...--", // 3 
  "....-", // 4 
  ".....", // 5 
  "-....", // 6 
  "--...", // 7 
  "---..", // 8 
  "----."  // 9 
}; 

// --- FUNKTIOT MORSE-KOODIA VARTEN --- 
void playMorseSymbol(char symbol) { 
  int dotDuration = 100; 
  if (symbol == '.') { 
    tone(buzzerPin, 1000); 
    delay(dotDuration); 
    noTone(buzzerPin); 
  } else if (symbol == '-') { 
    tone(buzzerPin, 1000); 
    delay(dotDuration * 3); 
    noTone(buzzerPin); 
  } 
  delay(dotDuration); // väli symbolien välillä 
} 

void playNumberAsMorse(int number) { 
  String numStr = String(number); 
  for (int i = 0; i < numStr.length(); i++) { 
    char digit = numStr.charAt(i); 
    if (digit >= '0' && digit <= '9') { 
      const char* morse = morseDigits[digit - '0']; 
      for (int j = 0; morse[j] != '\0'; j++) { 
        playMorseSymbol(morse[j]); 
      } 
      delay(300); // tauko numeroiden välillä 
    } 
  } 
} 

// --- ASCII SUODATUS --- 
String extractPrintableASCII(const String &s, size_t minLen = 4) { 
  String collected = ""; 
  String current = ""; 
  for (size_t i = 0; i < s.length(); i++) { 
    unsigned char c = static_cast<unsigned char>(s[i]); 
    if (c >= 32 && c <= 126) { 
      current += char(c); 
    } else { 
      if (current.length() >= minLen) { 
        if (collected.length()) collected += " | "; 
        collected += current; 
      } 
      current = ""; 
    } 
  } 

  if (current.length() >= minLen) { 
    if (collected.length()) collected += " | "; 
    collected += current; 
  } 
  return collected; 
} 

// --- BLE CALLBACK --- 
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks { 
  void onResult(BLEAdvertisedDevice advertisedDevice) { 
    Serial.println("=== Device found ==="); 
    Serial.printf("Address: %s\n", advertisedDevice.getAddress().toString().c_str()); 
    Serial.printf("RSSI: %d\n", advertisedDevice.getRSSI()); 

    if (advertisedDevice.haveServiceData()) { 
      serviceDataCount++; 
      String sd = advertisedDevice.getServiceData(); 
      String text = extractPrintableASCII(sd, 4); 
      Serial.printf("Service Data (raw): %s\n", sd.c_str()); 

      if (text.length()) { 
        serviceDataCountText++; 
        Serial.printf("Text inside Service Data: %s\n", text.c_str()); 
      } 
    } 
    Serial.println("======================================"); 
  } 
}; 

  

// --- SETUP --- 
void setup() { 
  pinMode(buttonPin, INPUT_PULLUP); 
  pinMode(buzzerPin, OUTPUT); 
  
  Serial.begin(115200); 

  lcd.init(); 
  lcd.backlight(); 
  lcd.setCursor(0, 0); 
  lcd.print("Button Ready..."); 

  BLEDevice::init(""); 
  pBLEScan = BLEDevice::getScan(); 
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks()); 
  pBLEScan->setActiveScan(true); 
  pBLEScan->setInterval(100); 
  pBLEScan->setWindow(99); 
} 

  

// --- LOOP --- 
void loop() { 
  bool currentState = digitalRead(buttonPin); 

  if (currentState == LOW && lastState == HIGH) { 
    Serial.println("Button Pressed!"); 

    lcd.setCursor(0, 0); 
    lcd.print("Scanning...      "); 
    lcd.setCursor(0, 1); 
    lcd.print("Please wait "); 

    serviceDataCount = 0; 
    serviceDataCountText = 0; 

    BLEScanResults *foundDevices = pBLEScan->start(scanTime, false); 
    int change = serviceDataCountText - prevServiceDataCount; 

    Serial.println("=== Scan complete ==="); 
    Serial.printf("Total devices found: %d\n", foundDevices->getCount()); 
    Serial.printf("Devices with Service Data: %d\n", serviceDataCount); 
    Serial.printf("Text in Service Data: %d\n", serviceDataCountText); 
    Serial.printf("Change vs prev scan: %d\n", change); 
    Serial.println("====================="); 

    lcd.clear(); 
    lcd.setCursor(0, 0); 
    lcd.print("Found: "); 
    lcd.print(foundDevices->getCount()); 
    lcd.setCursor(0, 1); 
    lcd.print("ServiceData: "); 
    lcd.print(serviceDataCount); 

    delay(5000); 

    lcd.clear(); 
    lcd.setCursor(0, 0); 
    lcd.print("TextData: "); 
    lcd.print(serviceDataCountText); 
    lcd.setCursor(0, 1); 
    lcd.print("Change:"); 

    if (change == 0) lcd.print("No change"); 
    else { 
      lcd.print(change > 0 ? change : -change); 
      lcd.print(change > 0 ? " more" : " less"); 
    } 

    // 🔊 Piippaa morse-koodina löydetty tekstidata 
    playNumberAsMorse(serviceDataCountText); 

    delay(10000); 

    lcd.clear(); 
    lcd.setCursor(0, 0); 
    lcd.print("Button Ready..."); 

    prevServiceDataCount = serviceDataCountText; 
    showFirstScreen = !showFirstScreen; 
    pBLEScan->clearResults(); 
  } 

  lastState = currentState; 
  delay(50); 
} 
