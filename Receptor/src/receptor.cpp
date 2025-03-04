#include <LoRa.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>

#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS 18
#define LORA_RST 14
#define LORA_DI0 26

#define SPREAD_FACTOR 12
#define CODING_RATE 5

// Configurações WiFi
const char* ssid = "Nome da sua rede Wifi";
const char* password = "Senha da sua rede Wifi";

// Configurações ThingSpeak
const String apiKey = "Sua API do the thingSpeak";
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
  Serial.println("\nConectado à WiFi!");

  // Inicializar LoRa
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DI0);
  if (!LoRa.begin(915E6)) {  
    Serial.println("Erro ao iniciar LoRa!");
    while (1);
  }
  Serial.println("LoRa inicializado com sucesso!");
}

void loop() {
// Verificar conexão WiFi
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    Serial.println("Reconectando ao WiFi...");
    delay(5000);
  }

  // Verificar se há um pacote LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    long receivedTimestamp = 0;
    String receivedData = "";
    // Lê o RSSI do último pacote recebido
    //int rssi = LoRa.packetRssi();
    Serial.println("\n====================================================");
    //Serial.print("RSSI: ");
    //Serial.println(rssi);
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }

    receivedTimestamp = receivedData.toInt();
    long currentTime = millis();  // Captura o tempo atual ao receber o pacote
    long uplinkTime = currentTime - receivedTimestamp;  // Calcula o tempo de uplink

    // Exibir dados recebidos
    Serial.println("Dados recebidos:");
    Serial.println(receivedData);
    
    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();

    Serial.print(" | Tempo de Uplink: ");
    Serial.print(uplinkTime);
    Serial.print(" ms | RSSI: ");
    Serial.print(rssi);
    Serial.print(" dBm | SNR: ");
    Serial.print(snr);
    Serial.println(" dB");
    
    // Processar os dados recebidos
    processReceivedData(receivedData, rssi, snr);

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


    // Verificar o código HTTP e resposta do servidor
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.printf("Dados enviados! Código HTTP: %d\nResposta: %s\n", httpCode, payload.c_str());
    } else {
      Serial.printf("Erro no envio: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.println("WiFi desconectado, não foi possível enviar os dados.");
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
    Serial.println("Nível de Risco: " + risco);
    sendToThingSpeak(temperatura.toFloat(), umidade.toFloat(), co2.toInt(), chama.toInt(), risco.toInt(), rssi, snr);
  } else {
    Serial.println("Formato inesperado dos dados recebidos.");
  }
}

