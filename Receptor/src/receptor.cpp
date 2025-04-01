#include <LoRa.h>
#include <RadioLib.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>

// Defini��o dos pinos SPI para o m�dulo LoRa
#define LORA_CLK   SCK     // Pino de clock SPI
#define LORA_MISO  MISO    // Pino MISO SPI
#define LORA_MOSI  MOSI    // Pino MOSI SPI
#define LORA_NSS   SS      // Pino de sele��o de escravo SPI (Chip Select)

SX1262 Lora = new Module(8, 14, 12, 13);

volatile bool transmitFlag = false;  // Flag para indicar que um pacote foi recebido
volatile bool enableInterrupt = true; // Habilita/desabilita interrup��es

// Fun��o chamada ao receber um pacote LoRa
void setFlag(void) {
    if (!enableInterrupt) return; // Ignora se as interrup��es estiverem desabilitadas
    transmitFlag = true; // Sinaliza que um pacote foi recebido
}

// Configura��es WiFi
const char* ssid = "Sua rede WiFi"; // Substitua pelo nome da sua rede WiFi
const char* password = "Sua senha WiFi"; // Substitua pela senha da sua rede WiFi

// Configura��es ThingSpeak
const String apiKey = "Sua API"; // Substitua pela sua API do The Thing Speak
const String server = "http://api.thingspeak.com/update";

void processReceivedData(const String &data, int rssi, float snr);
void sendToThingSpeak(float temp, float umid, int co2, bool chama, int risco, int rssi, float snr);

void setup() {
  Serial.begin(115200);

  // Conectar ao WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado � WiFi!");

  // Inicializar LoRa
  SPI.begin(LORA_CLK, LORA_MISO, LORA_MOSI, LORA_NSS);

   // Inicializa o m�dulo LoRa
   Serial.print("Iniciando LoRa... ");
   int state = Lora.begin(915.0); // Frequ�ncia de 915 MHz (ajuste para 868 MHz se necess�rio)
   if (state == RADIOLIB_ERR_NONE) {
     Serial.println("Sucesso!");
   } else {
     Serial.print("Falha, c�digo: ");
     Serial.println(state);
     while (true); // Trava o programa se o LoRa n�o iniciar
   }
    // Configura��o para alcance m�ximo
    state = Lora.setSpreadingFactor(7); // SF7
    state = Lora.setCodingRate(5); // CR5
    state = Lora.setOutputPower(22); // 22 dBm
 
   // Configura a interrup��o para receber pacotes
   Lora.setDio1Action(setFlag);
   Serial.print("Iniciando recep��o... ");
   state = Lora.startReceive();
   if (state == RADIOLIB_ERR_NONE) {
     Serial.println("Sucesso!");
   } else {
     Serial.print("Falha, c�digo: ");
     Serial.println(state);
     while (true);
   }
 
}

void loop() {
// Verificar conex�o WiFi
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    Serial.println("Reconectando ao WiFi...");
    delay(5000);
  }

  // Verifica se um pacote foi recebido
  if (transmitFlag) {
    transmitFlag = false; // Reseta a flag
    enableInterrupt = false; // Desabilita interrup��es temporariamente

    // L� o pacote recebido
    String receivedData;
    int state = Lora.readData(receivedData);

    if (state == RADIOLIB_ERR_NONE) {
      // Exibe os dados recebidos no monitor serial
      Serial.println("\n====================================================");
      Serial.println("Dados recebidos: " + receivedData);

      // Obt�m a intensidade do sinal RSSI e SNR
      int16_t rssi = Lora.getRSSI();
      float snr = Lora.getSNR();

      // Exibe o RSSI e SNR
      Serial.print("RSSI: ");
      Serial.print(rssi);
      Serial.print(" dBm | SNR: ");
      Serial.print(snr);
      Serial.println(" dB");

      // Processa os dados recebidos
      processReceivedData(receivedData, rssi, snr);
    } else {
      Serial.println("Erro ao receber dados!");
    }

    // Reativa a recep��o de pacotes
    Lora.startReceive();
    enableInterrupt = true; // Reabilita interrup��es
  }
}

void sendToThingSpeak(float temp, float umid, int co2, bool chama, int risco, int rssi, float snr) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    String url = server + "?api_key=" + apiKey +
             "&field1=" + String(temp) +
             "&field2=" + String(umid) +
             "&field3=" + String(co2) +
             "&field4=" + String(chama) +
             "&field5=" + String(risco) +
             "&field6=" + String(rssi) +
             "&field7=" + String(snr);


    http.begin(url);
    int httpCode = http.GET();


    // Verificar o c�digo HTTP e resposta do servidor
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.printf("Dados enviados! C�digo HTTP: %d\nResposta: %s\n", httpCode, payload.c_str());
    } else {
      Serial.printf("Erro no envio: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("WiFi desconectado, n�o foi poss�vel enviar os dados.");
  }
}

void processReceivedData(const String &data, int rssi, float snr) {
  int tempIndex = data.indexOf("Temperatura: ");
  int co2Index = data.indexOf("CO2: ");
  int chamaIndex = data.indexOf("Chama: ");
  int umidadeIndex = data.indexOf("Umidade: ");
  int riscoIndex = data.indexOf("Risco: ");

  if (tempIndex != -1 && co2Index != -1 && chamaIndex != -1 && 
      umidadeIndex != -1 && riscoIndex != -1) {
      
    String temperatura = data.substring(tempIndex + 13, co2Index - 2);
    String co2 = data.substring(co2Index + 5, chamaIndex - 2);
    String chama = data.substring(chamaIndex + 7, umidadeIndex - 1);
    String umidade = data.substring(umidadeIndex + 9, riscoIndex - 1);
    String risco = data.substring(riscoIndex + 7);

    Serial.println("Temperatura: " + temperatura + " *C");
    Serial.println("CO2: " + co2 + " ppm");
    Serial.println("Chama: " + chama);
    Serial.println("Umidade: " + umidade + " %");
    Serial.println("N�vel de Risco: " + risco);
    sendToThingSpeak(temperatura.toFloat(), umidade.toFloat(), co2.toInt(), chama.toInt(), risco.toInt(), rssi, snr);
  } else {
    Serial.println("Formato inesperado dos dados recebidos.");
  }
}
 
