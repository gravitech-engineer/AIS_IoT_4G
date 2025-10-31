#include <Arduino.h>
#include <MAGELLAN_SIM7600E_MQTT.h>

MAGELLAN_SIM7600E_MQTT magel;
void setup() 
{
  Serial.begin(115200);
  magel.begin();
  magel.getResponse(RESP_REPORT_JSON, [](EVENTS events){  // optional for make sure report success CODE = 20000
    Serial.println("\n =============== Callback ============== ");
    Serial.print("# [RESP REPORT] code: ");
    Serial.println(events.CODE); // follow status code on https://magellan.ais.co.th/api-document/3/0 {Error code topic}
    Serial.print("# [RESP REPORT] response message: ");
    Serial.println(events.RESP);
    Serial.print("# [MsgId] : ");
    Serial.println(events.MsgId);
    Serial.println(" =============== Callback ============== \n");
  });
}
void loop()
{
  magel.loop();
  magel.subscribes([]()
                   {
                     magel.subscribe.report.response(); // optional register for get Resp report
                   });
  magel.interval(10, []()
                 {
                   ResultReport result;              // decleare struct ResultReport for buffer result report with advance setting to report(optional)
                   RetransmitSetting priorityReport; // decleare struct RetransmitSetting for advance setting to report

                   //{1.} auto buildJSON report with RetransmitSetting
                   Serial.println("\n############## [Example Report] {1.} ##(retransmit)");
                   int repeat = 10;                               // repeat to retransmit 10 time
                   int duration = 5;                              // duration every repeat to retranse 5 sec.
                   priorityReport.option(true, repeat, duration); // assign option muti set args for retransmit
                   Serial.println("enabled: " + String(priorityReport.enabled));
                   magel.sensor.add("GPS", "13.777864,100.544068");
                   magel.sensor.add("random", (int)random(0, 100));
                   magel.sensor.add("Humidity", (int)random(0, 100));
                   result = magel.sensor.report(priorityReport); // using option setting retransmit to report
                   Serial.print("[MsgId report]: ");
                   Serial.println(result.msgId);
                   Serial.print("[Status report]: ");
                   Serial.println((result.statusReport)? "SUCCESS" : "FAIL");

                   //{2.} manual report with RetransmitSetting(disable retransmit)
                   Serial.println("\n############## [Example Report] {2.} ##(not retransmit)");
                   priorityReport.setEnabled(false); // set disable retransmit but report with msgId(if not set auto generate) *reusability struct
                   priorityReport.generateMsgId(); // from {1.} has already generate msgId if reusability same setting recommend geneateMsgId for new msgId
                   result = magel.report.send("{\"hello\":\"world\"}", priorityReport);
                   Serial.print("[MsgId report]: ");
                   Serial.println(result.msgId);
                   Serial.print("[Status report]: ");
                   Serial.println((result.statusReport)? "SUCCESS" : "FAIL");

                   //{3.} manual report with RetransmitSetting(enable retransmit) and set manual MsgId
                   Serial.println("\n############## [Example Report] {3.} ##(retransmit)");
                   int MsgId = magel.report.generateMsgId(); // generate message id
                   priorityReport.setEnabled(true);          // set enabled retransmit default is "false" *reusability struct
                   priorityReport.setMsgId(MsgId);           // set manual retransmit message id
                   result = magel.report.send("{\"hello\":\"magellan\"}", priorityReport);
                   Serial.print("[MsgId report]: ");
                   Serial.println(result.msgId);
                   Serial.print("[Status report]: ");
                   Serial.println((result.statusReport)? "SUCCESS" : "FAIL");

                   //{4.} auto buildJSON report with advance setting and specific RetransmitSetting
                   Serial.println("\n############## [Example Report] {4.} ##(not retransmit)");
                   RetransmitSetting lowPriority; // decleare new struct RetransmitSetting for advance setting to report
                   lowPriority.setEnabled(false); // set disable retransmit but report with msgId(if not set auto generate) "if retransmit disabled not using repeat and duration"
                   lowPriority.setMsgId(99999);   // set manual retransmit message id to "99999"

                   magel.sensor.add("GPS", "13.877864,100.644068");
                   magel.sensor.add("random", (int)random(0, 100));
                   magel.sensor.add("Humidity", (int)random(0, 100));
                   result = magel.sensor.report(lowPriority); // using option setting retransmit to report
                   Serial.print("[MsgId report]: ");
                   Serial.println(result.msgId);
                   Serial.print("[Status report]: ");
                   Serial.println((result.statusReport)? "SUCCESS" : "FAIL");

                   //{5.} auto buildJSON report with advance setting and specific RetransmitSetting
                   Serial.println("\n############## [Example Report] {5.} ##(retransmit)");
                   lowPriority;                  // decleare new struct RetransmitSetting for advance setting to report
                   lowPriority.setEnabled(true); // set enable retransmit but report with msgId(if not set auto generate)  *reusability struct"
                   lowPriority.setMsgId(666666); // set manual retransmit message id to "666666"

                   magel.sensor.add("GPS", "13.777864,100.644068");
                   magel.sensor.add("random", (int)random(0, 100));
                   magel.sensor.add("Humidity", (int)random(0, 100));
                   result = magel.sensor.report(lowPriority); // using option setting retransmit to report
                   Serial.print("[MsgId report]: ");
                   Serial.println(result.msgId);
                   Serial.print("[Status report]: ");
                   Serial.println((result.statusReport)? "SUCCESS" : "FAIL");
                 });
}