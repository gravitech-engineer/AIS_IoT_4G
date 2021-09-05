#include <SIM76xx.h>
#include <Storage.h>

/*
  Storage has two drive:
    - C: is internal SIM7600 storage
    - D: is MicroSD Card
  
  example 1: Write test1.txt to internal SIM7600 storage (Drive C)
    Storage.fileWrite("C:/test1.txt", "Hello");
  
  example 2: Write test1.txt to MicroSD Card (Drive D)
    Storage.fileWrite("D:/test1.txt", "Hello");

*/

void setup() {
  Serial.begin(115200);
  Serial.println("Hello!");
  
  if (!GSM.begin()) {
    Serial.println("Setup GSM fail");
    while(1) delay(1);
  }

  bool write_ok = Storage.fileWrite("C:/test1.txt", "Hello, write to test1.txt");
  if (!write_ok) {
    Serial.println("Write file error !");
  }

  String file_content = Storage.fileRead("C:/test1.txt");
  Serial.println("File content: " + file_content);
}

void loop() {
  delay(500);
}
