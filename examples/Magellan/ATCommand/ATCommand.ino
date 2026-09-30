

#include <Arduino.h>

#include <MAGELLAN_SIM7600E_MQTT.h>
MAGELLAN_SIM7600E_MQTT board;
MAGELLAN_SIM7600E_MQTT::ConnectivityModem &gsmBoard = board.GSMModem;

#define LED_BUILTIN 15

String atcmd(String _atcmd, TinyGsm &modem);

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW); // set status ON OFF Relay Channel 1
  gsmBoard.begin();
  Serial.println("Start Test \"ATI\" Command");
  TinyGsm modem = gsmBoard.getModem();
  String res = atcmd("I", modem);
  Serial.print(F("Received: "));
  Serial.println(res);
  Serial.println(F("============================================"));
  Serial.println(F(">>> Modem Initialized and Connected"));
  Serial.println(F("Type AT Comand here without 'AT' prefix and press Enter:"));
  Serial.println(F("If you want only send 'AT' command, just press Enter without typing anything."));
  Serial.println(F("============================================"));
}

void loop()
{
  if (Serial.available() > 0) // handle user input from Serial Monitor
  {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    String input = Serial.readStringUntil('\n');
    input.trim();
    Serial.println(F("============================================"));
    TinyGsm modem = gsmBoard.getModem();
    String res = atcmd(input, modem);
    Serial.print(F("Received: "));
    Serial.println(res);
    Serial.println(F("Ready to listen for next command..."));
    Serial.println(F("============================================"));
    digitalWrite(LED_BUILTIN, LOW);
  }
  delay(5);
}

String atcmd(String _atcmd, TinyGsm &modem)
{
  Serial.print(F("Sent: \nAT"));
  Serial.println(_atcmd);
  modem.sendAT(_atcmd);
  String res;
  if (modem.waitResponse(1000, res) == 1)
  {
    Serial.println(F("--- Scan Results ---"));
    // Serial.println(res);
    return res;
  }
  else
  {
    Serial.println(F("Scan failed or timeout."));
    return "";
  }
}
