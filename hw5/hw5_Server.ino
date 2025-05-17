/*********
  ESP32 BLE Server with DHT11 Sensor
  Sends Temperature and Humidity readings via BLE notifications.
  Based on original code by Rui Santos (RandomNerdTutorials.com)
*********/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>        // For BLE Descriptors (like CCCD for notifications)

#include <DHT.h>            // Include DHT sensor library

// --- DHT Sensor Settings ---
#define DHT_PIN 18           // GPIO pin ESP32 is connected to the DHT11 data pin (예: GPIO4)
                            // <<<<<<<<<<< 중요: 실제 연결한 GPIO 핀 번호로 바꿔줘!
#define DHT_TYPE DHT11      // Define an DHT TYPE as DHT 11
                            // (만약 DHT22나 AM2302를 사용한다면 DHT22로 바꿔)

DHT dht(DHT_PIN, DHT_TYPE); // Initialize DHT sensor object

//Default Temperature is in Celsius
//Comment the next line for Temperature in Fahrenheit
#define temperatureCelsius

//BLE server name
#define bleServerName "DHT11_ESP32_Server" // BLE Server name

float temp; // Variable to store temperature
float tempF; // Variable to store temperature in Fahrenheit
float hum;  // Variable to store humidity

// Timer variables to control how often data is sent
unsigned long lastTime = 0;
unsigned long timerDelay = 5000; // Send data every 5 seconds (DHT11 can be slow, don't query too often)

bool deviceConnected = false; // Flag to track BLE connection status

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
// You can use the same UUIDs as before or generate new ones
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"

// Temperature Characteristic and Descriptor
#ifdef temperatureCelsius
  BLECharacteristic dhtTemperatureCelsiusCharacteristics("cba1d466-344c-4be3-ab3f-189f80dd7518", BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor dhtTemperatureCelsiusDescriptor(BLEUUID((uint16_t)0x2901)); // "Characteristic User Description"
#else
  BLECharacteristic dhtTemperatureFahrenheitCharacteristics("f78ebbff-c8b7-4107-93de-889a6a06d408", BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor dhtTemperatureFahrenheitDescriptor(BLEUUID((uint16_t)0x2901)); // "Characteristic User Description"
#endif

// Humidity Characteristic and Descriptor
BLECharacteristic dhtHumidityCharacteristics("ca73b3ba-39f6-4ab3-91ae-186dc9577d99", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor dhtHumidityUserDescriptor(BLEUUID((uint16_t)0x2901)); // "Characteristic User Description"

// Server Callbacks: Handle BLE connection and disconnection events
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Client Connected");
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Client Disconnected");
  }
};

// Function to initialize the DHT11 sensor
void initDHT(){
  dht.begin(); // Initialize DHT sensor
  Serial.println("DHT11 sensor initialized. Note: First few readings might be inaccurate.");
  delay(2000); // Wait a bit for sensor to stabilize (DHT11 needs some time)
}

void setup() {
  // Start serial communication for debugging
  Serial.begin(115200);
  Serial.println("Starting ESP32 BLE Server for DHT11 (Temp & Hum)");

  // Initialize DHT11 Sensor
  initDHT();

  // Create the BLE Device
  BLEDevice::init(bleServerName); // Set the BLE server name

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks()); // Set server callbacks

  // Create the BLE Service
  BLEService *dhtService = pServer->createService(SERVICE_UUID);

  // Add Temperature Characteristic to the service
  #ifdef temperatureCelsius
    dhtService->addCharacteristic(&dhtTemperatureCelsiusCharacteristics);
    dhtTemperatureCelsiusDescriptor.setValue("DHT11 Temperature Celsius");
    dhtTemperatureCelsiusCharacteristics.addDescriptor(&dhtTemperatureCelsiusDescriptor);
    dhtTemperatureCelsiusCharacteristics.addDescriptor(new BLE2902()); // Add CCCD for notifications
  #else
    dhtService->addCharacteristic(&dhtTemperatureFahrenheitCharacteristics);
    dhtTemperatureFahrenheitDescriptor.setValue("DHT11 Temperature Fahrenheit");
    dhtTemperatureFahrenheitCharacteristics.addDescriptor(&dhtTemperatureFahrenheitDescriptor);
    dhtTemperatureFahrenheitCharacteristics.addDescriptor(new BLE2902()); // Add CCCD for notifications
  #endif  

  // Add Humidity Characteristic to the service
  dhtService->addCharacteristic(&dhtHumidityCharacteristics);
  dhtHumidityUserDescriptor.setValue("DHT11 Humidity");
  dhtHumidityCharacteristics.addDescriptor(&dhtHumidityUserDescriptor);
  dhtHumidityCharacteristics.addDescriptor(new BLE2902()); // Add CCCD for notifications
  
  // Start the BLE service
  dhtService->start();

  // Start advertising so clients can find this server
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();
  Serial.println("Waiting for a client connection to notify...");
}

void loop() {
  if (deviceConnected) {
    if ((millis() - lastTime) > timerDelay) {
      // Read humidity
      hum = dht.readHumidity();
      // Read temperature as Celsius (the default)
      temp = dht.readTemperature();
      // Read temperature as Fahrenheit (if `#define temperatureCelsius` is commented out)
      //Notify temperature reading from BME sensor
      #ifdef temperatureCelsius
        static char temperatureCTemp[6];
        dtostrf(temp, 6, 2, temperatureCTemp);
        //Set temperature Characteristic value and notify connected client
        dhtTemperatureCelsiusCharacteristics.setValue(temperatureCTemp);
        dhtTemperatureCelsiusCharacteristics.notify();
        Serial.print("Temperature Celsius: ");
        Serial.print(temp);
        Serial.print(" ºC");
      #else
        static char temperatureFTemp[6];
        dtostrf(tempF, 6, 2, temperatureFTemp);
        //Set temperature Characteristic value and notify connected client
        dhtTemperatureFahrenheitCharacteristics.setValue(temperatureFTemp);
        dhtTemperatureFahrenheitCharacteristics.notify();
        Serial.print("Temperature Fahrenheit: ");
        Serial.print(tempF);
        Serial.print(" ºF");
      #endif
      
      //Notify humidity reading from BME
      static char humidityTemp[6];
      dtostrf(hum, 6, 2, humidityTemp);
      //Set humidity Characteristic value and notify connected client
      dhtHumidityCharacteristics.setValue(humidityTemp);
      dhtHumidityCharacteristics.notify();   
      Serial.print(" - Humidity: ");
      Serial.print(hum);
      Serial.println(" %");
      
      lastTime = millis();
    }
  }
  delay(100); // Small delay
}
