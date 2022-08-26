# AIS 4G Board Library for Arduino


AIS 4G Board คือบอร์ดพัฒนาที่สามารถเชื่อมต่ออินเตอร์เน็ตผ่าน 4G มาพร้อมกับไมโครคอนโทรลเลอร์ ESP32-WROOM-32 และโมดูลสื่อสาร SIM7600E-H1C รองรับการเชื่อมต่ออุปกรณ์ภายนอกผ่าน GPIO ทั้ง ESP32 และ SIM7600 นอกจากนี้ยังรองรับการเชื่อมต่อแบบ I2C/RS485/SPI/I2S/UART

ไลบรารีสำหรับ AIS 4G Board ใช้กับโปรแกรม Arduino IDE รองรับการเชื่อมต่อ MQTT, HTTP, Azure IoT Hub, Azure IoT Central มาพร้อมคำสั่งอ่านพิกัดจาก GPS (บนโมดูล SIM7600), อ่านค่าอุณหภูมิ/ความชื้นจากเซ็นเซอร์บนบอร์ด, ติดต่อกับอุปกรณ์ภายนอกผ่าน RS485/ModbusRTU, บันทึกและเปิดไฟล์จาก MicroSD Card เป็นต้น


-------

## สารบัญ

 * [รู้จัก AIS 4G Board](#รู้จัก-ais-4g-board)
 * [การเริ่มต้นใช้งาน](#การเริ่มต้นใช้งาน)
 * [การส่งข้อมูลขึ้น Azure IoT Central](#การส่งข้อมูลขึ้น-azure-iot-central)
   * [เตรียมโค้ดโปรแกรม](#เตรียมโค้ดโปรแกรม)
   * [สมัคร Azure และสร้างโปรเจค](#สมัคร-azure-และสร้างโปรเจค)
   * [สร้าง Device templates](#สร้าง-device-templates)
   * [สร้าง Device](#สร้าง-device)
   * [แก้โค้ดโปรแกรม](#แก้โค้ดโปรแกรม)
   * [ตรวจสอบผลการทำงาน](ตรวจสอบผลการทำงาน)
 * [คำสั่งที่มีให้ใช้งาน](คำสั่งที่มีให้ใช้งาน)
   * [GSM.h](#include-gsmh) 
   * [GSMNetwok.h](#include-gsmnetwokh)
   * [GSMClient.h](#include-gsmclienth)
   * [GSMClientSecure.h](#include-gsmclientsecureh)
   * [GSMUdp.h](#include-gsmudph)
   * [GPS.h](#include-gpsh)
   * [Storage.h](#include-storageh)
   * [SHT40.h](#include-sht40h)
   * [RS485.h](#include-rs485h)
   * [AzureIoTHub.h](#include-azureiothubh)
   * [AzureIoTCentral.h](#include-azureiotcentralh)
 * [ศึกษาเพิ่มเติม](#ศึกษาเพิ่มเติม)
   * [เอกสารการใช้งาน](#เอกสารการใช้งาน)
   * [ตัวอย่างโค้ดโปรแกรม](#ตัวอย่างโค้ดโปรแกรม)
   * [Dimension](#Dimension)

## รู้จัก AIS 4G Board

AIS 4G Board เป็นบอร์ดที่รวมไมโครคอนโทรลเลอร์ ESP32-WROOM-32 (โมดูล WiFi/Bluetooth) เข้ากับ SIM7600E-H1C (โมดูล 4G) รองรับการเชื่อมต่ออินเตอร์เน็ตเพื่อทำงานด้าน IoT ครบทุกรูปแบบการเชื่อมต่อ ทั้งผ่านเครือข่ายโทรศัพท์มือถือ 4G, WiFi และ Bluetooth รองรับการอ่านค่าตำแหน่งปัจจุบันจาก GPS มีเซ็นเซอร์วัดอุณหภูมิและความชื้นบนบอร์ด มีปุ่มโปรแกรมได้อิสระ 1 ปุ่ม มีหลอดแอลอีดีโปรแกรมได้อิสระ 1 ดวง มีช่องเสียบ MicroSD Card มีช่องต่ออุปกรณ์ภายนอกผ่าน RS485, I2C, SPI, UART, I2S

![AIS Pinout](https://user-images.githubusercontent.com/86958708/140851317-1e8378d4-c79c-49e5-a5c2-14dd23df07b6.jpg)

## การเริ่มต้นใช้งาน

 * เสียบสาย USB Type-C เข้ากับ AIS 4G Board ฝั่ง ESP32 ปลายอีกด้านเสียบเข้ากับคอมพิวเตอร์
 * ดาวน์โหลดไดร์เวอร์ FT231X จาก [VCP Drivers](https://ftdichip.com/drivers/vcp-drivers/) และติดตั้งตามขั้นตอน [Installation Guides](https://ftdichip.com/document/installation-guides/)
 * ดาวน์โหลดโปรแกรม Arduino IDE V1.xx.xx จาก [Software | Arduino](https://www.arduino.cc/en/software) แล้วติดตั้งโปรแกรมตามขั้นตอน [Install the Arduino Software (IDE) on Windows PCs](https://www.arduino.cc/en/Guide/Windows)
 * เปิดโปรแกรม Arduino IDE ขึ้นมา แล้วติดตั้งแพ็กเกจบอร์ด ESP32 เพิ่ม โดยทำตามขั้นตอน [Installing - Arduino-ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)
 * ติดตั้งไลบรารี่ `AIS 4G Board` ผ่าน Library Manager (อ่านเพิ่มเติม [Installing Additional Arduino Libraries](https://www.arduino.cc/en/Guide/Libraries))
 * เลือกบอร์ดเป็น ESP32 Dev Module แล้วเลือกพอร์ตเป็น COM port ที่ใช้
 * เปิดโปรแกรมตัวอย่าง [Read_IMEI](examples/GSM/Read_IMEI/Read_IMEI.ino) เพื่อทดสอบอ่านหมายเลข IMEI ของโมดูล 4G (ทดสอบการอัพโหลดโปรแกรม-ทดสอบการทำงานของโมดูล 4G)
 * อัพโหลดโปรแกรมลงบอร์ด แล้วเปิด Serial Monitor ขึ้นมา ปรับ baud rate เป็น 115200 จากนั้นให้กดปุ่ม RESET แล้วดูผลลัพธ์ที่ได้ จะต้องแสดงหมายเลข IMEI ออกมา

## การส่งข้อมูลขึ้น Azure IoT Central

### เตรียมโค้ดโปรแกรม

 * ติดตั้งไลบรารี AIS IoT 4G ตามหัวข้อ [การเริ่มต้นใช้งาน](#การเริ่มต้นใช้งาน)
 * ใช้ตัวอย่าง [IoT_Central_sample](https://github.com/gravitech-engineer/AIS_IoT_4G/blob/main/examples/Azure_IoT/IoT_Central_sample/IoT_Central_sample.ino) ในการทดสอบส่งข้อมูลอุณหภูมิและความชื้นจากเซ็นเซอร์บนบอร์ด ขึ้น Azure IoT Central

### สมัคร Azure และสร้างโปรเจค

 * สมัครสมาชิก https://portal.azure.com/ กดปุ่ม Start free แล้วสมัครสมาชิกพร้อมตั้งค่ารูปแบบการชำระเงินให้เรียบร้อย (ใช้งานครั้งแรกได้เครดิทฟรี $200 เป็นเวลา 7 วัน)
 * เข้าไปที่ https://apps.azureiotcentral.com/myapps กดสร้าง Custom app ตั้งชื้อ พร้อมตั้งค่าการชำระเงินให้เรียบร้อย

### สร้าง Device templates

 * เข้าไปที่ Device templates กดปุ่ม New แล้วดูตรง Create a custom device template ให้กดเลือก IoT Device ตั้งชื่อ แล้วดำเนินขั้นตอนจนจบ
 * เข้าไปที่ Device template ที่สร้างขึ้น สร้าง/ตั้งค่า Modal ดังนี้

| Display name | Name | Capability type | Semantic type |
|--|--|--|--|
| temperature | temperature | Telemetry | Temperature |
| humidity | humidity | Telemetry | Relative humidity |
| light | light | Command |   |

 * ของ light (ที่เป็น Command) ให้กดเปิดรายละเอียดขึ้นมา แล้ว
   * กดเปิดใช้ Request
   * Display name กำหนดเป็น Level
   * Name กำหนดเป็น level
   * Schema กำหนดเป็น Integer
 * แล้วกดปุ่ม Save ให้เรียบร้อย
 * กดที่เมนู Views แล้วกดเลือก Generate default views
 * ตั้งค่าหน้า Dashboard ย้ายตำแหน่งกราฟ-กล่องข้อความ เปลี่ยนชื่อได้ตามต้องการ แล้วกด Save
 * กด Back กลับมาหน้าจัดการ Device template กดปุ่ม Publish เพื่อให้นำ template ไปใช้ตอนสร้าง Device ได้

### สร้าง Device

 * เข้าไปที่เมนู Devices กดปุ่ม New
 * ตั้งชื่ออุปกรณ์ตรง Device name แล้วเลือก Device template เป็นชื่อ template ที่สร้างไว้ก่อนหน้านี้
 * กดปุ่ม Create

### แก้โค้ดโปรแกรม

 * ใน Azure ที่หน้าอุปกรณ์ ให้กดปุ่ม Connect ด้านบนมุมซ้าย
 * คัดลอก ID scope, Device ID, Primary key ไปใส่ในโค้ดโปรแกรมตัวอย่าง [IoT_Central_sample](https://github.com/gravitech-engineer/AIS_IoT_4G/blob/main/examples/Azure_IoT/IoT_Central_sample/IoT_Central_sample.ino)
 * อัพโหลดโปรแกรมลงบอร์ด

### ตรวจสอบผลการทำงาน

 * เปิด Serial Monitor ขึ้นมา ในหน้าต่าง Serial Monitor จะแจ้งสถานะการเชื่อมต่อกับ Azure IoT Central เป็นระยะ ๆ
 * เมื่อเชื่อมต่อสำเร็จ ค่าอุณหภูมิและความชื้นจะส่งขึ้น Azure IoT Central ทุก ๆ 3 วินาที
 * ใน Azure ที่หน้าอุปกรณ์ จะแสดงสถานะอุปกรณ์เป็น Connected ให้กดที่แถบ View เพื่อดูค่าในรูปแบบกราฟ
 * กดดูข้อมูลแบบละเอียดได้ในแถบ Raw data
 * สั่งเปิด-ปิด หลอด LED E15 บนบอร์ดได้โดยกดแถบ Command ตรง Level ให้ใส่ 1 หากต้องการให้ไฟติด และใส่ 0 หากต้องการให้ไฟดับ แล้วกดปุ่ม Run
 * สังเกตว่าใน Serial Monitor จะแสดงผลข้อความแจ้งได้รับ Command ใหม่เข้ามา พร้อมหลอด LED E15 ติด-ดับ ตามคำสั่งที่ส่งเข้ามา

## คำสั่งที่มีให้ใช้งาน

### `#include <GSM.h>`

ใช้สั่งงานโมดูล SIM7600 บนบอร์ดเบื้องต้น มีคำสั่งดังนี้

  * `GSM.begin()` สั่งให้โมดูล SIM7600 เริ่มทำงาน
  * `GSM.shutdown()` สั่งให้โมดูล SIM7600 หยุดทำงาน
  * `GSM.lowPowerMode()` สั่งให้โมดูล SIM7600 เข้าโหมดประหยัดพลังงาน (โหมดเครื่องบิน)
  * `GSM.noLowPowerMode()` สั่งให้โมดูล SIM7600 ออกจากโหมดประหยัดพลังงาน
  * `GSM.getIMEI()` อ่านหมายเลข IMEI ของโมดูล
  * `GSM.getIMSI()` อ่านหมายเลข IMSI
  * `GSM.pinMode()` กำหนดโหมด INPUT/OUTPUT ของขา S3 ถึง S77
  * `GSM.digitalWrite()` กำหนดเขียนสถานะลอจิก HIGH / LOW ไปที่ขา S3 ถึง S77
  * `GSM.digitalRead()` อ่านสถานะลอจิก HIGH / LOW จากขา S3 ถึง S77

### `#include <GSMNetwok.h>`

คำสั่งเกี่ยวกับการเชื่อมต่อเครือข่าย 4G มีดังนี้

  * `Network.getCurrentCarrier()` อ่านชื่อเครือข่ายที่เชื่อมต่ออยู่
  * `Network.getSignalStrength()` อ่านความแรงของสัญญาณ 4G
  * `Network.getDeviceIP()` อ่านหมายเลข IP ของอุปกรณ์
  * `Network.pingIP()` ใช้ Ping ไปที่ Host ใด ๆ เพื่อทดสอบการเชื่อมต่ออินเตอร์เน็ต

### `#include <GSMClient.h>`

*(สืบทอดคลาส Client)* ใช้เชื่อมต่อ TCP ผ่านเครือข่าย 4G มีคำสั่งดังนี้

  * `GSMClient client` สร้าง Socket ของ TCP และสร้างออปเจค client
  * `client.connect()` สั่งเชื่อมต่อไปที่ TCP Server
  * `client.connected()` ตรวจสอบสถานะการเชื่อมต่อ TCP Server
  * `client.write()` ส่งข้อมูลไปที่ TCP Server
  * `client.available()` ตรวจสอบจำนวนข้อมูลที่ TCP Server ส่งมา
  * `client.read()` อ่านข้อมูลที่ TCP Server ส่งมา
  * `client.stop()` ตัดการเชื่อมต่อกับ TCP Server

### `#include <GSMClientSecure.h>`

*(สืบทอดคลาส Client)* ใช้เชื่อมต่อ TCP ผ่าน TLS ผ่านเครือข่าย 4G มีคำสั่งเหมือนกับ `GSMClient.h` แต่มีคำสั่งเพิ่มขึ้นมาดังนี้

  * `client.setInsecure()` ปิดการตรวจสอบใบรับรอง (CA) ของ TCP/TLS Server
  * `client.setCACert()` ตั้งค่าใบรับรองของ TCP/TLS Server  

### `#include <GSMUdp.h>`

*(สืบทอดคลาส Client)* ใช้เชื่อมต่อ UDP ผ่านเครือข่าย 4G มีคำสั่งดังนี้

  * `GSMUdp udp` จองใช้ Socket และสร้างออปเจค udp
  * `udp.begin()` เริ่มต้นใช้งาน UDP และกำหนด Local Port
  * `udp.beginPacket()` สร้าง Data Packet ใหม่ พร้อมสร้างบัฟเฟอร์ใช้เก็บข้อมูลเพื่อส่ง
  * `udp.write()` เขียนข้อมูลที่ต้องการส่งลงบัฟเฟอร์
  * `udp.endPacket()` ใช้บอกจบ Packet และส่งข้อมูล UDP ในบัฟเฟอร์ไปยัง Server
  * `udp.parsePacket()` ตรวจสอบว่ามี Data Packet ใหม่เข้ามาหรือไม่
  * `udp.available()` ตรวจสอบจำนวนข้อมูลที่ยังไม่ได้อ่าน
  * `udp.read()` อ่านข้อมูลใน Data Packet
  * `udp.stop()` ยกเลิกการใช้งาน UDP

### `#include <GPS.h>`

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

### `#include <Storage.h>`

ใช้อ่าน-เขียนไฟล์ใน SIM7600 และใน MicroSD Card

  * `Storage.fileWrite()` ใช้เขียนไฟล์
  * `Storage.fileRead()` ใช้อ่านไฟล์

**remark:** มี 2 ไดร์ให้สามารถอ่านเขียนไฟล์ได้ คือ `C:` เป็นพื้นที่เก็บข้อมูลภายใน SIM7600 และ `D:` เป็นพื้นที่ใน MicroSD Card

### `#include <SHT40.h>`

ใช้อ่านค่าเซ็นเซอร์วัดอุณหภูมิและความชื้นบนบอร์ด AIS 4G Board

 * `SHT40.begin()` เริ่มต้นใช้งานเซ็นเซอร์วัดอุณหภูมิและความชื้น
 * `SHT40.readTemperature()` อ่านค่าอุณหภูมิในหน่วยองศาเซลเซียส
 * `SHT40.readHumidity()` อ่านค่าความชื้นในหน่วย %RH

**remark:** ก่อนเรียกใช้คำสั่ง `SHT40.begin()` ต้องเรียกใช้ `Wire.begin()` ก่อนเสมอ

### `#include <RS485.h>`

ใช้รับ-ส่งข้อมูลผ่านช่อง RS485 พร้อมคำสั่งอ่านค่าผ่านโปรโตคอล MODBUS RTU มีคำสั่งพื้นฐาน `RS485.begin()` `RS485.write()` `RS485.available()` `RS485.read()` เหมือน Serial ปกติ แต่มีคำสั่งเพิ่มขึ้นมาดังนี้

 * `RS485.beginTransmission()` คำสั่งเริ่มต้นส่งข้อมูลผ่าน RS485 (จองใช้บัส RS485)
 * `RS485.endTransmission()` คำสั่งจบการส่งข้อมูลผ่าน RS485 (คืนบัส RS485 ให้อุปกรณ์อื่นใช้บัสต่อ)
 * `RS485.receive()` สั่งเข้าโหมดรับข้อมูล
 * `RS485.noReceive()` สั่งออกจากโหมดรับข้อมูล (เข้าโหมดส่งข้อมูล)
 * `RS485.coilRead()` ส่งคำสั่ง Coil Read (Function Code 1) อ่านค่าหน้าสัมผัสจากอุปกรณ์ MODBUS
 * `RS485.discreteInputRead()` ส่งคำสั่ง Read Discrete Inputs (Function Code 2) อ่านค่าหน้าสัมผัสจากอุปกรณ์ MODBUS
 * `RS485.holdingRegisterRead()` ส่งคำสั่ง Read Holding Registers (Function Code 3) อ่านค่าจาก Holding Register ในอุปกรณ์ MODBUS
 * `RS485.inputRegisterRead()` ส่งคำสั่ง Read Input Registers (Function Code 4) อ่านค่าจาก Input Register ในอุปกรณ์ MODBUS
 * `RS485.coilWrite()` ส่งคำสั่ง Coil Write (Function Code 5) สั่งให้รีเลย์/หน้าสัมผัส ของอุปกรณ์ MODBUS เปิด-ปิด

**remark:** ก่อนส่งข้อมูลด้วย `RS485.write()` `RS485.print()` `RS485.println()` ต้องเรียกใช้คำสั่ง `RS485.beginTransmission()` ก่อนเสมอ และเมื่อส่งข้อมูลครบแล้ว ต้องเรียกใช้คำสั่ง `RS485.endTransmission()` ด้วย

### `#include <AzureIoTHub.h>`

ใช้เชื่อมต่อ รับ-ส่งข้อมูลกับ Azure IoT Hub

 * `AzureIoTHub iot;` เริ่มต้นใช้งานไลบรารี Azure IoT Hub สร้างออปเจค iot
 * `iot.configs()` ตั้งค่าการเชื่อมต่อ Azure IoT Hub
 * `iot.connect()` สั่งให้เชื่อมต่อไปที่ Azure IoT Hub
 * `iot.isConnected()` ตรวจสอบการเชื่อมต่อกับ Azure IoT Hub
 * `iot.setTelemetryValue()` กำหนดค่าให้ Telemetry ที่ต้องการส่งขึ้น Azure IoT Hub
 * `iot.sendMessage()` ส่ง Telemetry ขึ้น Azure IoT Hub
 * `iot.addCommandHandle()` ใช้เพิ่มฟังก์ชั่นรับข้อมูลจาก Command ที่ส่งมาจาก Azure IoT Hub
 * `iot.loop()`

**remark:** คำสั่ง `iot.loop()` จำเป็นต้องถูกเรียกใช้ให้บ่อยที่สุดเท่าที่เป็นไปได้ หากเรียกใช้งานไม่บ่อย จะไม่สามารถรับข้อมูล (Command) จาก Azure IoT Hub ได้ และจะถูกตัดการเชื่อมต่อจาก Azure IoT Hub เป็นระยะ ๆ

### `#include <AzureIoTCentral.h>`

ใช้เชื่อมต่อ รับ-ส่งข้อมูลกับ Azure IoT Central มีคำสั่งเหมือนกับ `AzureIoTHub.h` ทุกประการ ยกเว้นตอนสร้างออปเจค ให้สร้างโดยใช้คำสั่ง `AzureIoTCentral iot;` แทน

## ศึกษาเพิ่มเติม

ข้อมูลเพิ่มเติมที่จะช่วยให้เริ่มต้นใช้งาน AIS IoT 4G board ได้ง่ายขึ้น

### เอกสารการใช้งาน

ไลบรารีนี้พัฒนาขึ้นโดยยึดมาตรฐานชื่อคำสั่งที่ Arduino กำหนดไว้ ให้ใช้เอกสารบนเว็บ Arduino ในการอ้างอิงได้เลย

 * [Arduino - GSM](https://www.arduino.cc/en/Reference/GSM)
 * [Arduino - Arduino MKR GPS](https://www.arduino.cc/en/Reference/ArduinoMKRGPS)
 * [Arduino - ArduinoRS485](https://www.arduino.cc/en/Reference/ArduinoRS485)

### ตัวอย่างโค้ดโปรแกรม

โค้ดโปรแกรมตัวอย่างอยู่ในโฟลเดอร์ `examples` แยกตามหมวดหมู่ ดังนี้

 * `GPS`
   * [Location](examples/GPS/Location/Location.ino) - อ่านพิกัดจาก GNSS แสดงผลบน Serial Monitor
   * [UnixTime](examples/GPS/UnixTime/UnixTime.ino) - อ่านค่าเวลา่ Unix (Timestamp) แสดงผลบน Serial Monitor
   * [LocalTime](examples/GPS/LocalTime/LocalTime.ino) - อ่านค่าเวลาประเทศไทย แสดงผลบน Serial Monitor
 * `Storage`
   * [File_Read_Write](examples/Storage/File_Read_Write/File_Read_Write.ino) - อ่าน-เขียนไฟล์บน SIM7600
 * `GSM`
   * [Read_IMEI](examples/GSM/Read_IMEI/Read_IMEI.ino) - อ่านหมายเลข IMEI แสดงผลบน Serial Monitor
   * [Read_IMSI](examples/GSM/Read_IMSI/Read_IMSI.ino) - อ่านหมายเลข IMSI แสดงผลบน Serial Monitor
   * [Read_ICCID](examples/GSM/Read_ICCID/Read_ICCID.ino) - อ่านหมายเลข ICCID ของ eSIM แสดงผลบน Serial Monitor
   * [LowPowerMode](examples/GSM/LowPowerMode/LowPowerMode.ino) - ตัวอย่างการสั่งให้ SIM7600 เข้าโหมดประหยัดพลังงาน
   * [digitalWrite_Sx_pin](examples/GSM/digitalWrite_Sx_pin/digitalWrite_Sx_pin.ino) สั่งให้สถานะลอจิกขา S3 เป็น HIGH/LOW ทุก ๆ 500 วินาที (โปรแกรมไฟกระพริบ)
   * [digitalRead_Sx_pin](examples/GSM/digitalRead_Sx_pin/digitalRead_Sx_pin.ino) อ่านสถานะลอจิกขา S77 แสดงผลบน Serial Monitor
 * `Network`
   * [Ping](examples/Network/Ping/Ping.ino) - Ping เว็บ www.ais.co.th
   * [Read_Device_IP](examples/Network/Read_Device_IP/Read_Device_IP.ino) - อ่านหมายเลข IP แสดงผลบน Serial Monitor
   * [Read_Operator_Name](examples/Network/Read_Operator_Name/Read_Operator_Name.ino) - อ่านชื่อเครือข่ายที่เชื่อมต่ออยู่
   * [Read_Signal_Strength](examples/Network/Read_Signal_Strength/Read_Signal_Strength.ino) - อ่านความแรงสัญญาณ 4G
 * `TCP`
   * [GSMClient](examples/TCP/GSMClient/GSMClient.ino) - ตัวอย่างการรับ-ส่งข้อมูลผ่าน TCP
   * [GSMClientSecure](examples/TCP/GSMClientSecure/GSMClientSecure.ino) - ตัวอย่างการรับ-ส่งข้อมูลผ่าน TCP/TLS
 * `UDP`
   * [GSMUdpNtpClient](examples/UDP/GSMUdpNtpClient/GSMUdpNtpClient.ino) - อ่านค่าเวลาจากอินเตอร์เน็ตด้วยโปรโตคอล NTP
 * `MQTT` - ตัวอย่างในโฟลเดอร์นี้ดัดแปลงมาจาก [PubSubClient](https://github.com/knolleary/pubsubclient) ศึกษารายละเอียดเพิ่มเติมได้ในลิ้งต้นฉบับ
   * [mqtt_basic](examples/MQTT/mqtt_basic/mqtt_basic.ino) - ตัวอย่างการเชื่อมต่อ MQTT อย่างง่าย ส่งข้อมูลเข้า Topic `outTopic` และ Subscribe Topic `inTopic`
   * [mqtt_auth](examples/MQTT/mqtt_auth/mqtt_auth.ino) - ตัวอย่างการเชื่อมต่อ MQTT แบบต้องใช้ Username และ Password
   * [mqtt_publish_in_callback](examples/MQTT/mqtt_publish_in_callback/mqtt_publish_in_callback.ino) - ตัวอย่างการส่งข้อมูลเข้า Topic `outTopic` ในฟังก์ชั่น Callback
 * `Sensor`
   * [SHT40_Read](examples/Sensor/SHT40_Read/SHT40_Read.ino) - อ่านอุณหภูมิและความชื้นจากเซ็นเซอร์บนบอร์ด แสดงผลบน Serial Monitor
 * `OTA`
   * [OTA_via_HTTP_over_4G](examples/OTA/OTA_via_HTTP_over_4G/OTA_via_HTTP_over_4G.ino) - อัพเดทเฟิร์มแวร์ผ่าน HTTP ด้วยเครือข่าย 4G (โค้ดโปรแกรมส่วนใหญ่ดัดแปลงมาจากตัวอย่าง [AWS_S3_OTA_Update](https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/examples/AWS_S3_OTA_Update/AWS_S3_OTA_Update.ino))
 * `RS485`
   * [RS485_Slave_Echo](examples/RS485/RS485_Slave_Echo/RS485_Slave_Echo.ino) - ตัวอย่างการรับข้อมูลจาก RS485 Master แล้วตอบข้อมูลได้ที่รับมากลับไป
   * [SDM120CT_Read](examples/RS485/SDM120CT_Read/SDM120CT_Read.ino) - ตัวอย่างการอ่านค่าแรงดันไฟฟ้า กระแสไฟฟ้า พลังงานไฟฟ้า จาก Power Meter รุ่น SDM120CT ด้วย RS485 ผ่านโปรโตคอล MODBUS RTU
   * [XY_MD02_Read](examples/RS485/XY_MD02_Read/XY_MD02_Read.ino) - ตัวอย่างการอ่านค่าอุณหภูมิและความชื้นจากเซ็นเซอร์รุ่น XY-MD02 ด้วย RS485 ผ่านโปรโตคอล MODBUS RTU
   * [PZEM_016_Read](examples/RS485/PZEM_016_Read/PZEM_016_Read.ino) - ตัวอย่างการอ่านค่าแรงดันไฟฟ้า กระแสไฟฟ้า พลังงานไฟฟ้า จาก Power Meter รุ่น PZEM-016
   * [PZEM_016_to_IoT_Central](examples/RS485/PZEM_016_to_IoT_Central/PZEM_016_to_IoT_Central.ino) - ตัวอย่างการอ่านค่าแรงดันไฟฟ้า กระแสไฟฟ้า พลังงานไฟฟ้า จาก Power Meter รุ่น PZEM-016 ส่งค่าขึ้น Azure IoT Central
 * `HTTP` ตัวอย่างการรับ-ส่งข้อมูลผ่าน HTTP **จำเป็นต้องติดตั้งไลบรารี [ArduinoHttpClient](https://github.com/arduino-libraries/ArduinoHttpClient) เพิ่มเติม**
   * [HTTP_SimpleGet](examples/HTTP/HTTP_SimpleGet/HTTP_SimpleGet.ino) - ตัวอย่างการรับ-ส่งข้อมูลผ่าน HTTP ด้วย Method GET
   * [HTTPS_SimpleGet](examples/HTTP/HTTPS_SimpleGet/HTTPS_SimpleGet.ino) - ตัวอย่างการรับ-ส่งข้อมูลผ่าน HTTPS ด้วย Method POST
   * [HTTPS_SimplePost](examples/HTTP/HTTPS_SimplePost/HTTPS_SimplePost.ino) - ตัวอย่างการรับ-ส่งข้อมูลผ่าน HTTPS ด้วย Method POST
 * `Azure IoT`
   * `4G`
     * [IoT_Hub_sample](examples/Azure_IoT/4G/IoT_Hub_sample/IoT_Hub_sample.ino) - ตัวอย่างการอ่านค่าอุณหภูมิและความชื้นส่งค่าขึ้น Azure IoT Hub ผ่าน 4G (SIM7600)
     * [IoT_Central_sample](examples/Azure_IoT/4G/IoT_Central_sample/IoT_Central_sample.ino) - ตัวอย่างการอ่านค่าอุณหภูมิและความชื้นส่งค่าขึ้น Azure IoT Central ผ่าน 4G (SIM7600)
   * `WiFi`
     * [IoT_Hub_sample](examples/Azure_IoT/WiFi/IoT_Hub_sample/IoT_Hub_sample.ino) - ตัวอย่างการอ่านค่าอุณหภูมิและความชื้นส่งค่าขึ้น Azure IoT Hub ผ่าน WiFi (ESP32)
     * [IoT_Central_sample](examples/Azure_IoT/WiFi/IoT_Central_sample/IoT_Central_sample.ino) - ตัวอย่างการอ่านค่าอุณหภูมิและความชื้นส่งค่าขึ้น Azure IoT Central ผ่าน WiFi (ESP32)
  
### ไลบรารีแนะนำให้ใช้งานร่วมกัน

 * [ArduinoHttpClient](https://github.com/arduino-libraries/ArduinoHttpClient) - ไลบรารีเชื่อมต่อ HTTP/HTTPS

### Dimension

![AIS Pinout](https://github.com/gravitech-engineer/AIS_IoT_4G/blob/main/AIS%204G%20Dimensions.jpg?raw=true)

