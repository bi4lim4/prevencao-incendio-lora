#include <LoRa.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS 18
#define LORA_RST 14
#define LORA_DI0 26

// Configurações WiFi
const char* ssid = "Nome da sua rede Wifi";
const char* password = "Senha da sua rede Wifi";

// Configurações ThingSpeak
const String apiKey = "Sua API do the thingSpeak";
const String server = "http://api.thingspeak.com/update";

void processReceivedData(const String &data);
void sendToThingSpeak(float temp, float umid, int co2, bool chama, int risco);

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
    delay(5000);
  }

  // Verificar se há um pacote LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedData = "";
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }

    // Exibir dados recebidos
    Serial.println("Dados recebidos:");
    Serial.println(receivedData);

    // Processar os dados recebidos
    processReceivedData(receivedData);
  }
}

void sendToThingSpeak(float temp, float umid, int co2, bool chama, int risco) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    String url = server + "?api_key=" + apiKey +
             "&field1=" + String(temp) +
             "&field2=" + String(umid) +
             "&field3=" + String(co2) +
             "&field4=" + String(chama) +
             "&field5=" + String(risco);


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

void processReceivedData(const String &data) {
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
    sendToThingSpeak(temperatura.toFloat(), umidade.toFloat(), co2.toInt(), chama.toInt(), risco.toInt());
  } else {
    Serial.println("Formato inesperado dos dados recebidos.");
  }
}
