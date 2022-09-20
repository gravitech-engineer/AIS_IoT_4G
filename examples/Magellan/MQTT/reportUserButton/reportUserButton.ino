#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>
#define userButton_E18 18
MAGELLAN_SIM7600E_MQTT magel;
void setup() 
{
  Serial.begin(115200);
  pinMode(userButton_E18, INPUT);
  magel.begin(); 
  magel.getResponse(RESP_REPORT_JSON, [](EVENTS value){  // optional for make sure report success CODE = 20000
    Serial.println("[RESP REPORT]");
    Serial.println(value.CODE);// follow status code on https://magellan.ais.co.th/api-document/3/0 {Error code topic}
  });
}

void loop() 
{
  magel.loop();
  magel.subscribes([]()
  {
    magel.subscribe.report.response(); // optional register for get Resp report
  });
  bool stateButton = !digitalRead(userButton_E18);
  String value = (stateButton == true)? "Press" : "Release"; 
  String buttonValue = "{\"userButton\":\""+ value +"\"}";
  if(stateButton)
  {
    magel.report.send(buttonValue);
    delay(300);
  }
  magel.interval(10, [=](){ //time interval function inside every 10 second
    /*<< lambda expressions capture clause [=] allow variable accessible to inside funtion
   as copy(use value only),if [&]  allow variable accessible to inside funtion as address(can use value and modify value from refference variable) 
   read more https://docs.microsoft.com/en-us/cpp/cpp/lambda-expressions-in-cpp?view=msvc-170
   */
   magel.report.send(buttonValue);
  });
}

