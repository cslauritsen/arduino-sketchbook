
 
#include <SPI.h>
#include <MySensor.h>  
#include <DHT.h>  

#define CHILD_ID_HUM 0
#define CHILD_ID_TEMP 1
#define CHILD_ID_GAS 2
#define HUMIDITY_SENSOR_DIGITAL_PIN 3
#define GAS_PIN A0


MySensor gw;
DHT dht;
float lastTemp;
float lastHum;
boolean metric = true; 
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgGas(CHILD_ID_GAS, V_LEVEL);
#define CSLDEBUG 1
#define NODEID 0X18
#define REPEATER_NODE true
#define PARENT_NODEID AUTO

void setup()  
{ 
  Serial.begin(115200);
  gw.begin(NULL, NODEID, REPEATER_NODE, PARENT_NODEID);
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN); 

  // Send the Sketch Version Information to the Gateway
  gw.sendSketchInfo("GasHumidityTemp", "1.0.1");

  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID_HUM, S_HUM);
  gw.present(CHILD_ID_TEMP, S_TEMP);
  gw.present(CHILD_ID_GAS, S_AIR_QUALITY);
  
  metric = gw.getConfig().isMetric;
}

int lastGasReading = -2;
boolean doSend = true;
long lastSend = -1;
long lastDhtRead = -1;
float temperature = -0.1;
float humidity = -0.2;
int gasReading = -1;
void loop()      
{  
  gw.process();
  if (millis() > 5L * 60L * 60L * 1000L) {
    // need to let the coil heatup for 5 minutes before giving out gas readings
    gasReading = analogRead(GAS_PIN);
    if (lastGasReading != gasReading) {
      lastGasReading = gasReading;
      doSend = true;
    }
  }

  if (abs(millis() - lastDhtRead) > dht.getMinimumSamplingPeriod()) {
    temperature = dht.getTemperature();
    if (isnan(temperature)) {
        Serial.println("Failed reading temperature from DHT");
    } else if (temperature != lastTemp) {
      lastTemp = temperature;
      doSend = true;
    }
    lastDhtRead = millis();
  }
  
  gw.process();
  
  humidity = dht.getHumidity();
  if (isnan(humidity)) {
      Serial.println("Failed reading humidity from DHT");
  } else if (humidity != lastHum) {
      lastHum = humidity;
      doSend = true;
  }

  if (millis() - lastSend > 60000L) {
    // Send at least 1x/minute, even if values haven't changed
    doSend = true;
  }
  
  if (doSend) {
    if (!metric) {
      temperature = dht.toFahrenheit(temperature);
    }
    gw.send(msgTemp.set(temperature, 1));
    Serial.print("T: ");
    Serial.println(temperature);
    gw.send(msgHum.set(humidity, 1));
    Serial.print("H: ");
    Serial.println(humidity);
    gw.send(msgGas.set(gasReading));
    Serial.print("G: ");
    Serial.println(gasReading);
    doSend = false;
    lastSend = millis();
  }
}

void incomingMessage(const MyMessage &message) {
  #ifdef CSLDEBUG
  char pbuf[2*MAX_PAYLOAD+1];
  Serial.print(",mt=");
  Serial.print(message.type);
  Serial.print(",sens=");
  Serial.print(message.sensor);
  Serial.print(",sndr=");
  Serial.print(message.sender);
  Serial.print(",dest=");
  Serial.print(message.destination);
  Serial.print(",dest=");
  Serial.print(message.destination);
  Serial.print(",payx=");
  message.getString(pbuf);
  Serial.println(pbuf);
  #endif
}


