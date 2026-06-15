# 💡 Lâmpada Inteligente com Arduino IoT Cloud

**SENAI CIMATEC — Práticas Integradas: Camada de Aplicação**
#### Gabriel Queiroz, Marcos Paulo e Rian Dultra

## Projeto

Um sistema de lâmpada inteligente baseado em ESP32 integrado ao **Arduino IoT Cloud**, capaz de:

- Acender e apagar o LED RGB automaticamente com base na luminosidade e temperatura ambiente
- Receber comandos remotos via painel na nuvem (ligar, desligar, mudar cor, ativar/desativar sensores)
- Emitir alerta sonoro via buzzer quando a temperatura sai da faixa segura (< 0°C ou > 25°C)
- Permitir controle físico de emergência via botão com interrupção de hardware
- Variar a cor do LED pelo potenciômetro, seguindo o espectro de temperatura de cor

---

## Componentes Utilizados

| Componente | Função | Pino |
|---|---|---|
| ESP32 | Microcontrolador principal | — |
| DHT22 | Temperatura e umidade | 4 |
| Fotorresistor (LDR) | Luminosidade ambiente | 9 |
| LED RGB | Lâmpada inteligente | R:5 / B:6 / G:7 |
| Buzzer | Alarme sonoro | 15 |
| Potenciômetro | Seleção de cor | 16 |
| Botão | Liga/desliga o sistema | 17 |

---

## Etapa 01 — Circuito Físico

Todos os componentes foram conectados ao ESP32 conforme a tabela de pinos acima. O botão utiliza o pull-up interno do ESP32 e foi implementado via interrupção externa para garantir resposta imediata independente do estado do loop principal.

> 📷 Para visualizar o circuito montado, consulte as imagens na pasta `/images` deste repositório.

---

## Etapa 02 — Firmware e Arduino IoT Cloud

### Configuração do Arduino IoT Cloud

Foi criado um **Thing** chamado `GRM-Thing` com as seguintes variáveis:

| Variável | Tipo | Permissão |
|---|---|---|
| `comando` | String | READ/WRITE |
| `temperatura` | float | READ |
| `luminosidade` | int | READ |
| `valorPot` | int | READ |
| `estadoLED` | bool | READ |

O arquivo `thingProperties.h` é gerado automaticamente pela plataforma e não deve ser editado manualmente.

---

### Código — `Sketch.ino`

O firmware implementa três modos de operação do LED em cascata:

1. **Cor temporária** — comandos "Vermelho", "Amarelo" ou "Azul" acendem o LED naquela cor por 1 segundo
2. **Manual** — comandos "Ligar" e "Desligar" forçam o estado do LED independente do ambiente
3. **Automático** — o LED acende sozinho quando está escuro (LDR < 400) e a temperatura está entre 0°C e 25°C

O botão físico, via interrupção (`IRAM_ATTR`), alterna o sistema entre ligado e desligado a qualquer momento e reseta o modo para automático. O buzzer emite som em 1kHz enquanto a temperatura estiver fora da faixa segura. O potenciômetro (ADC 12 bits, 0–4095) mapeia a posição para 5 cores: Vermelho → Laranja → Branco → Azul Turquesa → Azul.

```cpp
void onComandoChange() {
  comando.trim();
  if      (comando == "Ligar")                { ledModoManual = 1; }
  else if (comando == "Desligar")             { ledModoManual = 0; }
  else if (comando == "Vermelho")             { corTemporariaAtiva=true; rTemp=255; gTemp=0;   bTemp=0;   tempoFimCor=millis()+1000; }
  else if (comando == "Amarelo")              { corTemporariaAtiva=true; rTemp=255; gTemp=255; bTemp=0;   tempoFimCor=millis()+1000; }
  else if (comando == "Azul")                 { corTemporariaAtiva=true; rTemp=0;   gTemp=0;   bTemp=255; tempoFimCor=millis()+1000; }
  else if (comando == "Desativar Temperatura"){ sensorTempAtivo = false; }
  else if (comando == "Ativar Temperatura")   { sensorTempAtivo = true;  }
  else if (comando == "Desativar Detector")   { sensorLDRAtivo  = false; }
  else if (comando == "Ativar Detector")      { sensorLDRAtivo  = true;  }
  else if (comando == "Desativar Buzzer")     { buzzerAtivoIoT  = false; }
  else if (comando == "Ativar Buzzer")        { buzzerAtivoIoT  = true;  }
}
```

---

## Etapa 02 — Dashboard no Arduino IoT Cloud

O dashboard foi criado na plataforma com atualização automática em tempo real. Contém quatro widgets:

- **Estado LED** — indicador visual mostrando se a lâmpada está ligada ou desligada
- **Temperatura** — gauge de 0 a 100°C com a leitura atual do DHT22
- **Potência** — gauge de 0 a 4095 indicando a posição do potenciômetro e a faixa de cor ativa
- **Luminosidade** — gráfico de linha em modo LIVE com histórico de variação ambiental, com janelas de 1H, 1D, 7D e 15D

> 📷 Para visualizar o dashboard em funcionamento, consulte as imagens na pasta `/images` deste repositório.

---

## Estrutura de Arquivos

```
/
├── firmware/
│   ├── SketchArduinoCloud.ino           # Firmware principal integrado com o Arduino Cloud
│   ├── SketchFisico.ino           # Firmware para teste dos componentes físicos sem integração com o Arudino Cloud
│   └── thingProperties.h   # Gerado pelo Arduino IoT Cloud (não editar)
├── images/
│   ├── fotos do circuito
│   └── print do dashboard
└── README.md
```

---

## Desafios Encontrados

**Inicialização bloqueante** — o `ArduinoCloud.begin()` travava o `setup()` caso a conexão Wi-Fi demorasse, impedindo a configuração dos pinos. A solução foi entender o ciclo de vida da biblioteca e garantir a ordem correta de inicialização.

**Compatibilidade com Apple Silicon** — ao gravar via Arduino Cloud Agent em Mac M1/M2/M3, o erro `bad CPU type in executable` era retornado. A solução foi instalar o Rosetta 2:
```bash
softwareupdate --install-rosetta --agree-to-license
```

---

## Equipe

[Gabriel Queiroz, Marcos Paulo, Rian Dultra] — SENAI CIMATEC, 2026
