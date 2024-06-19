#include <SoftwareSerial.h>
#include <DFRobot_SIM808.h>

// Define the GSM modem model for TinyGSM
#define TINY_GSM_MODEM_SIM808
#include <TinyGsmClient.h>

#define PIN_TX 10
#define PIN_RX 11
#define green 3
#define red 2

// Create a SoftwareSerial object for SIM808 communication
SoftwareSerial mySerial(PIN_TX, PIN_RX);

// Create TinyGsmClient and DFRobot_SIM808 objects
TinyGsm modem(mySerial);
TinyGsmClient client(modem);
DFRobot_SIM808 sim808(&mySerial);

// Variables to store GPS data
char lat[12];
char lon[12];

void setup() {
  // Start serial communication for debugging
  Serial.begin(9600);

  // Start SIM808 communication
  mySerial.begin(9600);
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);

  Serial.println("Initializing modem...");

  // Initialize modem
  modem.restart();

  // Wait until connected to network
  while (!modem.waitForNetwork()) {
    Serial.println("Connecting to network...");
    delay(1000);
  }

  Serial.println("Connected to network");

  // GPRS connection setup
  Serial.println("Setting up GPRS...");
  if (!modem.gprsConnect("mtnwap")) {
    Serial.println("Failed to connect to GPRS");
    while (true);
  }
  Serial.println("GPRS connected");

  // Initialize GPS
  Serial.println("Initializing GPS...");
  if (sim808.attachGPS()) {
    Serial.println("GPS power on success");
  } else {
    Serial.println("GPS power on failure");
  }

  // Send initial data to local tunnel
  sendToLocalTunnel("Initial HelloWorld message");
}

void loop() {
  // Get GPS data and send it
  if (sim808.getGPS()) {
    float la = sim808.GPSdata.lat;
    float lo = sim808.GPSdata.lon;
    int hours = sim808.GPSdata.hour + 1;
    int minutes = sim808.GPSdata.minute;
    int seconds = sim808.GPSdata.second;

    dtostrf(la, 6, 6, lat);  // Convert latitude to string
    dtostrf(lo, 6, 6, lon);  // Convert longitude to string

    String gpsData = "{\"latitude\":\"" + String(lat) + "\",\"longitude\":\"" + String(lon) + "\",\"timestamp\":\"" + String(hours) + ":" + String(minutes) + ":" + String(seconds) + "\"}";

    sendToLocalTunnel(gpsData);

    digitalWrite(green, HIGH);
    delay(10000);  // Wait for 10 seconds before sending data again
    digitalWrite(green, LOW);
  }
}

void sendToLocalTunnel(String data) {
  Serial.println("Attempting to connect to localtunnel...");

  if (client.connect("test.loca.lt", 80)) {
    Serial.println("Connected to localtunnel");

    String postData = "message=" + data;

    client.println("POST /sendMessage HTTP/1.1");
    client.println("Host: test.loca.lt");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Content-Length: " + String(postData.length()));
    client.println();
    client.println(postData);

    Serial.println("Data sent. Awaiting response...");

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }
    Serial.println("Server response:");
    while (client.available()) {
      String line = client.readStringUntil('\n');
      Serial.println(line);
    }

    client.stop();
    Serial.println("Closing connection");
  } else {
    Serial.println("Connection to localtunnel failed");
  }
}
