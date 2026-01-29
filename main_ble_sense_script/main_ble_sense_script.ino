void setup() {
  // put your setup code here, to run once:
  // Serial initialisations
  Serial.begin(115200); // For USB serial monitor
  Serial1.begin(9600); // For UART from Uno

  pinMode(LED_BUILTIN, OUTPUT); // Initialise built in LED for debugging

}

void loop() {
  // put your main code here, to run repeatedly:
  // Print to serial montior the values received
  if (Serial1.available()) {
    int data_in = Serial1.parseInt();

    Serial.print("Val received: ");
    Serial.println(data_in);

    if (data_in == 8) { digitalWrite(LED_BUILTIN, HIGH); }
    else { digitalWrite(LED_BUILTIN, LOW); }
  }

}
