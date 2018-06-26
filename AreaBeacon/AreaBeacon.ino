#include "sys/time.h"
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLEBeacon.h"
#include "esp_sleep.h"

#define SLEEP_DURATION 2000000LL  // sleep x seconds and then wake up
#define BEACON_UUID "17f3a0c0-99c7-484d-8778-c9ae0e0d1804"
RTC_DATA_ATTR static time_t last;        // remember last boot in RTC Memory
RTC_DATA_ATTR static uint32_t bootcount; // remember number of boots in RTC Memory

BLEAdvertising *pAdvertising;

// No hardcode, this is variable
uint32_t areaBeaconID = 0;

void setBeacon() {
  BLEBeacon oBeacon = BLEBeacon();
  oBeacon.setManufacturerId(0x4C00); // fake Apple 0x004C LSB (ENDIAN_CHANGE_U16!)
  oBeacon.setProximityUUID(BLEUUID(BEACON_UUID));
  oBeacon.setMajor(0);
  oBeacon.setMinor(areaBeaconID);
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  BLEAdvertisementData oScanResponseData = BLEAdvertisementData();
  
  oAdvertisementData.setFlags(0x04); // BR_EDR_NOT_SUPPORTED 0x04
  
  std::string strServiceData = "";
  
  strServiceData += (char)26;     // Len
  strServiceData += (char)0xFF;   // Type
  strServiceData += oBeacon.getData(); 
  oAdvertisementData.addData(strServiceData);
  
  pAdvertising->setAdvertisementData(oAdvertisementData);
  pAdvertising->setScanResponseData(oScanResponseData);

}

void setup() {

    
  Serial.begin(115200);
  sensors.begin();

  // Create the BLE Device
  BLEDevice::init("AreaBeacon");

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();

  pAdvertising = pServer->getAdvertising();

  Serial.println("Sending Beacon...");
  
  setBeacon();
   // Start advertising
  pAdvertising->start();
  delay(100);
  pAdvertising->stop();
  esp_deep_sleep(SLEEP_DURATION);
}

void loop() {

}
