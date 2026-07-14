HardwareSerial printerSerial(1);

void setup() {
  Serial.begin(115200);
  printerSerial.begin(9600, SERIAL_8N1, -1, 47);
  delay(1000);

  printerSerial.write(0x1B);
  printerSerial.write(0x40);
  delay(500);

  printerSerial.print("Hello from ESP32!\n");
  printerSerial.write(0x0A);
  printerSerial.write(0x0A);

  Serial.println("Sent to printer!");
}

void loop() {}
