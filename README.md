# AIS IoT 4G Library for Arduino

 > ไลบารี่นี้อยู่ในระหว่างการพัฒนา ไม่แนะนำให้ใช้งานบน Production

ไลบารี่สำหรับบอร์ด AIS 4G Dev Kit ใช้กับโปรแกรม ArduinoIDE เป็นไลบารี่พื้นฐานสำหรับไลบารี่ระดับสูง เช่น MQTT, HTTP, NETPIE (2015/2020), LINE Notify, Blynk, Firebase และอื่น ๆ ประกอบด้วย

## `#include <GSM.h>`

ใช้สั่งงานโมดูล SIM7600 บนบอร์ดเบื้องต้น มีคำสั่งดังนี้

  * `GSM.begin()` สั่งให้โมดูล SIM7600 เริ่มทำงาน
  * `GSM.shutdown()` สั่งให้โมดูล SIM7600 หยุดทำงาน
  * `GSM.lowPowerMode()` สั่งให้โมดูล SIM7600 เข้าโหมดประหยัดพลังงาน (โหมดเครื่องบิน)
  * `GSM.noLowPowerMode()` สั่งให้โมดูล SIM7600 ออกจากโหมดประหยัดพลังงาน
  * `GSM.getIMEI()` อ่านหมายเลข IMEI ของโมดูล
  * `GSM.getIMSI()` อ่านหมายเลข IMSI

## `#include <GSMNetwok.h>`

คำสั่งเกี่ยวกับการเชื่อมต่อเครือข่าย 4G มีดังนี้

  * `Network.getCurrentCarrier()` อ่านชื่อเครือข่ายที่เชื่อมต่ออยู่
  * `Network.getSignalStrength()` อ่านความแรงของสัญญาณ 4G
  * `Network.getDeviceIP()` อ่านหมายเลข IP ของอุปกรณ์
  * `Network.pingIP()` ใช้ Ping ไปที่ Host ใด ๆ เพื่อทดสอบการเชื่อมต่ออินเตอร์เน็ต

## `#include <GSMClient.h>`

*(สืบทอดคลาส Client)* ใช้เชื่อมต่อ TCP ผ่านเครือข่าย 4G มีคำสั่งดังนี้

  * `GSMClient client` สร้าง Socket ของ TCP และสร้างออปเจค client
  * `client.connect()` สั่งเชื่อมต่อไปที่ TCP Server
  * `client.connected()` ตรวจสอบสถานะการเชื่อมต่อ TCP Server
  * `client.write()` ส่งข้อมูลไปที่ TCP Server
  * `client.available()` ตรวจสอบจำนวนข้อมูลที่ TCP Server ส่งมา
  * `client.read()` อ่านข้อมูลที่ TCP Server ส่งมา
  * `client.stop()` ตัดการเชื่อมต่อกับ TCP Server

## `#include <GSMClientSecure.h>`

*(สืบทอดคลาส Client)* ใช้เชื่อมต่อ TCP ผ่าน TLS ผ่านเครือข่าย 4G มีคำสั่งเหมือนกับ `GSMClient.h` แต่มีคำสั่งเพิ่มขึ้นมาดังนี้

  * `client.setInsecure()` ปิดการตรวจสอบใบรับรอง (CA) ของ TCP/TLS Server
  * `client.setCACert()` ตั้งค่าใบรับรองของ TCP/TLS Server

## `#include <GPS.h>`

ใช้อ่านค่าพิกัด เวลา ความเร็ว จาก GNSS (GPS) มีคำสั่งดังนี้

 * `GPS.begin()` เริ่มต้นใช้งาน GNSS
 * `GPS.available()` ตรวจสอบสถานะการจับสัญญาณ GNSS (จับสัญญาณได้แล้ว/ยังจับสัญญาณไม่ได้)
 * `GPS.latitude()` อ่านค่าละติจูด
 * `GPS.longitude()` อ่านค่าลองจิจูด
 * `GPS.speed()` อ่านค่าความเร็ว
 * `GPS.course()` 
 * `GPS.altitude()` อ่านค่าความสูง
 * `GPS.getTime()` อ่านค่าเวลา Timestamp หน่วยวินาที (GMT+0)
 * `GPS.standby()` ปิดใช้ GNSS
 * `GPS.wakeup()` เปิดใช้งาน GNSS

# ศึกษาเพิ่มเติม

ข้อมูลเพิ่มเติมที่จะช่วยให้เริ่มต้นใช้งาน AIS IoT 4G board ได้ง่ายขึ้น

## เอกสารการใช้งาน

ไลบารี่นี้พัฒนาขึ้นโดยยึดมาตรฐานชื่อคำสั่งที่ Arduino กำหนดไว้ ให้ใช้เอกสารบนเว็บ Arduino ในการอ้างอิงได้เลย

 * [Arduino - GSM](https://www.arduino.cc/en/Reference/GSM)
 * [Arduino - Arduino MKR GPS](https://www.arduino.cc/en/Reference/ArduinoMKRGPS)

## ไลบารี่แนะนำให้ใช้งานร่วมกัน

 * [Azure IoT Library]() - ไลบารี่เชื่อมต่อ Azure IoT Hub / Azure IoT Central ด้วย AIS IoT 4G board
 * [PubSubClient](https://github.com/knolleary/pubsubclient) - ไลบารี่เชื่อมต่อ MQTT(S)
 * [HttpClient](https://github.com/amcewen/HttpClient) - ไลบารี่เชื่อมต่อ HTTP(S)

## ตัวอย่างโค้ดโปรแกรม

โค้ดโปรแกรมตัวอย่างอยู่ในโฟลเดอร์ `examples` แยกตามหมวดหมู่ ดังนี้

 * `GPS`
   * [Location](https://github.com/maxpromer/AIS_IoT_4G/tree/main/examples/GPS/Location/Location.ino) - อ่านพิกัดจาก GNSS แสดงผลบน Serial Monitor
   * [UnixTime](https://github.com/maxpromer/AIS_IoT_4G/tree/main/examples/GPS/UnixTime/UnixTime.ino) - อ่านค่าเวลา่ Unix (Timestamp) แสดงผลบน Serial Monitor
   * [LocalTime](https://github.com/maxpromer/AIS_IoT_4G/tree/main/examples/GPS/LocalTime/LocalTime.ino) - อ่านค่าเวลาประเทศไทย แสดงผลบน Serial Monitor
 * `GSM`
   * [Read_IMEI](https://github.com/maxpromer/AIS_IoT_4G/tree/main/examples/GSM/Read_IMEI/Read_IMEI.ino) - อ่านหมายเลข IMEI แสดงผลบน Serial Monitor
   * [Read_IMSI](https://github.com/maxpromer/AIS_IoT_4G/tree/main/examples/GSM/Read_IMSI/Read_IMSI.ino) - อ่านหมายเลข IMSI แสดงผลบน Serial Monitor
   * [LowPowerMode](https://github.com/maxpromer/AIS_IoT_4G/tree/main/examples/GSM/LowPowerMode/LowPowerMode.ino) - ตัวอย่างการสั่งให้ SIM7600 เข้าโหมดประหยัดพลังงาน
 * `Network`
   * [Ping](https://github.com/maxpromer/AIS_IoT_4G/tree/main/examples/Network/Ping/Ping.ino) - Ping เว็บ www.ais.co.th
   * [Read_Device_IP](https://github.com/maxpromer/AIS_IoT_4G/tree/main/examples/Network/Read_Device_IP/Read_Device_IP.ino) - อ่านหมายเลข IP แสดงผลบน Serial Monitor
   * [Read_Operator_Name](https://github.com/maxpromer/AIS_IoT_4G/tree/main/examples/Network/Read_Operator_Name/Read_Operator_Name.ino) - อ่านชื่อเครือข่ายที่เชื่อมต่ออยู่
   * [Read_Signal_Strength](https://github.com/maxpromer/AIS_IoT_4G/tree/main/examples/Network/Read_Signal_Strength/Read_Signal_Strength.ino) - อ่านความแรงสัญญาณ 4G
 * `TCP`
   * [GSMClient](https://github.com/maxpromer/AIS_IoT_4G/tree/main/examples/TCP/GSMClient/GSMClient.ino) - ตัวอย่างการรับ-ส่งข้อมูลผ่าน TCP
   * [GSMClientSecure](https://github.com/maxpromer/AIS_IoT_4G/tree/main/examples/TCP/GSMClientSecure/GSMClientSecure.ino) - ตัวอย่างการรับ-ส่งข้อมูลผ่าน TCP/TLS
 * `MQTT` - ตัวอย่างในโฟลเดอร์นี้จำเป็นต้องติดตั้งไลบารี่ [PubSubClient](https://github.com/knolleary/pubsubclient) เพิ่มเติม
   * [mqtt_basic](https://github.com/maxpromer/AIS_IoT_4G/tree/main/examples/MQTT/mqtt_basic/mqtt_basic.ino) - ตัวอย่างการเชื่อมต่อ MQTT อย่างง่าย ส่งข้อมูลเข้า Topic `outTopic` และ Subscribe Topic `inTopic`
   * [mqtt_auth](https://github.com/maxpromer/AIS_IoT_4G/tree/main/examples/MQTT/mqtt_auth/mqtt_auth.ino) - ตัวอย่างการเชื่อมต่อ MQTT แบบต้องใช้ Username และ Password
   * [mqtt_publish_in_callback](https://github.com/maxpromer/AIS_IoT_4G/tree/main/examples/MQTT/mqtt_publish_in_callback/mqtt_publish_in_callback.ino) - ตัวอย่างการส่งข้อมูลเข้า Topic `outTopic` ในฟังก์ชั่น Callback
 * `Sensor`
   * `SHT40_Read`- อ่านอุณหภูมิและความชื้นจากเซ็นเซอร์บนบอร์ด แสดงผลบน Serial Monitor
