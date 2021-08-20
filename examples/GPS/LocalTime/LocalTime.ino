#include <SIM76xx.h>
#include <GPS.h>

#define TIMEZONE (7 * 60 * 60) // +7 Hour of Bangkok, Thailand

void setup() {
  Serial.begin(115200);
  Serial.println("Start !");

  if (!GSM.begin()) {
    Serial.println("GSM setup fail");
    while(1) delay(100);
  }

  if (!GPS.begin()) {
    Serial.println("GPS setup fail");
    while(1) delay(100);
  }
}

void loop() {
  if (GPS.available()) {
    time_t timestamp = GPS.getTime() + TIMEZONE;
    struct tm *local_time = gmtime(&timestamp);
    local_time->tm_year += 1900;
    local_time->tm_year += 543; // แปลง ค.ศ เป็น พ.ศ

    Serial.printf("Local time: %d/%d/%d %02d:%02d:%02d\n", // format: d/m/Y hh:mm:ss
      local_time->tm_mday, local_time->tm_mon, local_time->tm_year,
      local_time->tm_hour, local_time->tm_min, local_time->tm_sec
    );
  } else {
    Serial.println("GPS not fixed");
  }
  delay(1000);
}
