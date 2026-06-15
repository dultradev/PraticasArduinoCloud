#include "thingProperties.h"
#include <DHT.h>

// ==========================================
// Definição dos Pinos
// ==========================================
#define DHTPIN 4
#define DHTTYPE DHT22

#define LDR_PIN 9
#define LED_R_PIN 5
#define LED_B_PIN 6
#define LED_G_PIN 7

#define BUZZER_PIN 15
#define POT_PIN 16
#define BUTTON_PIN 17

// ==========================================
// Objetos e Variáveis de Controle
// ==========================================
DHT dht(DHTPIN, DHTTYPE);

// Controle Físico (Botão/Interrupção)
volatile bool sistemaAtivo = true;
volatile unsigned long ultimoTempoBotao = 0;
const unsigned long tempoDebounce = 250;

// Temporizadores
unsigned long ultimoTempoSerial = 0;
const unsigned long intervaloSerial = 1500;
unsigned long ultimoTempoDHT = 0;
const unsigned long intervaloDHT = 2000;

// Variáveis de Controle IoT (Ativação/Desativação)
bool sensorTempAtivo = true;
bool sensorLDRAtivo = true;
bool buzzerAtivoIoT = true;
int ledModoManual = -1; // -1: Auto, 1: Forçado Ligado, 0: Forçado Desligado

// Controle da Cor Temporária (IoT)
bool corTemporariaAtiva = false;
unsigned long tempoFimCor = 0;
int rTemp = 0, gTemp = 0, bTemp = 0;
float umidade = 0.0;

// Variáveis locais exclusivas de processamento (não sincronizadas com a nuvem)
String corAtual = "Nenhuma";
int r = 0, g = 0, b = 0; 

// ==========================================
// Interrupção do Botão
// ==========================================
void IRAM_ATTR alternarSistema() {
  unsigned long tempoAtual = millis();
  if (tempoAtual - ultimoTempoBotao > tempoDebounce) {
    sistemaAtivo = !sistemaAtivo;
    ledModoManual = -1; // O botão físico também reseta o LED pro modo automático
    ultimoTempoBotao = tempoAtual;
  }
}

// ==========================================
// Configuração Inicial
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(1500); // Aguarda inicialização da Serial
  
  // Inicialização IoT (Gerada pelo Arduino Cloud)
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  // Configuração dos Pinos
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  dht.begin();
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), alternarSistema, FALLING);
}

// ==========================================
// Loop Principal
// ==========================================
void loop() {
  ArduinoCloud.update(); // Mantém a conexão com a nuvem viva
  unsigned long tempoAtual = millis();

  // 1. Leitura do Sensor DHT22
  if (sensorTempAtivo && (tempoAtual - ultimoTempoDHT >= intervaloDHT)) {
    ultimoTempoDHT = tempoAtual;
    float tempLida = dht.readTemperature();
    float umLida = dht.readHumidity();
    if (!isnan(tempLida) && !isnan(umLida)) {
      temperatura = tempLida; // Atualiza a variável da nuvem
      umidade = umLida;       // Atualiza a variável da nuvem
    }
  }

  // 2. Leitura dos Sensores Analógicos
  if (sensorLDRAtivo) {
    luminosidade = analogRead(LDR_PIN); // Atualiza a variável da nuvem
  }
  valorPot = analogRead(POT_PIN);       // Atualiza a variável da nuvem
  definirCor(valorPot);

  // 3. Lógica do LED RGB
  if (corTemporariaAtiva) {
    if (tempoAtual > tempoFimCor) {
      corTemporariaAtiva = false; // Finaliza o 1 segundo
    } else {
      aplicarCorRGB(rTemp, bTemp, gTemp);
      estadoLED = true; // Atualiza a variável da nuvem
    }
  } 
  
  // Lógica normal / manual se não houver cor temporária
  if (!corTemporariaAtiva) {
    if (ledModoManual == 1 && sistemaAtivo) {
      estadoLED = true;
      aplicarCorRGB(r, b, g);
    } else if (ledModoManual == 0 || !sistemaAtivo) {
      estadoLED = false;
      aplicarCorRGB(0, 0, 0);
    } else {
      // Modo Automático (ledModoManual == -1)
      bool ambienteEscuro = (luminosidade < 400);
      bool tempLED_OK = (!sensorTempAtivo) || (temperatura >= 0.0 && temperatura <= 25.0);
      
      if (sistemaAtivo && sensorLDRAtivo && ambienteEscuro && tempLED_OK) {
        estadoLED = true;
        aplicarCorRGB(r, b, g);
      } else {
        estadoLED = false;
        aplicarCorRGB(0, 0, 0);
      }
    }
  }

  // 4. Lógica de Acionamento do Buzzer
  bool tempAlarme = sensorTempAtivo && (temperatura < 0.0 || temperatura > 25.0);
  if (sistemaAtivo && buzzerAtivoIoT && tempAlarme) {
    tone(BUZZER_PIN, 1000);
  } else {
    noTone(BUZZER_PIN);
  }

  // 5. Monitor Serial (A cada 1,5s)
  if (tempoAtual - ultimoTempoSerial >= intervaloSerial) {
    ultimoTempoSerial = tempoAtual;
    Serial.println("=========================================");
    Serial.print("Comando IoT Recebido: "); Serial.println(comando);
    Serial.print("Sistema: "); Serial.println(sistemaAtivo ? "LIGADO" : "DESLIGADO");
    if (sensorLDRAtivo) { Serial.print("Luminosidade: "); Serial.println(luminosidade); }
    if (sensorTempAtivo) { 
      Serial.print("Temperatura: "); Serial.print(temperatura); Serial.println(" C");
      Serial.print("Umidade: "); Serial.print(umidade); Serial.println(" %");
    }
    Serial.print("Estado do LED: "); Serial.println(estadoLED ? "LIGADO" : "DESLIGADO");
    Serial.print("Cor Potenciometro: "); Serial.println(corAtual);
    Serial.println("=========================================");
  }
}

// ==========================================
// Funções Auxiliares
// ==========================================
void aplicarCorRGB(int red, int blue, int green) {
  analogWrite(LED_R_PIN, red);
  analogWrite(LED_G_PIN, blue);
  analogWrite(LED_B_PIN, green);
}

void definirCor(int valorP) {
  if (valorP < 820) { r = 255; g = 0; b = 0; corAtual = "Vermelho"; } 
  else if (valorP < 1640) { r = 255; g = 165; b = 0; corAtual = "Amarelo/Laranja"; } 
  else if (valorP < 2460) { r = 255; g = 255; b = 255; corAtual = "Branco"; } 
  else if (valorP < 3280) { r = 0; g = 255; b = 255; corAtual = "Azul Turquesa"; } 
  else { r = 0; g = 0; b = 255; corAtual = "Azul"; }
}

// ==========================================
// Callback do Arduino IoT Cloud
// Recebe a string de comando da nuvem e aciona as flags
// ==========================================
void onComandoChange()  {
  // Padroniza removendo espaços extras do início e fim
  comando.trim(); 
  
  if (comando == "Ligar") {
    ledModoManual = 1;
  } 
  else if (comando == "Desligar") {
    ledModoManual = 0;
  } 
  else if (comando == "Vermelho") {
    corTemporariaAtiva = true;
    rTemp = 255; gTemp = 0; bTemp = 0;
    tempoFimCor = millis() + 1000;
  } 
  else if (comando == "Amarelo") {
    corTemporariaAtiva = true;
    rTemp = 255; gTemp = 255; bTemp = 0;
    tempoFimCor = millis() + 1000;
  } 
  else if (comando == "Azul") {
    corTemporariaAtiva = true;
    rTemp = 0; gTemp = 0; bTemp = 255;
    tempoFimCor = millis() + 1000;
  } 
  else if (comando == "Desativar Temperatura") {
    sensorTempAtivo = false;
  } 
  else if (comando == "Ativar Temperatura") {
    sensorTempAtivo = true;
  } 
  else if (comando == "Desativar Detector") {
    sensorLDRAtivo = false;
  } 
  else if (comando == "Ativar Detector") {
    sensorLDRAtivo = true;
  } 
  else if (comando == "Desativar Buzzer") {
    buzzerAtivoIoT = false;
  } 
  else if (comando == "Ativar Buzzer") {
    buzzerAtivoIoT = true;
  }
}
/*
  Since Temperatura is READ_WRITE variable, onTemperaturaChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onTemperaturaChange()  {
  // Add your code here to act upon Temperatura change
}
/*
  Since Luminosidade is READ_WRITE variable, onLuminosidadeChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onLuminosidadeChange()  {
  // Add your code here to act upon Luminosidade change
}
/*
  Since ValorPot is READ_WRITE variable, onValorPotChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onValorPotChange()  {
  // Add your code here to act upon ValorPot change
}
/*
  Since EstadoLed is READ_WRITE variable, onEstadoLedChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onEstadoLedChange()  {
  // Add your code here to act upon EstadoLed change
}
