#include <MQUnifiedsensor.h>
#include "DHT.h"
#include <RadioLib.h>
#include <Fuzzy.h>

// Definição dos pinos SPI para o módulo LoRa
#define LORA_CLK   SCK     // Pino de clock SPI
#define LORA_MISO  MISO    // Pino MISO SPI
#define LORA_MOSI  MOSI    // Pino MOSI SPI
#define LORA_NSS   SS      // Pino de seleção de escravo SPI (Chip Select)

void setFlag(void); 

// Sensores
const int flamePin = 39; // Pino do sensor de chama
const int sampleSize = 10; // Tamanho da amostra para leitura do sensor de chama
const int RelePin = 40; // Pino do relé
#define DHTPIN 41 // Pino do DHT22
#define DHTTYPE DHT22

// Configuração do MQ-135
#define placa "Heltec WiFi LoRa(V3)"
#define Voltage_Resolution 5
#define pin 2 // Pino analógico do MQ-135
#define type "MQ-135"
#define ADC_Bit_Resolution 12
#define RatioMQ135CleanAir 3.6

// Inicialização dos sensores
MQUnifiedsensor MQ135(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);
DHT dht(DHTPIN, DHTTYPE);

// Inicialização do LoRa
SX1262 Lora = new Module(8, 14, 12, 13);

// Variáveis de controle do LoRa
volatile bool transmitFlag = false;  // Flag para indicar que um pacote foi recebido
volatile bool enableInterrupt = true; // Habilita/desabilita interrupções

// Lógica Fuzzy
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

// Declaração das funções
void setFlag(void);
bool readFlameSensor();
void criarRegraFuzzy(Fuzzy* fuzzy, int id, FuzzySet* temperatura, FuzzySet* umidade, FuzzySet* co2, FuzzySet* chama, FuzzySet* risco);

void setup() {
  Serial.begin(115200);
  pinMode(flamePin, INPUT);
  pinMode(RelePin, OUTPUT);
  digitalWrite(RelePin, LOW);

  dht.begin();
  MQ135.setRegressionMethod(1);
  MQ135.setA(110.47); MQ135.setB(-2.862);
  MQ135.init();

  Serial.print("Calibrando MQ-135... ");
  float calcR0 = 0;
  for (int i = 1; i <= 10; i++) {
    MQ135.update();
    calcR0 += MQ135.calibrate(RatioMQ135CleanAir);
    Serial.print(".");
  }
  MQ135.setR0(calcR0 / 10);
  Serial.println(" Pronto!");

  // Inicialização do LoRa
  SPI.begin(LORA_CLK, LORA_MISO, LORA_MOSI, LORA_NSS);
  Serial.print("Iniciando LoRa... ");
  int state = Lora.begin(915.0); // Frequência de 915 MHz (ajuste para 868 MHz se necessário)
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Sucesso!");
  } else {
    Serial.print("Falha, código: ");
    Serial.println(state);
    while (true);
  }

  // Configura a interrupção para receber pacotes
  Lora.setDio1Action(setFlag);
  Serial.print("Iniciando recepção... ");
  state = Lora.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Sucesso!");
  } else {
    Serial.print("Falha, código: ");
    Serial.println(state);
    while (true);
  }

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
 // Configuração para alcance máximo
 state = Lora.setSpreadingFactor(7); // SF7
 state = Lora.setCodingRate(5); // CR5
 state = Lora.setOutputPower(22); // 22 dBm

}

void loop() {
  bool flameDetected = readFlameSensor();
  MQ135.update();
  float co2_ppm = MQ135.readSensor();

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

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
  Serial.println("Enviando dados via LoRa...");
  String data = "Temperatura: " + String(temperature) + " *C, CO2: " + String(co2_ppm) + " ppm, Chama: " + String(flameDetected) + ", Umidade: " + String(humidity) + ", Risco: " + String(fireRisk);
    // Envia os dados via LoRa
  int state = Lora.startTransmit(data);
  if (state == RADIOLIB_ERR_NONE) {
      Serial.println("Dados enviados com sucesso!");
  } else {
      Serial.print("Falha ao enviar dados, código: ");
      Serial.println(state);
  }

  delay(10000); // 10 segundos
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
    FuzzyRuleAntecedent* antecedente = new FuzzyRuleAntecedent();
    FuzzyRuleAntecedent* tempUmidade = new FuzzyRuleAntecedent();
    tempUmidade->joinWithAND(temperatura, umidade);
    FuzzyRuleAntecedent* tempUmidadeCO2 = new FuzzyRuleAntecedent();
    tempUmidadeCO2->joinWithAND(tempUmidade, co2);
    antecedente->joinWithAND(tempUmidadeCO2, chama);

    FuzzyRuleConsequent* consequente = new FuzzyRuleConsequent();
    consequente->addOutput(risco);

    FuzzyRule* fuzzyRule = new FuzzyRule(id, antecedente, consequente);
    fuzzy->addFuzzyRule(fuzzyRule);
}

// Função chamada ao receber um pacote LoRa
void setFlag(void) {
  if (!enableInterrupt) return;
  transmitFlag = true;
}
