# Cockpit Digital — Yamaha Ténéré 250 (2011)
> Log de projeto gerado a partir da conversa com Claude  
> Data: maio de 2026

---

## Peças adquiridas

| Peça | Detalhe |
|------|---------|
| Conector Fogopin | 5 pinos |
| Tela Nextion HMI | NX4832F035 — 3.5", 480×320 px |
| Arduino Nano | Microcontrolador principal |
| Regulador de tensão | LM2596 — step-down ajustável |

---

## Arquitetura do sistema

```
[Moto 12V] ──► [LM2596] ──► 5V ──► [Arduino Nano] ──► TX/RX ──► [Nextion]
                                          ▲
                              sinais dos sensores
                         (velocímetro, RPM, temp, etc.)
```

### Pinagem sugerida — Conector Fogopin 5 pinos

| Pino | Sinal |
|------|-------|
| 1 | GND |
| 2 | 12V (alimentação) |
| 3 | Velocímetro |
| 4 | RPM |
| 5 | Temperatura |

### Pinos do Arduino Nano

| Pino | Função |
|------|--------|
| D2 | Velocímetro (interrupção externa) |
| D3 | RPM (interrupção externa) |
| A0 | Temperatura (NTC) |
| A1 | Nível de combustível / Tensão da bateria |
| D4–D6 | Indicadores (seta, farol, neutro) |
| TX/RX | Comunicação serial com a Nextion |

---

## Sensores — detalhes por canal

### Velocímetro
- A Ténéré 250 2011 usa **reed switch** na roda dianteira
- Interceptar o fio que vai ao painel original
- Conectar no pino **D2** (interrupção externa INT0)
- Calibrar: descobrir quantos pulsos por km (normalmente 6–8 pulsos por rotação da roda)
- Calcular circunferência da roda: `C = π × diâmetro` (roda 21" dianteira ≈ 2,10 m)

```cpp
// Exemplo de leitura de velocidade por interrupção
volatile unsigned long lastPulse = 0;
volatile float velocidade = 0;
const float CIRCUNFERENCIA_M = 2.10;  // metros — ajustar para sua roda
const int PULSOS_POR_VOLTA = 6;       // ajustar conforme sensor

void IRAM_ATTR contaPulso() {
  unsigned long agora = millis();
  unsigned long intervalo = agora - lastPulse;
  if (intervalo > 5) { // debounce 5ms
    velocidade = (CIRCUNFERENCIA_M / PULSOS_POR_VOLTA) / (intervalo / 1000.0) * 3.6;
    lastPulse = agora;
  }
}

void setup() {
  attachInterrupt(digitalPinToInterrupt(2), contaPulso, FALLING);
}
```

### RPM
- Sinal vem da bobina de ignição — tensão de pico pode chegar a 400V
- **NUNCA conectar direto ao Arduino**
- Usar **optoacoplador PC817** ou divisor resistivo com zener para proteção
- Conectar no pino **D3** (interrupção externa INT1)
- 1 pulso por rotação (motor monocilíndrico 4T = 1 faísca a cada 2 rotações — verificar)

```
Bobina ──► R1 (10kΩ) ──► PC817 (LED) ──► GND
                              │
                         PC817 (transistor) ──► D3 (Arduino)
                              │
                             GND
```

```cpp
volatile unsigned long lastIgnicao = 0;
volatile float rpm = 0;

void IRAM_ATTR contaIgnicao() {
  unsigned long agora = micros();
  unsigned long intervalo = agora - lastIgnicao;
  if (intervalo > 5000) { // debounce 5ms
    rpm = 60000000.0 / intervalo; // 1 pulso por rotação
    lastIgnicao = agora;
  }
}
```

### Temperatura do motor
- Verificar se a moto tem sensor NTC original (rosca no cabeçote)
- Se não tiver: instalar **DS18B20** ou **NTC 10k** com adaptador BSP
- Leitura analógica no pino **A0**

```cpp
// NTC 10k com resistor de pull-up 10k
float lerTemperatura() {
  int raw = analogRead(A0);
  float tensao = raw * (5.0 / 1023.0);
  float resistencia = 10000.0 * (5.0 - tensao) / tensao;
  // Equação Steinhart-Hart simplificada
  float tempK = 1.0 / (0.001129 + 0.000234 * log(resistencia) + 0.0000000876 * pow(log(resistencia), 3));
  return tempK - 273.15;
}
```

### Tensão da bateria
- Usar **divisor resistivo** para reduzir 12–15V para 0–5V
- R1 = 30kΩ, R2 = 10kΩ → fator: 0,25 (15V × 0,25 = 3,75V — dentro do range)

```
Bateria (+) ──► R1 (30kΩ) ──┬──► A1 (Arduino)
                             │
                           R2 (10kΩ)
                             │
                            GND
```

```cpp
float lerBateria() {
  int raw = analogRead(A1);
  float tensaoArduino = raw * (5.0 / 1023.0);
  return tensaoArduino * ((30000.0 + 10000.0) / 10000.0); // fator divisor
}
```

### Nível de combustível
- Sensor original: resistor flutuador (varia de ~10Ω cheio a ~180Ω vazio — verificar com multímetro)
- Usar divisor resistivo com resistor fixo de 220Ω em série
- Mapear a faixa com `map()` do Arduino

```cpp
int lerNivel() {
  int raw = analogRead(A1); // ou pino dedicado
  return map(raw, 100, 900, 0, 100); // calibrar os extremos
}
```

---

## Estimativa de marcha (sem sensor dedicado)

A Ténéré 250 não possui sensor de marcha de fábrica. Duas abordagens:

### Opção 1 — Cálculo por software (recomendado para começar)

Usa a relação `velocidade ÷ RPM` para estimar a marcha. Ratios calibrados para a T250:

| Marcha | Ratio (vel/rpm × 1000) |
|--------|------------------------|
| 1ª | < 3.2 |
| 2ª | 3.2 – 5.5 |
| 3ª | 5.5 – 7.8 |
| 4ª | 7.8 – 10.2 |
| 5ª | 10.2 – 13.0 |
| 6ª | > 13.0 |

```cpp
String estimarMarcha(float vel, float rpm) {
  if (rpm < 800 || vel < 2) return "N";
  float ratio = (vel / rpm) * 1000.0;
  if (ratio < 3.2)  return "1";
  if (ratio < 5.5)  return "2";
  if (ratio < 7.8)  return "3";
  if (ratio < 10.2) return "4";
  if (ratio < 13.0) return "5";
  return "6";
}
```

> **Calibração:** rode em cada marcha a velocidade constante e anote os valores reais de vel e RPM. Ajuste os limites da tabela acima.

### Opção 2 — Sensor de posição no câmbio
- Reed switches ou sensor resistivo nas posições do tambor seletor
- Requer abrir a tampa do câmbio
- Kits genéricos disponíveis no Aliexpress (~R$40–80)
- Leitura analógica direta — cada posição gera uma tensão diferente

---

## Alimentação — LM2596

- Entrada: 12V da bateria da moto (via chave geral)
- Saída: **5V** (ajustar pelo potenciômetro com multímetro ANTES de conectar)
- Corrente máxima: 3A (suficiente para Arduino + Nextion)
- Sempre testar a tensão de saída na bancada antes de ligar na moto

```
Bateria (+12V) ──► chave ──► LM2596 IN+
Bateria (GND)  ──────────► LM2596 IN–
LM2596 OUT+ ──► Arduino Vin / Nextion 5V
LM2596 OUT– ──► GND comum
```

---

## Tela Nextion NX4832F035

### Especificações
- Resolução: **480 × 320 px**
- Comunicação: **Serial UART** (TX/RX)
- Velocidade padrão: **9600 baud** (pode aumentar para 115200)
- Alimentação: 5V / ~500mA

### Cartão microSD
- Usado **apenas para gravar o firmware** na tela
- Após gravar o arquivo `.tft`, o cartão pode ser removido
- Tamanho máximo suportado: **32 GB**
- Formato obrigatório: **FAT32**
- Cartão de 2–4 GB funciona perfeitamente

### Layout do painel (480×320 px)

```
┌────────────────────────────────────────────────────┐
│  ┌──────────────┐ │  RPM        │  Temp motor      │
│  │              │ │  4200       │  078 °C           │
│  │  087  km/h   │ ├─────────────┴──────────────────┤
│  │  ▓▓▓▓▓░░░░   │ │  Bateria │  Combustível │ Odo  │
│  │              │ │  13.1V   │  ▮▮▮▯         │12487 │
│  │  marcha: 3   │ ├──────────┴──────────────┴──────┤
│  └──────────────┘ │  Parcial │  Hora  │  ◄ N hi ►  │
│                   │  0342 km │  14:35 │             │
│                   ├──────────────────────────────── │
│                   │  ténéré 250 · 2011        v1.0  │
└────────────────────────────────────────────────────┘
```

### Cores
| Elemento | Cor hex |
|----------|---------|
| Fundo | `#000000` |
| Velocidade (grande) | `#FF6A00` |
| Demais valores | `#E0A000` |
| Indicadores ativos | `#00CC44` |
| Farol alto | `#4488FF` |
| Separadores | `#1A1A1A` |
| Rótulos | `#666666` |

### Protocolo serial Arduino → Nextion

```cpp
// Função auxiliar — envia dado para componente da Nextion
void nextionSet(String componente, String propriedade, String valor) {
  Serial.print(componente + "." + propriedade + "=" + valor);
  Serial.write(0xFF); Serial.write(0xFF); Serial.write(0xFF);
}

// Exemplos de uso no loop()
nextionSet("vel",   "val", String((int)velocidade));
nextionSet("rpm",   "val", String((int)rpm));
nextionSet("temp",  "val", String((int)temperatura));
nextionSet("bat",   "txt", "\"" + String(bateria, 1) + "\"");
nextionSet("marcha","txt", "\"" + estimarMarcha(velocidade, rpm) + "\"");
```

---

## Código Arduino completo (esqueleto)

```cpp
#include <SoftwareSerial.h>

// Nextion na serial de hardware (pinos 0/1 do Nano)
// Para debug, usar SoftwareSerial em outros pinos

// ── CONSTANTES ──────────────────────────────────────
const float CIRCUNFERENCIA_M  = 2.10;
const int   PULSOS_POR_VOLTA  = 6;
const float R1_BATERIA        = 30000.0;
const float R2_BATERIA        = 10000.0;

// ── VARIÁVEIS GLOBAIS ────────────────────────────────
volatile unsigned long lastPulseVel = 0;
volatile unsigned long lastPulseRPM = 0;
volatile float velocidade = 0;
volatile float rpm        = 0;
float temperatura = 0;
float bateria     = 0;
int   nivel       = 0;
unsigned long odoMetros   = 0;
unsigned long tripMetros  = 0;

// ── INTERRUPÇÕES ─────────────────────────────────────
void IRAM_ATTR isrVelocidade() {
  unsigned long agora = millis();
  unsigned long dt = agora - lastPulseVel;
  if (dt > 5) {
    velocidade = (CIRCUNFERENCIA_M / PULSOS_POR_VOLTA) / (dt / 1000.0) * 3.6;
    odoMetros  += (unsigned long)(CIRCUNFERENCIA_M / PULSOS_POR_VOLTA * 1000);
    tripMetros += (unsigned long)(CIRCUNFERENCIA_M / PULSOS_POR_VOLTA * 1000);
    lastPulseVel = agora;
  }
}

void IRAM_ATTR isrRPM() {
  unsigned long agora = micros();
  unsigned long dt = agora - lastPulseRPM;
  if (dt > 5000) {
    rpm = 60000000.0 / dt;
    lastPulseRPM = agora;
  }
}

// ── NEXTION ───────────────────────────────────────────
void nx(String comp, String prop, String val) {
  Serial.print(comp + "." + prop + "=" + val);
  Serial.write(0xFF); Serial.write(0xFF); Serial.write(0xFF);
}

// ── MARCHA ESTIMADA ───────────────────────────────────
String estimarMarcha(float vel, float rpm) {
  if (rpm < 800 || vel < 2) return "N";
  float r = (vel / rpm) * 1000.0;
  if (r < 3.2)  return "1";
  if (r < 5.5)  return "2";
  if (r < 7.8)  return "3";
  if (r < 10.2) return "4";
  if (r < 13.0) return "5";
  return "6";
}

// ── LEITURAS ANALÓGICAS ───────────────────────────────
float lerBateria() {
  int raw = analogRead(A1);
  float v = raw * (5.0 / 1023.0);
  return v * ((R1_BATERIA + R2_BATERIA) / R2_BATERIA);
}

float lerTemperatura() {
  int raw = analogRead(A0);
  float v = raw * (5.0 / 1023.0);
  if (v <= 0) return 0;
  float r = 10000.0 * (5.0 - v) / v;
  float tK = 1.0 / (0.001129 + 0.000234 * log(r) + 0.0000000876 * pow(log(r), 3));
  return tK - 273.15;
}

int lerNivel() {
  int raw = analogRead(A2);
  return constrain(map(raw, 100, 900, 0, 100), 0, 100);
}

// ── SETUP ─────────────────────────────────────────────
void setup() {
  Serial.begin(9600); // Nextion
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP); // seta esquerda
  pinMode(5, INPUT_PULLUP); // seta direita
  pinMode(6, INPUT_PULLUP); // neutro
  pinMode(7, INPUT_PULLUP); // farol alto
  attachInterrupt(digitalPinToInterrupt(2), isrVelocidade, FALLING);
  attachInterrupt(digitalPinToInterrupt(3), isrRPM, FALLING);
  delay(500); // aguarda Nextion iniciar
}

// ── LOOP ──────────────────────────────────────────────
unsigned long ultimaAtualizacao = 0;
const int INTERVALO_MS = 200; // atualiza tela 5x por segundo

void loop() {
  unsigned long agora = millis();
  if (agora - ultimaAtualizacao < INTERVALO_MS) return;
  ultimaAtualizacao = agora;

  // Zera velocidade se sem pulso por 2s
  if (agora - lastPulseVel > 2000) velocidade = 0;
  if (agora - lastPulseRPM > 2000) rpm = 0;

  temperatura = lerTemperatura();
  bateria     = lerBateria();
  nivel       = lerNivel();

  // Envia para Nextion
  nx("vel",    "val", String((int)velocidade));
  nx("rpm",    "val", String((int)rpm));
  nx("temp",   "val", String((int)temperatura));
  nx("bat",    "txt", "\"" + String(bateria, 1) + "V\"");
  nx("nivel",  "val", String(nivel));
  nx("odo",    "txt", "\"" + String(odoMetros / 1000) + "\"");
  nx("trip",   "txt", "\"" + String(tripMetros / 1000) + "\"");
  nx("marcha", "txt", "\"" + estimarMarcha(velocidade, rpm) + "\"");

  // Indicadores digitais
  nx("ind_se", "pic", digitalRead(4)==LOW ? "1" : "0"); // seta esq
  nx("ind_sd", "pic", digitalRead(5)==LOW ? "1" : "0"); // seta dir
  nx("ind_n",  "pic", digitalRead(6)==LOW ? "1" : "0"); // neutro
  nx("ind_hi", "pic", digitalRead(7)==LOW ? "1" : "0"); // farol alto
}
```

---

## Referência de arquivos do projeto

| Arquivo | Descrição |
|---------|-----------|
| `painel_tenere250.html` | Mockup visual interativo da tela (480×320) |
| `cockpit_tenere250_log.md` | Este arquivo — log completo do projeto |
| `cockpit.ino` | Código Arduino (a gerar) |
| `cockpit.HMI` | Projeto Nextion Editor (a montar) |

---

## Próximos passos

- [ ] Testar LM2596 na bancada — ajustar para 5V antes de conectar
- [ ] Montar proteção do sinal de RPM (optoacoplador PC817)
- [ ] Medir resistência do sensor de combustível (cheio e vazio) com multímetro
- [ ] Montar o projeto no Nextion Editor com os componentes nomeados
- [ ] Gravar firmware na Nextion via cartão microSD ≤ 32GB (FAT32)
- [ ] Calibrar velocímetro (pulsos por volta)
- [ ] Calibrar estimativa de marcha em cada relação
- [ ] Definir fixação física do painel no guidão

---

*Gerado com Claude (Anthropic) — maio de 2026*
