#include <MQUnifiedsensor.h>
#include "DHT.h"
#include <LoRa.h>
#include <Fuzzy.h>

const int flamePin = 17;
const int sampleSize = 10;

//relé
const int RelePin = 26; // pino ao qual o Módulo Relé está conectado
int incomingByte;      // variavel para ler dados recebidos pela serial

#define DHTPIN 4
#define DHTTYPE DHT22

#define placa "Heltec WiFi LoRa(V2)"
#define Voltage_Resolution 5
#define pin 13 // Analog input pin
#define type "MQ-135"
#define ADC_Bit_Resolution 12
#define RatioMQ135CleanAir 3.6

#define CO2_THRESHOLD 1000

// Pinos SPI para LoRa
#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS 18
#define LORA_RST 14
#define LORA_DI0 26

#define SPREADING_FACTOR 12     
#define CODING_RATE      5  

// Sensores
MQUnifiedsensor MQ135(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);
DHT dht(DHTPIN, DHTTYPE);

Fuzzy *fuzzy = new Fuzzy();

// Conjuntos Fuzzy para Temperatura
FuzzySet *temperaturaBaixa = new FuzzySet(0, 20, 20, 25);
FuzzySet *temperaturaMedia = new FuzzySet(25, 30, 30, 35);
FuzzySet *temperaturaAlta = new FuzzySet(35, 40, 40, 50);

// Conjuntos Fuzzy para Umidade
FuzzySet *umidadeBaixa = new FuzzySet(0, 15, 15, 30);
FuzzySet *umidadeMedia = new FuzzySet(30, 50, 50, 60);
FuzzySet *umidadeAlta = new FuzzySet(60, 75, 75, 100);

// Conjuntos Fuzzy para CO2
FuzzySet *co2Baixo = new FuzzySet(0, 150, 150, 300);
FuzzySet *co2Medio = new FuzzySet(300, 650, 650, 1000);
FuzzySet *co2Alto = new FuzzySet(900, 1500, 1500, 2000);

// Conjuntos Fuzzy para Chama
FuzzySet *semChama = new FuzzySet(0, 0, 0, 1);
FuzzySet *comChama = new FuzzySet(1, 1, 1, 1);

// Conjuntos Fuzzy para Risco de Incêndio
FuzzySet *riscoBaixo = new FuzzySet(0, 15, 15, 30);
FuzzySet *riscoMedio = new FuzzySet(30, 55, 55, 70);
FuzzySet *riscoAlto = new FuzzySet(70, 85, 85, 100);

// Declaração da função readFlameSensor
bool readFlameSensor();
void criarRegraFuzzy(Fuzzy* fuzzy, int id, FuzzySet* temperatura, FuzzySet* umidade, FuzzySet* co2, FuzzySet* chama, FuzzySet* risco);

void setup() {
  Serial.begin(115200);
  pinMode(flamePin, INPUT);

  //relé
  pinMode(RelePin, OUTPUT); // seta o pino como saída
  digitalWrite(RelePin, LOW); // seta o pino com nivel logico baixo

  dht.begin();
  MQ135.setRegressionMethod(1); 
  MQ135.setA(110.47); MQ135.setB(-2.862); 
  MQ135.init();

  Serial.print("Calibrando... ");
  float calcR0 = 0;
  for (int i = 1; i <= 10; i++) {
    MQ135.update();
    calcR0 += MQ135.calibrate(RatioMQ135CleanAir);
    Serial.print(".");
  }
  MQ135.setR0(calcR0 / 10);
  Serial.println(" Pronto!");

  // Inicialização LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DI0);
  
  if (!LoRa.begin(915E6)) {
    Serial.println("Erro ao iniciar LoRa!");
    while (1);
  }
  LoRa.setSpreadingFactor(SPREADING_FACTOR);
  LoRa.setCodingRate4(CODING_RATE);
  
  Serial.println("LoRa iniciado!");

  // Configurar lógica fuzzy
  FuzzyInput *temperatura = new FuzzyInput(1);
  temperatura->addFuzzySet(temperaturaBaixa);
  temperatura->addFuzzySet(temperaturaMedia);
  temperatura->addFuzzySet(temperaturaAlta);
  fuzzy->addFuzzyInput(temperatura);

  FuzzyInput *umidade = new FuzzyInput(2);
  umidade->addFuzzySet(umidadeBaixa);
  umidade->addFuzzySet(umidadeMedia);
  umidade->addFuzzySet(umidadeAlta);
  fuzzy->addFuzzyInput(umidade);

  FuzzyInput *co2 = new FuzzyInput(3);
  co2->addFuzzySet(co2Baixo);
  co2->addFuzzySet(co2Medio);
  co2->addFuzzySet(co2Alto);
  fuzzy->addFuzzyInput(co2);

  FuzzyInput *chama = new FuzzyInput(4);
  chama->addFuzzySet(semChama);
  chama->addFuzzySet(comChama);
  fuzzy->addFuzzyInput(chama);

  FuzzyOutput *risco = new FuzzyOutput(1);
  risco->addFuzzySet(riscoBaixo);
  risco->addFuzzySet(riscoMedio);
  risco->addFuzzySet(riscoAlto);
  fuzzy->addFuzzyOutput(risco);

  criarRegraFuzzy(fuzzy, 1, temperaturaAlta, umidadeAlta, co2Alto, semChama, riscoMedio);
  criarRegraFuzzy(fuzzy, 2, temperaturaAlta, umidadeAlta, co2Alto, comChama, riscoAlto);
  criarRegraFuzzy(fuzzy, 3, temperaturaAlta, umidadeAlta, co2Medio, comChama, riscoAlto);
  criarRegraFuzzy(fuzzy, 4, temperaturaAlta, umidadeAlta, co2Baixo, semChama, riscoBaixo);
  criarRegraFuzzy(fuzzy, 5, temperaturaAlta, umidadeAlta, co2Baixo, comChama, riscoAlto);
  criarRegraFuzzy(fuzzy, 6, temperaturaAlta, umidadeMedia, co2Alto, semChama, riscoAlto);
  criarRegraFuzzy(fuzzy, 7, temperaturaAlta, umidadeMedia, co2Alto, comChama, riscoAlto);
  criarRegraFuzzy(fuzzy, 8, temperaturaAlta, umidadeMedia, co2Medio, semChama, riscoMedio);
  criarRegraFuzzy(fuzzy, 9, temperaturaAlta, umidadeMedia, co2Medio, comChama, riscoAlto);
  criarRegraFuzzy(fuzzy, 10, temperaturaAlta, umidadeMedia, co2Baixo, semChama, riscoMedio);
  criarRegraFuzzy(fuzzy, 11, temperaturaBaixa, umidadeBaixa, co2Baixo, semChama, riscoBaixo);
  criarRegraFuzzy(fuzzy, 12, temperaturaBaixa, umidadeAlta, co2Alto, semChama, riscoMedio);
  criarRegraFuzzy(fuzzy, 13, temperaturaAlta, umidadeBaixa, co2Alto, semChama, riscoAlto);
  criarRegraFuzzy(fuzzy, 14, temperaturaAlta, umidadeBaixa, co2Alto, comChama, riscoAlto);
  criarRegraFuzzy(fuzzy, 15, temperaturaAlta, umidadeAlta, co2Medio, semChama, riscoMedio);
  criarRegraFuzzy(fuzzy, 16, temperaturaBaixa, umidadeMedia, co2Medio, semChama, riscoBaixo);
  criarRegraFuzzy(fuzzy, 17, temperaturaMedia, umidadeAlta, co2Baixo, semChama, riscoBaixo);
  criarRegraFuzzy(fuzzy, 18, temperaturaMedia, umidadeAlta, co2Baixo, comChama, riscoAlto);
  criarRegraFuzzy(fuzzy, 19, temperaturaMedia, umidadeMedia, co2Baixo, comChama, riscoAlto);
  criarRegraFuzzy(fuzzy, 20, temperaturaMedia, umidadeMedia, co2Baixo, semChama, riscoBaixo);
}

void loop() {
  bool flameDetected = readFlameSensor();
  MQ135.update();
  float co2_ppm = MQ135.readSensor();

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  long timestamp = millis(); //captura o tempo atual

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Falha na leitura do DHT22!");
    return;
  }

  // Processamento fuzzy
  fuzzy->setInput(1, temperature);
  fuzzy->setInput(2, humidity);
  fuzzy->setInput(3, co2_ppm);
  fuzzy->setInput(4, flameDetected); // Chama: 1 para Com Chama, 0 para Sem Chama
  fuzzy->fuzzify();

  float fireRisk = fuzzy->defuzzify(1);

  // Exibir dados locais
  Serial.print("Temperatura: "); Serial.print(temperature); Serial.println(" *C");
  Serial.print("Umidade: "); Serial.print(humidity); Serial.println(" *%");
  Serial.print("CO2: "); Serial.print(co2_ppm); Serial.println(" ppm");
  Serial.print("Chama: "); Serial.println(flameDetected);
  Serial.print("Nível de Risco: "); Serial.println(fireRisk);

  // Acionar o relé apenas em caso de risco alto
  if (fireRisk >= 70) { // Considerando 70 como risco alto
    Serial.println("Risco alto detectado! Acionando relé...");
    digitalWrite(RelePin, HIGH); // Liga o relé
  } else {
    Serial.println("Risco baixo ou médio. Mantendo relé desligado.");
    digitalWrite(RelePin, LOW); // Desliga o relé
  }

  // Enviar dados via LoRa
  Serial.println("\n=================================================");
  Serial.println("Enviando dados via Lora...");
  LoRa.beginPacket();
  LoRa.print("Temperatura: "); LoRa.print(temperature);
  LoRa.print(" *C, CO2: "); LoRa.print(co2_ppm);
  LoRa.print(" ppm, Chama: "); LoRa.print(flameDetected);
  LoRa.print(", Umidade: "); LoRa.print(humidity);
  LoRa.print(", Risco: "); LoRa.print(fireRisk);
  LoRa.print(", Time stamp: ");LoRa.println(timestamp);
  LoRa.endPacket();


  delay(10000); //10 segundos
}

bool readFlameSensor() {
  int total = 0;
  for (int i = 0; i < sampleSize; i++) {
    total += digitalRead(flamePin);
    delay(10);
  }
  return (total < sampleSize / 2);
}

void criarRegraFuzzy(Fuzzy* fuzzy, int id, FuzzySet* temperatura, FuzzySet* umidade, FuzzySet* co2, FuzzySet* chama, FuzzySet* risco) {
    // Cria o antecedente da regra
    FuzzyRuleAntecedent* antecedente = new FuzzyRuleAntecedent();
    FuzzyRuleAntecedent* tempUmidade = new FuzzyRuleAntecedent();
    tempUmidade->joinWithAND(temperatura, umidade); // Junta temperatura e umidade
    FuzzyRuleAntecedent* tempUmidadeCO2 = new FuzzyRuleAntecedent();
    tempUmidadeCO2->joinWithAND(tempUmidade, co2); // Adiciona CO2
    antecedente->joinWithAND(tempUmidadeCO2, chama); // Adiciona chama

    // Cria o consequente da regra
    FuzzyRuleConsequent* consequente = new FuzzyRuleConsequent();
    consequente->addOutput(risco); // Define o risco

    // Cria a regra fuzzy
    FuzzyRule* fuzzyRule = new FuzzyRule(id, antecedente, consequente);
    fuzzy->addFuzzyRule(fuzzyRule); // Adiciona a regra ao sistema fuzzy
}