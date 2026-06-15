#include <DHT.h>

// ==========================================
// Definição dos Pinos
// ==========================================
#define DHTPIN 4
#define DHTTYPE DHT22

#define LDR_PIN 9
#define LED_R_PIN 5
#define LED_B_PIN 7
#define LED_G_PIN 6

#define BUZZER_PIN 15
#define POT_PIN 16
#define BUTTON_PIN 17

// ==========================================
// Objetos e Variáveis de Controle
// ==========================================
DHT dht(DHTPIN, DHTTYPE);

// Controle do sistema (Botão/Interrupção)
volatile bool sistemaAtivo = true;
volatile unsigned long ultimoTempoBotao = 0;
const unsigned long tempoDebounce = 250;

// Temporizadores para Serial e DHT
unsigned long ultimoTempoSerial = 0;
const unsigned long intervaloSerial = 1500; // 1,5 segundos
unsigned long ultimoTempoDHT = 0;
const unsigned long intervaloDHT = 2000;    // DHT22 requer ~2s entre leituras

// Armazenamento das medições
float temperatura = 0.0;
float umidade = 0.0;
int luminosidade = 0;
int valorPot = 0;

// Estados do Hardware
bool estadoLED = false;
String corAtual = "Nenhuma";
int r = 0, g = 0, b = 0;

// ==========================================
// Interrupção do Botão
// ==========================================
void IRAM_ATTR alternarSistema() {
  unsigned long tempoAtual = millis();
  // Verifica o debounce para evitar disparos múltiplos
  if (tempoAtual - ultimoTempoBotao > tempoDebounce) {
    sistemaAtivo = !sistemaAtivo;
    ultimoTempoBotao = tempoAtual;
  }
}

// ==========================================
// Configuração Inicial
// ==========================================
void setup() {
  Serial.begin(115200);
  
  // Configuração dos Pinos de Saída
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Configuração do Botão com Pull-Up interno
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Inicialização do Sensor de Temperatura
  dht.begin();

  // Acopla a interrupção ao pino do botão (acionado na borda de descida)
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), alternarSistema, FALLING);
}

// ==========================================
// Loop Principal
// ==========================================
void loop() {
  unsigned long tempoAtual = millis();

  // 1. Leitura do Sensor DHT22 (A cada 2 segundos)
  if (tempoAtual - ultimoTempoDHT >= intervaloDHT) {
    ultimoTempoDHT = tempoAtual;
    float tempLida = dht.readTemperature();
    float umLida = dht.readHumidity();
    
    // Atualiza apenas se a leitura for válida
    if (!isnan(tempLida) && !isnan(umLida)) {
      temperatura = tempLida;
      umidade = umLida;
    }
  }

  // 2. Leitura dos Sensores Analógicos (Contínua)
  luminosidade = analogRead(LDR_PIN);
  valorPot = analogRead(POT_PIN);
  
  // Define a cor baseada na tensão do potenciômetro (0 a 4095 no ESP32)
  definirCor(valorPot);

  // 3. Lógica de Acionamento do LED RGB
  // Ambiente escuro (< 400) e temperatura dentro do limite seguro (0 a 25)
  bool ambienteEscuro = (luminosidade < 400);
  bool temperaturaLED_OK = (temperatura >= 0.0 && temperatura <= 25.0);

  if (sistemaAtivo && ambienteEscuro && temperaturaLED_OK) {
    estadoLED = true;
    analogWrite(LED_R_PIN, r);
    analogWrite(LED_G_PIN, b); // Corrigido a pinagem R-B-G para bater com os pinos informados
    analogWrite(LED_B_PIN, g);
  } else {
    estadoLED = false;
    analogWrite(LED_R_PIN, 0);
    analogWrite(LED_G_PIN, 0);
    analogWrite(LED_B_PIN, 0);
  }

  // 4. Lógica de Acionamento do Buzzer
  bool temperaturaAlarme = (temperatura < 0.0 || temperatura > 25.0);
  
  if (sistemaAtivo && temperaturaAlarme) {
    tone(BUZZER_PIN, 1000); // Emite frequência de 1kHz
  } else {
    noTone(BUZZER_PIN);     // Silencia o buzzer
  }

  // 5. Saída no Monitor Serial (A cada 1,5 segundos)
  if (tempoAtual - ultimoTempoSerial >= intervaloSerial) {
    ultimoTempoSerial = tempoAtual;
    
    Serial.println("=========================================");
    Serial.print("Sistema: "); Serial.println(sistemaAtivo ? "LIGADO" : "DESLIGADO");
    Serial.print("Luminosidade (LDR): "); Serial.println(luminosidade);
    Serial.print("Temperatura: "); Serial.print(temperatura); Serial.println(" C");
    Serial.print("Umidade: "); Serial.print(umidade); Serial.println(" %");
    Serial.print("Estado do LED: "); Serial.println(estadoLED ? "LIGADO" : "DESLIGADO");
    Serial.print("Cor do LED (Pot.): "); Serial.println(corAtual);
    Serial.println("=========================================");
  }
}

// ==========================================
// Função Auxiliar de Mapeamento de Cor
// ==========================================
void definirCor(int valorP) {
  // ESP32 ADC tem resolução de 12 bits: 0 a 4095
  // Dividido em 5 blocos seguindo a Temperatura de Cor desejada
  
  if (valorP < 820) {
    // Tensão Baixa: Vermelho
    r = 255; g = 0; b = 0;
    corAtual = "Vermelho";
  } 
  else if (valorP < 1640) {
    // Laranja / Amarelo
    r = 255; g = 165; b = 0;
    corAtual = "Amarelo/Laranja";
  } 
  else if (valorP < 2460) {
    // Tensão Média: Branco
    r = 255; g = 255; b = 255;
    corAtual = "Branco";
  } 
  else if (valorP < 3280) {
    // Azul Turquesa
    r = 0; g = 255; b = 255;
    corAtual = "Azul Turquesa";
  } 
  else {
    // Tensão Alta: Azul
    r = 0; g = 0; b = 255;
    corAtual = "Azul";
  }
}
