#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

const int buttonPin = 17;   // GPIO17
bool lastState = HIGH;      // Button starts HIGH (not pressed)
int scanTime = 10; //In seconds
BLEScan *pBLEScan;

LiquidCrystal_I2C lcd(0x27, 16, 2);

int serviceDataCount = 0;
int serviceDataCountText = 0;
int prevServiceDataCount = 0;
bool showFirstScreen = true;
unsigned long lastSwitchTime = 0;
const unsigned long screenInterval = 5000; // switch every 5s

// Helper: check if service data has printable ASCII
String extractPrintableASCII(const String &s, size_t minLen = 4) {
 String collected = "";
 String current = "";

 for (size_t i = 0; i < s.length(); i++) {
   unsigned char c = static_cast<unsigned char>(s[i]);
   if (c >= 32 && c <= 126) { // printable ASCII
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


class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
   void onResult(BLEAdvertisedDevice advertisedDevice) {

     // Print all devices (always)
     Serial.println("=== Device found ===");
     Serial.printf("Address: %s\n", advertisedDevice.getAddress().toString().c_str());
     Serial.printf("RSSI: %d\n", advertisedDevice.getRSSI());

     if (advertisedDevice.haveServiceData()) {
       serviceDataCount++;
       String sd = advertisedDevice.getServiceData(); // ✅ Arduino String
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

void setup() {
 pinMode(buttonPin, INPUT_PULLUP); // Internal pull-up resistor

 Serial.begin(115200);

 lcd.init(); // Initialize LCD
 lcd.backlight(); // Turn on backlight
 lcd.setCursor(0, 0);
 lcd.print("Button Ready...");

 BLEDevice::init("");

 pBLEScan = BLEDevice::getScan();
 pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
 pBLEScan->setActiveScan(true);
 pBLEScan->setInterval(100);
 pBLEScan->setWindow(99);
}

void loop() {
 bool currentState = digitalRead(buttonPin);

 if (currentState == LOW && lastState == HIGH) {  // Button pressed
   Serial.println("Button Pressed!");

   // Show scanning message
   lcd.setCursor(0, 0);
   lcd.print("Scanning...      ");
   lcd.setCursor(0, 1);
   lcd.print("Please wait ");

   serviceDataCount = 0;
   serviceDataCountText = 0;

   // Start scan (blocking)
   BLEScanResults *foundDevices = pBLEScan->start(scanTime, false);

   // Compute change
   int change = serviceDataCountText - prevServiceDataCount;


   // Print results to Serial
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

   delay(5000); // show this screen for 10 seconds

   // --- Show results screen 2 ---
   lcd.clear();
   lcd.setCursor(0, 0);
   lcd.print("TextData: ");
   lcd.print(serviceDataCountText);
   lcd.setCursor(0, 1);
   lcd.print("Change:");

   if (change > 0) lcd.print("");
   if (change == 0) lcd.print("No change");
   
   lcd.print(change > 0 ? change : (change < 0 ? -change : 0));
   if (change > 0) lcd.print(" more");
   else if (change < 0) lcd.print(" less");

   else lcd.print("");
   delay(10000); // show this screen for 10 seconds
   
   lcd.clear();
   lcd.setCursor(0, 0);
   lcd.print("Button Ready...");
   
   prevServiceDataCount = serviceDataCountText;

   // Switch screen for next scan
   showFirstScreen = !showFirstScreen;
   pBLEScan->clearResults();
 }


 lastState = currentState;

 delay(50); // Small debounce delay
}
