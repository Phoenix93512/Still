HardwareSerial printerSerial(1);

void setup() {
  Serial.begin(115200);
  printerSerial.begin(9600, SERIAL_8N1, -1, 47);
  delay(1000);

  printerSerial.write(0x1B); printerSerial.write(0x40);
  delay(200);

  printerSerial.write(0x1D); printerSerial.write(0x76);
  printerSerial.write(0x30); printerSerial.write(0x00);
  printerSerial.write(48);   printerSerial.write(0x00);
  printerSerial.write(8);    printerSerial.write(0x00);

  for (int row = 0; row < 8; row++) {
    byte pattern = (row % 2 == 0) ? 0xAA : 0x55;
    for (int i = 0; i < 48; i++) printerSerial.write(pattern);
  }

  printerSerial.write(0x0A); printerSerial.write(0x0A);
  Serial.println("Sent checkerboard!");
}

void loop() {}
