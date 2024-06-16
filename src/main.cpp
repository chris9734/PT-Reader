/*
  Ethernet Modbus TCP Server LED

  This sketch creates a Modbus TCP Server with a simulated coil.
  The value of the simulated coil is set on the LED

  Circuit:
   - Any Arduino MKR Board
   - MKR ETH Shield

  created 16 July 2018
  by Sandeep Mistry
*/

#include <SPI.h>
#include <Ethernet.h>

#include <Adafruit_MAX31865.h>

#include <ArduinoRS485.h> // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>
#include <Controllino.h>

#define RREF      4300.0
#define RNOMINAL  1000.0
#define NB_SENSORS 4

Adafruit_MAX31865 thermos[NB_SENSORS] = {Adafruit_MAX31865(4), Adafruit_MAX31865(3), Adafruit_MAX31865(7), Adafruit_MAX31865(2)};
int thermosRDY[NB_SENSORS] = {CONTROLLINO_AI5, CONTROLLINO_AI0, CONTROLLINO_AI10, CONTROLLINO_AI1};
void readSensors();
void setAnalogOutputs();

byte mac[] = {
 0, 0, 0, 0, 0
};

IPAddress ip(192, 168, 1, 11);
EthernetServer ethServer(8080);

ModbusTCPServer modbusTCPServer;

void setup() {
  Serial.begin(9600);
  for(int i = 0;i < NB_SENSORS; i++)
  {
    thermos[i].begin(MAX31865_2WIRE);  // set to 2WIRE or 4WIRE as necessary
    thermos[i].readRTD(); //Need to read once to "activate" ready-pins
  }    
}

void loop()
{
  Ethernet.begin(mac, ip);

  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start the server
  ethServer.begin();
  
  // start the Modbus TCP server
  if (!modbusTCPServer.begin()) {
    Serial.println("Failed to start Modbus TCP Server!");
    while (1);
  }

  modbusTCPServer.configureInputRegisters(0x00, NB_SENSORS);
  modbusTCPServer.configureHoldingRegisters(0x00, 2);

  // listen for incoming clients
  EthernetClient client = ethServer.available();
  
  if (client) {
    // a new client connected
    Serial.println("new client");

    // let the Modbus TCP accept the connection 
    modbusTCPServer.accept(client);

    while (client.connected())
    {
      // poll for Modbus TCP requests, while client connected
      readSensors();
      if(modbusTCPServer.poll())
        setAnalogOutputs();

    }

    //Serial.println("client disconnected");
  }
}

void readSensors()
{
  for(int i = 0; i < NB_SENSORS; i++)
  {
    if(digitalRead(thermosRDY[i]) == LOW)
    {
      Serial.print("Skipping ");
      Serial.println(i);
      continue;
    }
    modbusTCPServer.inputRegisterWrite(i, thermos[i].temperature(RNOMINAL, RREF));
  }
}

void setAnalogOutputs()
{
  for(int i = 0; i < 2; i++)
  {
    analogWrite(CONTROLLINO_AO0 + i, modbusTCPServer.holdingRegisterRead(i)); //Analog Write Pins 12 and 13
  }
}