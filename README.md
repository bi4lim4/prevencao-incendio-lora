# Sistema de Prevenção e Monitoramento de Incêndios com LoRa

Este projeto visa prevenir e monitorar incêndios na agricultura familiar utilizando sensores de temperatura, umidade, CO2 e chama, com comunicação via LoRa. O sistema é composto por um transmissor (coleta de dados) e um receptor (processamento e alertas).

## Funcionalidades
- **Transmissor**:
  - Coleta dados de temperatura, umidade, CO2 e chama.
  - Envia os dados via LoRa para o receptor.

- **Receptor**:
  - Recebe os dados via LoRa.
  - Processa os dados usando lógica fuzzy para determinar o risco de incêndio.
  - Aciona um relé em caso de risco alto para ativar uma bomba de água e resfriar o ambiente.

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

## Estrutura do Projeto
- **Transmissor**: Código e documentação do dispositivo que coleta e envia dados.
- **Receptor**: Código do dispositivo que recebe e processa dados.
- **Documentação**: Diagramas.

## Contribuição
Contribuições são bem-vindas! siga as diretrizes no arquivo [CONTRIBUTING.md](CONTRIBUTING.md).
