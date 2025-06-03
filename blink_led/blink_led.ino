

const int ledPin = 13; // Or any digital pin you want to toggle

void setup() {
  pinMode(ledPin, OUTPUT); // Set the pin as an output
}

void loop() {
  digitalWrite(ledPin, HIGH);   // Set the pin HIGH (turn LED on)
  delay(1000);                // Wait for 1 second
  digitalWrite(ledPin, LOW);    // Set the pin LOW (turn LED off)
  delay(1000);                // Wait for 1 second
}