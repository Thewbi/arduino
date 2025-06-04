#include <Arduino.h>

#define LED 13

// put function declarations here:
int myFunction(int, int);

void setup() {

  Serial.begin(9600);

  pinMode(LED, OUTPUT);

  // put your setup code here, to run once:
  int result = myFunction(2, 3);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  digitalWrite(LED, HIGH);
  Serial.println("LED is on");
  delay(1000);

  digitalWrite(LED, LOW);
  Serial.println("LED is off");
  delay(1000);
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}