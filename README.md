# Sistema de Prevenção e Monitoramento de Incêndios com LoRa

Este projeto visa prevenir e monitorar incêndios na agricultura familiar utilizando sensores de temperatura, umidade, CO2 e chama, com comunicação via LoRa. O sistema é composto por um transmissor (coleta de dados) e um receptor (processamento e alertas).

## Funcionalidades do Transmissor
  - Coleta dados de temperatura, umidade, CO2 e chama.
  - Utiliza Lógica Fuzzy para processamento dos dados e cálculo do nível de risco de incêndio.
  - Envia os dados via LoRa para o receptor.

## Funcionalidades do Receptor
  - Recebe os dados via LoRa.
  - Processa os dados e os envia para a plataforma Thing Speak utilizando conexão Wi-Fi.
  **Conexão WiFi**:
    - O receptor se conecta a uma rede Wi-Fi configurada no código (ssid e password).
  **Envio de Dados para o ThingSpeak**:
    - Os dados recebidos via LoRa são enviados para a plataforma ThingSpeak
    - A URL de envio é construída dinamicamente com a chave da API (apiKey) e os campos correspondentes:
      - field1: temperatura
      - field2: umidade
      - field3: CO2
      - field4: chama (0 ou 1)
      - field5: Nível de risco
      - field6: RSSI (intensidade do sinal LoRa)
      - field7: SNR (relação sinal-ruído do sinal LoRa)
  - O envio é feito via HTTP GET, e o código de resposta é verificado para garantir que os dados foram enviados com sucesso.
  
## Tecnologias Utilizadas
- **Hardware**:
  - ESP32 WiFi LoRa
  - Sensor DHT22 (temperatura e umidade)
  - Sensor MQ-135 (CO2)
  - Sensor de chama
  - Módulo relé

- **Software**:
  - PlatformIO (IDE para desenvolvimento)
  - Biblioteca LoRa
  - Biblioteca Fuzzy
  - Plataforma ThingSpeak para visualização e monitoramento dos dados em tempo real.

## Estrutura do Projeto
- **Transmissor**: Código e documentação do dispositivo que coleta e envia dados.
- **Receptor**: Código do dispositivo que recebe e processa dados.
- **Documentação**: Diagramas.

## Contribuição
Contribuições são bem-vindas! siga as diretrizes no arquivo [CONTRIBUTING.md](CONTRIBUTING.md).
