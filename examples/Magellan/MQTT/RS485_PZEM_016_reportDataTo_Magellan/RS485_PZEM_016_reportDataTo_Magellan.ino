#include <Arduino.h>
#include <RS485.h>
#include <MAGELLAN_SIM7600E_MQTT.h>

MAGELLAN_SIM7600E_MQTT magel;

void setup() {
  Serial.begin(115200);

  // Setup RS485 with 9600 band and 8 bit data, 1 stop bit, No parity bit
  // More : https://www.arduino.cc/reference/en/language/functions/communication/serial/begin/
  RS485.begin(9600, SERIAL_8N1);
  magel.begin();
  magel.getResponse(RESP_REPORT_JSON, [](EVENTS response){
    Serial.print("# Response from report: ");
    Serial.println(response.CODE);// follow status code on https://magellan.ais.co.th/api-document/3/0 {Error code topic}
  });
}

void loop() 
{
  magel.loop();
  magel.subscribes([](){
    magel.subscribe.report.response();
  });

  magel.interval(10, [](){
      Serial.print("# Read Power Meter... ");
      uint16_t input_register_buffer[10];
      // Read Input Register Device ID: 1, Address: 0x0000 - 0x0009 (10 register)
      RS485.inputRegisterReadU16Array(1, 0, input_register_buffer, 10);

      // Convert data
      float voltage = input_register_buffer[0] * 0.1f;
      float current = (((uint32_t)input_register_buffer[2] << 16) | input_register_buffer[1]) * 0.001f;
      float power = (((uint32_t)input_register_buffer[4] << 16) | input_register_buffer[3]) * 0.1f;
      float energy = (((uint32_t)input_register_buffer[6] << 16) | input_register_buffer[5]) * 1.0f;
      float frequency = input_register_buffer[7] * 0.1f;
      float pf = ((float)input_register_buffer[8]) * 0.01f;
      magel.sensor.add("voltage", voltage);
      magel.sensor.add("current", current);
      magel.sensor.add("power", power);
      magel.sensor.add("energy", energy);
      magel.sensor.add("frequency", frequency);
      magel.sensor.add("pf", pf);
      magel.sensor.report();
  });
}
