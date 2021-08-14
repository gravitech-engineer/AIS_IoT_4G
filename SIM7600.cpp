#include "Arduino.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include "SIM7600.h"

// #define DISABLE_RESET_EC21 1
// #define GSM_MAX_TRY_COMMAND 20
#define GSM_MAX_TRY_COMMAND 1
#define DEBUG_SERIAL Serial
#define GSM_SERIAL Serial1

// #define TYPE_SIM7600E 1

#define VPRINTF_STACK_BUFFER_SIZE 100

int lib_printf(const char *tag, const char *format, va_list arg) {
  char *temp = (char *)malloc(VPRINTF_STACK_BUFFER_SIZE);
  int len = vsnprintf(temp, VPRINTF_STACK_BUFFER_SIZE - 1, format, arg);
  temp[VPRINTF_STACK_BUFFER_SIZE - 1] = 0;
  int i;
  for (i = len - 1; i >= 0; --i) {
    if (temp[i] != '\n' && temp[i] != '\r' && temp[i] != ' ') {
      break;
    }
    temp[i] = 0;
  }
  if (i > 0) {
    DEBUG_SERIAL.printf("[%s]: %s\n", tag, temp);
  }
  va_end(arg);
  free(temp);
  return len;
}

static int GSM_LOG_I(const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  int res = lib_printf("EC21:I", format, arg);
  va_end(arg);
  return res;
}

static int GSM_LOG_E(const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  int res = lib_printf("EC21:E", format, arg);
  va_end(arg);
  return res;
}

/*
 #define GSM_LOG_I(...) ;
 #define GSM_LOG_E(...) ;
 */

static int GSM_Read(uint8_t *buff, uint8_t size, uint32_t timeout) {
  uint8_t next = 0;

  uint64_t start_time = millis();
  do {
    while (GSM_SERIAL.available()) {
      buff[next] = GSM_SERIAL.read();
      next++;
      if (next == size) {
        return 0;
      }
    }
    delay(1);
  } while ((millis() - start_time) < timeout);

  return -1;
}

static bool GSM_ReadEndWiths(char *buff, uint16_t size, char *endswith, uint32_t timeout) {
  uint16_t endswithLength = strlen(endswith);
  uint8_t next = 0;

  uint64_t start_time = millis();
  do {
    while (GSM_SERIAL.available()) {
      buff[next] = GSM_SERIAL.read();
      next++;
      if ((memcmp(endswith, &buff[next - endswithLength], endswithLength) == 0) || (next == (size - 1))) {
        buff[next] = 0;
        return true;
      }
    }
    delay(1);
  } while ((millis() - start_time) < timeout);

  return false;
}

static bool GSM_Send(char *str) {
  GSM_SERIAL.print(str);
  GSM_SERIAL.flush();

  return true;
}

static bool GSM_Send_Command(char *cmd) {
  String dummy_s = GSM_SERIAL.readString();

  uint8_t fullCommandLength = strlen(cmd) + 1 + 1; // 1 is \r and 1 is EOF
  char *bufferCommandFull = (char *)malloc(fullCommandLength);
  snprintf(bufferCommandFull, fullCommandLength, "%s\r", cmd);

  GSM_Send(bufferCommandFull);

  char *bufferReadBack = (char *)malloc(fullCommandLength);
  // if (HAL_UART_Receive(&huart1, (uint8_t*)bufferReadBack, strlen(bufferCommandFull), 100) != HAL_OK) {
  if (GSM_Read((uint8_t*) bufferReadBack, strlen(bufferCommandFull), 100) != 0) {
    GSM_LOG_E("Wait echo command %s timeout", cmd);
    free(bufferCommandFull);
    free(bufferReadBack);
    return false;
  }
  bufferReadBack[fullCommandLength - 1] = 0; // EOF

  if (strcmp(bufferReadBack, bufferCommandFull) != 0) {
    GSM_LOG_E("Command not match !, %s != %s", bufferCommandFull, bufferReadBack);
    free(bufferCommandFull);
    free(bufferReadBack);
    return false;
  }

  free(bufferCommandFull);
  free(bufferReadBack);

  GSM_LOG_I("Send %s OK!", cmd);

  return true;
}

static bool GSM_Send_WaitEcho(char *cmd) {
  String dummy_s = GSM_SERIAL.readString();

  uint8_t fullCommandLength = strlen(cmd) + 1; // 1 is EOF
  char *bufferCommandFull = (char *)malloc(fullCommandLength);
  snprintf(bufferCommandFull, fullCommandLength, "%s", cmd);

  GSM_Send(bufferCommandFull);

  char *bufferReadBack = (char *)malloc(fullCommandLength);
  // if (HAL_UART_Receive(&huart1, (uint8_t*)bufferReadBack, strlen(bufferCommandFull), 100) != HAL_OK) {
  if (GSM_Read((uint8_t*) bufferReadBack, strlen(bufferCommandFull), 100) != 0) {
    GSM_LOG_E("Wait echo command %s timeout", cmd);
    free(bufferCommandFull);
    free(bufferReadBack);
    return false;
  }
  bufferReadBack[fullCommandLength - 1] = 0; // EOF

  if (strcmp(bufferReadBack, bufferCommandFull) != 0) {
    GSM_LOG_E("Command not match !, %s != %s", bufferCommandFull, bufferReadBack);
    free(bufferCommandFull);
    free(bufferReadBack);
    return false;
  }

  free(bufferCommandFull);
  free(bufferReadBack);

  GSM_LOG_I("Send %s OK!", cmd);

  return true;
}

static bool GSM_Wait(char *str, uint32_t timeout) {
  uint8_t len = strlen(str);
  char *bufferRead = (char *)malloc(sizeof(char) * (len + 1));
  if (GSM_Read((uint8_t*) bufferRead, len, timeout) != 0) {
    GSM_LOG_E("Wait %s timeout", str);
    free(bufferRead);
    return false;
  }
  bufferRead[len] = 0; // EOF

  if (strcmp(bufferRead, str) != 0) {
    GSM_LOG_E("Command not send %s but send %s", str, bufferRead);
    free(bufferRead);
    return false;
  }

  free(bufferRead);

  return true;
}

static bool GSM_Send_CommandWaitOK(char *cmd) {
  if (!GSM_Send_Command(cmd)) {
    return false;
  }

  if (!GSM_Wait("\r\nOK\r\n", 500)) {
    return false;
  }

  GSM_LOG_I("Send %s found \"OK\"!", cmd);

  return true;
}

static bool GSM_Send_CommandWaitOKWithTimeOut(char *cmd, uint32_t timeout) {
  if (!GSM_Send_Command(cmd)) {
    return false;
  }

  if (!GSM_Wait("\r\nOK\r\n", timeout)) {
    return false;
  }

  GSM_LOG_I("Send %s found \"OK\"!", cmd);

  return true;
}

bool GSM_AT() {
  GSM_LOG_I("Test AT command...");
  if (!GSM_Send_CommandWaitOK("AT")) {
    GSM_LOG_I("ERROR, go to Power ON");
    return false;
  }

  GSM_LOG_I("OK !");
  return true;  
}

/* User API */
bool GSM_init(int rx_pin, int tx_pin, int power_on_pin) {
  static bool init_serial = false;
  if (!init_serial) {
    GSM_SERIAL.setTimeout(100);
    GSM_SERIAL.begin(115200, SERIAL_8N1, rx_pin, tx_pin);
    init_serial = true;
    delay(50);
  }

  pinMode(power_on_pin, OUTPUT);

  for (int i=0;i<5;i++) {
    // Test via AT
    GSM_LOG_I("Test AT command (1)...");
    if (GSM_Send_CommandWaitOK("AT")) {
      GSM_LOG_I("OK !");
      return true;
    }
    GSM_LOG_I("ERROR, go to Power ON");
    delay(100);
  }

  // Turn ON EC-21
  digitalWrite(power_on_pin, 0);
  delay(100);
  digitalWrite(power_on_pin, 1);
  delay(800);
  digitalWrite(power_on_pin, 0);
  delay(1000); // Wait Boot

  while (GSM_SERIAL.available()) (void)GSM_SERIAL.read();

  GSM_LOG_I("Wait Ready... (Max 30s)");
  if (!GSM_Wait("\r\nRDY\r\n", 30 * 1000)) { // Max wait 30s
    GSM_LOG_I("Timeout, Go back to Restart");
    // goto BACK_TO_START;
  }

  GSM_LOG_I("Ready !");

  GSM_LOG_I("Wait CPIN Ready... (Max 30s)");
  if (!GSM_Wait("\r\n+CPIN: READY\r\n", 5 * 1000)) { // Max wait 5s
    GSM_LOG_I("Timeout, Go back to Restart");
    // goto BACK_TO_START;
  }

  GSM_LOG_I("CPIN Ready !");

  GSM_LOG_I("Wait SMS Ready... (Max 30s)");
  if (!GSM_Wait("\r\nSMS DONE\r\n", 5 * 1000)) { // Max wait 5s
    GSM_LOG_I("Timeout, Go back to Restart");
    // goto BACK_TO_START;
  }

  GSM_LOG_I("SMS Ready !");

  GSM_LOG_I("Wait PB Ready... (Max 30s)");
  if (!GSM_Wait("\r\nPB DONE\r\n", 5 * 1000)) { // Max wait 5s
    GSM_LOG_I("Timeout, Go back to Restart");
    // goto BACK_TO_START;
  }

  GSM_LOG_I("PB Ready !");

  // Test via AT
  GSM_LOG_I("Test AT command (2)...");
  if (!GSM_Send_CommandWaitOK("AT")) {
    GSM_LOG_I("FAIL, Go back to Restart");
    return false;
  }
  GSM_LOG_I("OK !");

  return true;
}

bool GSM_internet_configs() {
  uint8_t error = 0;

  GSM_LOG_I("> Internet Configs");
  do {
    GSM_LOG_I("Get Network Info...");
    if (!GSM_Send_Command("AT+CREG?")) {
      GSM_LOG_I("FAIL (1), wait try...");
      error++;
      delay(500);
      continue;
    }

    if (!GSM_Wait("\r\n+CREG: ", 500)) {
      GSM_LOG_I("FAIL (2), wait try...");
      error++;
      delay(500);
      continue;
    }

    {
      char *networkInfoBuffer = (char *)malloc(sizeof(char) * 30);
      if (!GSM_ReadEndWiths(networkInfoBuffer, 30, "\r\n", 100)) {
        GSM_LOG_I("FAIL (3), wait try...");
        error++;
        free(networkInfoBuffer);
        delay(500);
        continue;
      }

      int n = 0;
      int state = 0;
      sscanf(networkInfoBuffer, "%d,%d", &n, &state);
      free(networkInfoBuffer);

      GSM_LOG_I("State: %d", state);

      if (state != 1) { // Check registered, home network
        GSM_LOG_I("FAIL (4), wait try...");
        error++;
        free(networkInfoBuffer);
        delay(500);
        continue;
      }
    }
    GSM_LOG_I("OK !");

    break;
  } while (error < GSM_MAX_TRY_COMMAND);
  if (error >= GSM_MAX_TRY_COMMAND) {
    GSM_LOG_I("Check network ready FAIL, Go back to Restart");
    return false;
  }

  return true;
}

bool GSM_internet_is_connected(bool *net) {
  uint8_t error = 0;

  GSM_LOG_I("> Internet Configs");
  do {
    GSM_LOG_I("Get Network Info...");
    if (!GSM_Send_Command("AT+CREG?")) {
      GSM_LOG_I("FAIL (1), wait try...");
      error++;
      delay(500);
      continue;
    }

    if (!GSM_Wait("\r\n+CREG: ", 500)) {
      GSM_LOG_I("FAIL (2), wait try...");
      error++;
      delay(500);
      continue;
    }

    {
      char *networkInfoBuffer = (char *)malloc(sizeof(char) * 30);
      if (!GSM_ReadEndWiths(networkInfoBuffer, 30, "\r\n", 100)) {
        GSM_LOG_I("FAIL (3), wait try...");
        error++;
        free(networkInfoBuffer);
        delay(500);
        continue;
      }

      int n = 0;
      int state = 0;
      sscanf(networkInfoBuffer, "%d,%d", &n, &state);
      free(networkInfoBuffer);

      GSM_LOG_I("State: %d", state);

      *net = state == 1;
    }
    GSM_LOG_I("OK !");

    break;
  } while (error < GSM_MAX_TRY_COMMAND);
  if (error >= GSM_MAX_TRY_COMMAND) {
    GSM_LOG_I("Check network ready FAIL, Go back to Restart");
    return false;
  }

  return true;
}

/*
bool GSM_internet_is_connected(bool *net) {
  uint8_t error = 0;

  GSM_LOG_I("> Internet Configs");
  do {
    GSM_LOG_I("Get Network Info...");
    if (!GSM_Send_Command("AT+CGDCONT?")) {
      GSM_LOG_I("FAIL (1), wait try...");
      error++;
      delay(500);
      continue;
    }

    if (!GSM_Wait("\r\n+CGDCONT: ", 500)) {
      GSM_LOG_I("FAIL (2), wait try...");
      error++;
      delay(500);
      continue;
    }

    {
      char *networkInfoBuffer = (char *)malloc(sizeof(char) * 30);
      if (!GSM_ReadEndWiths(networkInfoBuffer, 30, "\r\n", 100)) {
        GSM_LOG_I("FAIL (3), wait try...");
        error++;
        free(networkInfoBuffer);
        delay(500);
        continue;
      }

      GSM_LOG_I("Network info: %s", networkInfoBuffer);

      int n = 0;
      char PDP_type[10];
      char APN[20];
      char IP_Address[30];

      memset(PDP_type, 0, sizeof(PDP_type));
      memset(APN, 0, sizeof(APN));
      memset(IP_Address, 0, sizeof(IP_Address));
      
      sscanf(networkInfoBuffer, "%d,\"%s\",\"%s\",\"%s\"", &n, PDP_type, APN, IP_Address);
      free(networkInfoBuffer);

      GSM_LOG_I("Network IP: %s", IP_Address);

      *net = strlen(IP_Address) > 0;
    }
    GSM_LOG_I("OK !");

    break;
  } while (error < GSM_MAX_TRY_COMMAND);
  if (error >= GSM_MAX_TRY_COMMAND) {
    GSM_LOG_I("Check network ready FAIL, Go back to Restart");
    return false;
  }

  return true;
}
*/

bool GSM_ssl_configs() {
  GSM_LOG_I("> SSL/TLS Configs");
  uint8_t error = 0;
  do {
    GSM_LOG_I("Config SNI...");
    if (!GSM_Send_CommandWaitOK("AT+CSSLCFG=\"enableSNI\",0,1")) {
      GSM_LOG_I("FAIL, wait try...");
      error++;
      delay(500);
      continue;
    }
    GSM_LOG_I("OK !");

    break;
  } while (error < GSM_MAX_TRY_COMMAND);
  if (error >= GSM_MAX_TRY_COMMAND) {
    GSM_LOG_I("SSL/TLS Configs FAIL, Go back to Restart");
    // goto BACK_TO_START;
    return false;
  }

  return true;
}

bool GSM_send_http_get(char *url) {
  GSM_LOG_I("> Send HTTP Get");
  uint8_t error = 0;
  do {
    GSM_LOG_I("Init HTTP...");
    if (!GSM_Send_CommandWaitOK("AT+HTTPINIT")) {
      /*GSM_LOG_I("FAIL, wait try...");
      error++;
      delay(500);
      continue;*/
    }
    GSM_LOG_I("OK !");

    GSM_LOG_I("Config URL...");
    {
      char commandConfigURL[256];
      snprintf(commandConfigURL, 256, "AT+HTTPPARA=\"URL\",\"%s\"", url);
      if (!GSM_Send_CommandWaitOK(commandConfigURL)) {
        GSM_LOG_I("FAIL (1), wait try...");
        error++;
        delay(500);
        continue;
      }
    }
    GSM_LOG_I("OK !");

    GSM_LOG_I("Send HTTP Get...");
    if (!GSM_Send_CommandWaitOK("AT+HTTPACTION=0")) {
      GSM_LOG_I("FAIL, wait try...");
      error++;
      delay(500);
      continue;
    }
    GSM_LOG_I("OK !");

    GSM_LOG_I("Wait reply...");
    if (!GSM_Wait("\r\n+HTTPACTION: ", 80 * 1000)) {
      GSM_LOG_I("FAIL (1), wait try...");
      error++;
      delay(500);
      continue;
    }

    uint16_t reply_method_code = 0;
    uint16_t reply_code = 0;
    uint16_t reply_size = 0;
    {
      char *getInfoBuffer = (char *)malloc(sizeof(char) * 20);
      if (!GSM_ReadEndWiths(getInfoBuffer, 20, "\r\n", 100)) {
        GSM_LOG_I("FAIL (2), wait try...");
        error++;
        free(getInfoBuffer);
        delay(500);
        continue;
      }
      GSM_LOG_I("I got: %s", getInfoBuffer);
      uint8_t state = 0;
      char *token = strtok(getInfoBuffer, ",");
      while (token != NULL) {
        if (state == 0) {
          reply_method_code = atoi(token);
        } else if (state == 1) {
          reply_code = atoi(token);
        } else if (state == 2) {
          reply_size = atoi(token);
        }
        state++;
        token = strtok(NULL, ",");
      }
      free(getInfoBuffer);
    }
    if (reply_method_code != 0) { // Check GET method
      GSM_LOG_I("FAIL (3) METHOD ERROR %d, wait try...", reply_method_code);
      error++;
      delay(500);
      continue;
    }
    if (reply_code >= 701) { // Network error
      GSM_LOG_I("FAIL (4) HTTP ERROR %d (NETWORK ERROR), wait try...");
      error++;
      delay(500);
      continue;
    }
    if (reply_code != 200) {
      GSM_LOG_I("FAIL (4) HTTP ERROR %d, wait try...", reply_code);
      /*error++;
      delay(500);
      continue;*/
    }
    GSM_LOG_I("OK !, Content Length: %d", reply_size);

    GSM_LOG_I("Wait disconnect...");
    if (!GSM_Wait("\r\n+HTTP_PEER_CLOSED\r\n", 30 * 1000)) {
      GSM_LOG_I("FAIL HTTP not disconnect in 30 sec, wait try...", reply_code);
      error++;
      delay(500);
      continue;
    }
    GSM_LOG_I("OK !");

    GSM_LOG_I("Read reply...");
    if (!GSM_Send_CommandWaitOK("AT+HTTPREAD=0,80")) { // Read only 80 byte
      GSM_LOG_I("FAIL (1), wait try...");
      error++;
      delay(500);
      continue;
    }
    if (!GSM_Wait("\r\n+HTTPREAD: DATA,", 500)) {
      GSM_LOG_I("FAIL (2), wait try...");
      error++;
      delay(500);
      continue;
    }
    {
      char *dataSizeBuffer = (char *)malloc(sizeof(char) * 20);
      if (!GSM_ReadEndWiths(dataSizeBuffer, 20, "\r\n", 500)) {
        GSM_LOG_I("FAIL (3), wait try...");
        error++;
        delay(500);
        continue;
      }
      GSM_LOG_I("Content reply size: %d", atoi(dataSizeBuffer));
      free(dataSizeBuffer);
    }

    {
      char *dataReplyBuffer = (char *)malloc(sizeof(char) * 100);
      memset(dataReplyBuffer, 0, 100);
      char *endOfRead = "\r\n+HTTPREAD:0\r\n";
      if (!GSM_ReadEndWiths(dataReplyBuffer, 512, endOfRead, 80 * 1000)) {
        GSM_LOG_I("FAIL (4), wait try...");
        error++;
        delay(500);
        continue;
      }
      memset(&dataReplyBuffer[strlen(dataReplyBuffer) - strlen(endOfRead)], 0, strlen(endOfRead)); // Remove END of READ

      GSM_LOG_I("Content: %s", dataReplyBuffer);
      free(dataReplyBuffer);
    }
    GSM_LOG_I("OK !");

    break;
  } while (error < GSM_MAX_TRY_COMMAND);
  if (error >= GSM_MAX_TRY_COMMAND) {
    GSM_LOG_I("Send HTTP GET FAIL, Go back to Restart");
    // goto BACK_TO_START;
    return false;
  }
  
  return true;
}

bool GSM_send_http_post(char *url, char* data, char* dataOut) {
  GSM_LOG_I("> Send HTTP POST");
  uint8_t error = 0;
  do {
    GSM_LOG_I("Init HTTP...");
    if (!GSM_Send_CommandWaitOK("AT+HTTPINIT")) {
      /*GSM_LOG_I("FAIL, wait try...");
      error++;
      delay(500);
      continue;*/
    }
    GSM_LOG_I("OK !");

    GSM_LOG_I("Config URL...");
    {
      char commandConfigURL[256];
      snprintf(commandConfigURL, 256, "AT+HTTPPARA=\"URL\",\"%s\"", url);
      if (!GSM_Send_CommandWaitOK(commandConfigURL)) {
        GSM_LOG_I("FAIL (1), wait try...");
        error++;
        delay(500);
        continue;
      }
    }
    GSM_LOG_I("OK !");

    GSM_LOG_I("Config Header...");
    if (!GSM_Send_CommandWaitOK("AT+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\"")) {
      GSM_LOG_I("FAIL (1), wait try...");
      error++;
      delay(500);
      continue;
    }
#ifdef TYPE_SIM7600E
    // Not work with A7600E-H
    if (!GSM_Send_CommandWaitOK("AT+HTTPPARA=\"UA\",\"SIM7600E by ArtronShop\"")) {
      GSM_LOG_I("FAIL (2), wait try...");
      error++;
      delay(500);
      continue;
    }
    GSM_LOG_I("OK !");
#endif
    GSM_LOG_I("Config Payload...");
    {
      char commandConfigPayload[50];
      snprintf(commandConfigPayload, 50, "AT+HTTPDATA=%d,10", strlen(data));
      if (!GSM_Send_Command(commandConfigPayload)) {
        GSM_LOG_I("FAIL (1), wait try...");
        error++;
        delay(500);
        continue;
      }
    }
    // For SIM7600E-H
#ifdef TYPE_SIM7600E
    if (!GSM_Wait("\r\nDOWNLOAD\r\n", 500)) {
      GSM_LOG_I("FAIL (2), wait try...");
      error++;
      delay(500);
      continue;
    }
#else
    // For A7600E-H
    if (!GSM_Wait("\r\n\r\nDOWNLOAD\r\n", 500)) {
      GSM_LOG_I("FAIL (2), wait try...");
      error++;
      delay(500);
      continue;
    }
#endif
    if (!GSM_Send(data)) {
      GSM_LOG_I("FAIL (3), wait try...");
      error++;
      delay(500);
      continue;
    }
    // For SIM7600E-H
#ifdef TYPE_SIM7600E
    if (!GSM_Wait(data, 500)) {
      GSM_LOG_I("FAIL (4), wait try...");
      error++;
      delay(500);
      continue;
    }
#endif
    if (!GSM_Wait("\r\nOK\r\n", 3 * 1000)) { // 3s for A7600E-H
      GSM_LOG_I("FAIL (5), wait try...");
      error++;
      delay(500);
      continue;
    }
    GSM_LOG_I("OK !");

    GSM_LOG_I("Send HTTP Post...");
    if (!GSM_Send_CommandWaitOK("AT+HTTPACTION=1")) {
      GSM_LOG_I("FAIL, wait try...");
      error++;
      delay(500);
      continue;
    }
    GSM_LOG_I("OK !");

    GSM_LOG_I("Wait reply...");
    if (!GSM_Wait("\r\n+HTTPACTION: ", 80 * 1000)) {
      GSM_LOG_I("FAIL (1), wait try...");
      error++;
      delay(500);
      continue;
    }

    uint16_t reply_method_code = 0;
    uint16_t reply_code = 0;
    uint16_t reply_size = 0;
    {
      char *getInfoBuffer = (char *)malloc(sizeof(char) * 20);
      if (!GSM_ReadEndWiths(getInfoBuffer, 20, "\r\n", 100)) {
        GSM_LOG_I("FAIL (2), wait try...");
        error++;
        free(getInfoBuffer);
        delay(500);
        continue;
      }
      GSM_LOG_I("I got: %s", getInfoBuffer);
      uint8_t state = 0;
      char *token = strtok(getInfoBuffer, ",");
      while (token != NULL) {
        if (state == 0) {
          reply_method_code = atoi(token);
        } else if (state == 1) {
          reply_code = atoi(token);
        } else if (state == 2) {
          reply_size = atoi(token);
        }
        state++;
        token = strtok(NULL, ",");
      }
      free(getInfoBuffer);
    }
    if (reply_method_code != 1) { // Check POST method
      GSM_LOG_I("FAIL (3) METHOD ERROR %d, wait try...", reply_method_code);
      error++;
      delay(500);
      continue;
    }
    if (reply_code >= 701) { // Network error
      GSM_LOG_I("FAIL (4) HTTP ERROR %d (NETWORK ERROR), wait try...", reply_code);
      error++;
      delay(500);
      continue;
    }
    if (reply_code != 200) {
      GSM_LOG_I("FAIL (5) HTTP ERROR %d, wait try...", reply_code);
      /*error++;
      delay(500);
      continue;*/
    }
    GSM_LOG_I("OK !, Content Length: %d", reply_size);

    GSM_LOG_I("Wait disconnect...");
    if (!GSM_Wait("\r\n+HTTP_PEER_CLOSED\r\n", 1 * 1000)) {
      GSM_LOG_I("FAIL HTTP not disconnect in 1 sec", reply_code);
      /* error++;
      delay(500);
      continue; */
    }
    GSM_LOG_I("OK !");

    GSM_LOG_I("Read reply...");
    if (!GSM_Send_CommandWaitOK("AT+HTTPREAD=0,80")) { // Read only 80 byte
      GSM_LOG_I("FAIL (1), wait try...");
      error++;
      delay(500);
      continue;
    }
    /*if (!GSM_Wait("\r\n+HTTPREAD: DATA,", 500)) {*/  // For SIM7600E-H
    if (!GSM_Wait("\r\n+HTTPREAD: ", 500)) { // For A7600E-H
      GSM_LOG_I("FAIL (2), wait try...");
      error++;
      delay(500);
      continue;
    }
    {
      char *dataSizeBuffer = (char *)malloc(sizeof(char) * 20);
      if (!GSM_ReadEndWiths(dataSizeBuffer, 20, "\r\n", 500)) {
        GSM_LOG_I("FAIL (3), wait try...");
        error++;
        delay(500);
        continue;
      }
      GSM_LOG_I("Content reply size: %d", atoi(dataSizeBuffer));
      free(dataSizeBuffer);
    }

    {
      char *dataReplyBuffer = (char *)malloc(sizeof(char) * 100);
      memset(dataReplyBuffer, 0, 100);
      char *endOfRead = "\r\n+HTTPREAD: 0\r\n";
      if (!GSM_ReadEndWiths(dataReplyBuffer, 100, endOfRead, 500)) {
        GSM_LOG_I("FAIL (4), wait try...");
        error++;
        delay(500);
        continue;
      }
      memset(&dataReplyBuffer[strlen(dataReplyBuffer) - strlen(endOfRead)], 0, strlen(endOfRead)); // Remove END of READ

      GSM_LOG_I("Content: %s", dataReplyBuffer);
      strcpy(dataOut, dataReplyBuffer);
      free(dataReplyBuffer);
    }
    GSM_LOG_I("OK !");

    break;
  } while (error < GSM_MAX_TRY_COMMAND);
  if (error >= GSM_MAX_TRY_COMMAND) {
    GSM_LOG_I("Send HTTP POST FAIL, Go back to Restart");
    // goto BACK_TO_START;
    return false;
  }

  return true;
}

bool GSM_get_imei(char *imei) {
  uint8_t error = 0;

  do {
    GSM_LOG_I("Get IMEI...");
    if (!GSM_Send_Command("AT+CGSN")) {
      GSM_LOG_I("FAIL (1), wait try...");
      error++;
      delay(500);
      continue;
    }

    {
      char *imeiReadBuffer = (char *)malloc(sizeof(char) * 30);
      char *endOfFind = "\r\n\r\nOK\r\n";
      if (!GSM_ReadEndWiths(imeiReadBuffer, 30, endOfFind, 300)) {
        GSM_LOG_I("FAIL (2), wait try...");
        error++;
        free(imeiReadBuffer);
        delay(500);
        continue;
      }

      strncpy(imei, &imeiReadBuffer[2], strlen(imeiReadBuffer) - strlen(endOfFind) - 2);
      free(imeiReadBuffer);
    }
    GSM_LOG_I("OK !, %s", imei);

    break;
  } while (error < GSM_MAX_TRY_COMMAND);
  if (error >= GSM_MAX_TRY_COMMAND) {
    GSM_LOG_I("Get IMEI FAIL, Go back to Restart");
    return false;
  }

  return true;
}

bool GSM_get_local_datetime(int *year, int *month, int *day, int *hour, int *minute, int *second) {
  uint8_t error = 0;

  error = 0;
  do {
    GSM_LOG_I("Config NTP server...");
    if (!GSM_Send_CommandWaitOK("AT+CNTP=\"th.pool.ntp.org\",28")) {
      GSM_LOG_I("FAIL, wait try...");
      error++;
      delay(500);
      continue;
    }
    GSM_LOG_I("OK !");

    GSM_LOG_I("Update time from NTP server...");
    if (!GSM_Send_CommandWaitOK("AT+CNTP")) {
      GSM_LOG_I("FAIL (1), wait try...");
      error++;
      delay(500);
      continue;
    }
    if (!GSM_Wait("\r\n+CNTP: ", 10 * 1000)) {
      GSM_LOG_I("FAIL (2), wait try...");
      error++;
      delay(500);
      continue;
    }
    {
      char *ntpStatusBuffer = (char *)malloc(sizeof(char) * 10);
      if (!GSM_ReadEndWiths(ntpStatusBuffer, 10, "\r\n", 100)) {
        GSM_LOG_I("FAIL (3), wait try...");
        error++;
        free(ntpStatusBuffer);
        delay(500);
        continue;
      }
      int status = atoi(ntpStatusBuffer);
      free(ntpStatusBuffer);
      if (status != 0) {
        GSM_LOG_I("FAIL (4) NTP fail code %d, wait try...", status);
        error++;
        free(ntpStatusBuffer);
        delay(500);
        continue;
      }
    }

    GSM_LOG_I("OK !");

    GSM_LOG_I("Get datetime from RTC...");
    if (!GSM_Send_Command("AT+CCLK?")) {
      GSM_LOG_I("FAIL (1), wait try...");
      error++;
      delay(500);
      continue;
    }

    if (!GSM_Wait("\r\n+CCLK: \"", 300)) {
      GSM_LOG_I("FAIL (2), wait try...");
      error++;
      delay(500);
      continue;
    }

    {
      char *datetimeInfoBuffer = (char *)malloc(sizeof(char) * 50);
      if (!GSM_ReadEndWiths(datetimeInfoBuffer, 50, "\r\n", 100)) {
        GSM_LOG_I("FAIL (3), wait try...");
        error++;
        free(datetimeInfoBuffer);
        delay(500);
        continue;
      }

      sscanf(datetimeInfoBuffer, "%d/%d/%d,%d:%d:%d", year, month, day, hour, minute, second);
      free(datetimeInfoBuffer);

      GSM_LOG_I("DateTime: %d/%d/%d %d:%02d:%02d", *day, *month, *year, *hour, *minute, *second);
    }
    GSM_LOG_I("OK !");

    break;
  } while (error < GSM_MAX_TRY_COMMAND);
  if (error >= GSM_MAX_TRY_COMMAND) {
    GSM_LOG_I("Get datetime FAIL, Go back to Restart");
    return false;
  }

  return true;
}

bool gnssIsOn = false;

bool GSM_gnss_setup() {
  uint8_t error = 0;
    do {
      if (!gnssIsOn) {
        /*
        GSM_LOG_I("Config NMA output uartdebug...");
        if (!GSM_Send_CommandWaitOK("AT+QGPSCFG=\"outport\",\"uartdebug\"")) {
          GSM_LOG_I("FAIL, wait try...");
          error++;
          delay(500);
          continue;
        }
        GSM_LOG_I("OK !");
        */

        GSM_LOG_I("Turn on GNSS...");
        if (!GSM_Send_CommandWaitOK("AT+CGPS=1")) {
          GSM_LOG_I("FAIL, wait try...");
          error++;
          delay(500);
          continue;
        }
        GSM_LOG_I("OK !");
        gnssIsOn = true;
      }

      break;
    } while (error < GSM_MAX_TRY_COMMAND);
    if (error >= GSM_MAX_TRY_COMMAND) {
      GSM_LOG_I("Get location FAIL, Go back to Restart");
      return false;
    }

    return true;
}

bool GSM_get_location(bool *fixed, float *lat, float *lng) {
  *fixed = false;

  uint8_t error = 0;
  do {
    if (!gnssIsOn) {
      if (!GSM_gnss_setup()) {
        return false;
      }
    }

    GSM_LOG_I("Get location...");
    if (!GSM_Send_Command("AT+CGPSINFO")) {
      GSM_LOG_I("FAIL (1), wait try...");
      error++;
      delay(500);
      continue;
    }

    {
      char *posInfoBuffer = (char *)malloc(sizeof(char) * 200);
      memset(posInfoBuffer, 0, 200);
      if (!GSM_Wait("\r\n", 300)) {
        GSM_LOG_I("FAIL (2), wait try...");
        error++;
        free(posInfoBuffer);
        delay(500);
        continue;
      }

      if (!GSM_ReadEndWiths(posInfoBuffer, 200, "\r\n", 100)) {
        GSM_LOG_I("FAIL (3), wait try...");
        error++;
        free(posInfoBuffer);
        delay(500);
        continue;
      }

      GSM_LOG_I("Out :%s", posInfoBuffer);

      char *ok_found_str = "+CGPSINFO:";
      if (strncmp(posInfoBuffer, ok_found_str, strlen(ok_found_str)) != 0) {
        GSM_LOG_I("FAIL (5), ERROR Unknow : %s", posInfoBuffer);
        free(posInfoBuffer);
        break;
      }

      char ns = 0, we = 0;
      /*double tmp_lat = 0.0;
      double tmp_lng = 0.0;
      {
        char lat_s[20], lng_s[20];
        memset(lat_s, 0, 20);
        memset(lng_s, 0, 20);
        uint8_t char_n = strlen(ok_found_str) + 1;
        uint8_t i = 0;
        uint8_t lat_s_i = 0;
        uint8_t lng_s_i = 0;
        do {
          char c = posInfoBuffer[char_n++];
          if (c == ',') {
            i++;
            continue;
          }

          if (i == 0) {
            if (lat_s_i < 20) {
              lat_s[lat_s_i++] = c;
            }
          } else if (i == 1) {
            ns = c;
          } else if (i == 2) {
            if (lng_s_i < 20) {
              lng_s[lng_s_i++] = c;
            }
          } else if (i == 3) {
            we = c;
          }
        } while(i < 4);

        strcpy(lat_s, "10");
        strcpy(lng_s, "20");

        GSM_LOG_I("%s, %s, %c, %c", lat_s, lng_s, ns, we);

        if (strlen(lat_s) == 0 || strlen(lng_s) == 0) {
          GSM_LOG_I("FAIL (4), GNSS not fix");
          free(posInfoBuffer);
          break;
        }

        tmp_lat = 10;
        tmp_lng = 30;
      }

      GSM_LOG_I("::: %f, %f", 10.0f, 22.0f);*/

      if (sscanf(posInfoBuffer, "+CGPSINFO: %f,%c,%f,%c,", lat, &ns, lng, &we) <= 0) {
        GSM_LOG_I("FAIL (4), GNSS not fix");
        free(posInfoBuffer);
        break;
      }

      free(posInfoBuffer);

      int lat_deg = (*lat) / 100;
      float lat_min = (*lat) - (lat_deg * 100.0);
      int lng_deg = (*lng) / 100;
      float lng_min = (*lng) - (lng_deg * 100.0);

      *lat = lat_deg + (lat_min / 60.0) * (ns == 'N' ? 1.0 : -1.0);
      *lng = lng_deg + (lng_min / 60.0) * (we == 'E' ? 1.0 : -1.0);

      *fixed = true;

      GSM_LOG_I("Location: %.06f, %.06f", *lat, *lng);
    }
    GSM_LOG_I("OK !");

    break;
  } while (error < GSM_MAX_TRY_COMMAND);
  if (error >= GSM_MAX_TRY_COMMAND) {
    GSM_LOG_I("Get location FAIL, Go back to Restart");
    return false;
  }

  return true;
}

bool GSM_power_down() {
  uint8_t error = 0;
  do {
    GSM_LOG_I("Send power down...");
    if (!GSM_Send_CommandWaitOK("AT+CPOF")) {
      GSM_LOG_I("FAIL, wait try...");
      error++;
      delay(500);
      continue;
    }
    GSM_LOG_I("OK !");

    break;
  } while (error < GSM_MAX_TRY_COMMAND);
  if (error >= GSM_MAX_TRY_COMMAND) {
    GSM_LOG_I("Power down FAIL, Go back to Restart");
    return false;
  }

  return true;
}
