

const int digitalPin = 22; // Or any digital pin you want to toggle
const int ledPin = 13; // led pin

void setup() {
  pinMode(digitalPin, OUTPUT); // Set the pin as an output
  pinMode(ledPin, OUTPUT); // Set the pin as an output
}

void loop() {
  digitalWrite(digitalPin, HIGH);   // Set the pin HIGH (turn LED on)
  digitalWrite(ledPin, HIGH);
  delay(1000);                // Wait for 1 second
  digitalWrite(digitalPin, LOW);    // Set the pin LOW (turn LED off)
  digitalWrite(ledPin, LOW);    // Set the pin LOW (turn LED off)
  delay(1000);                // Wait for 1 second
}