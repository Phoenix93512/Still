void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Hello from ESP32-S3!");
}

void loop() {
  Serial.println("Running...");
  delay(2000);
}
