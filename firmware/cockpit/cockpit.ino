// cockpit-t250 — Yamaha Ténéré 250 (2011)
// Arduino Nano + Nextion NX4832F035 (3.5" 480x320)
// Ver o log completo em: docs/cockpit_tenere250_log.md

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
void isrVelocidade() {
  unsigned long agora = millis();
  unsigned long dt = agora - lastPulseVel;
  if (dt > 5) {
    velocidade = (CIRCUNFERENCIA_M / PULSOS_POR_VOLTA) / (dt / 1000.0) * 3.6;
    odoMetros  += (unsigned long)(CIRCUNFERENCIA_M / PULSOS_POR_VOLTA * 1000);
    tripMetros += (unsigned long)(CIRCUNFERENCIA_M / PULSOS_POR_VOLTA * 1000);
    lastPulseVel = agora;
  }
}

void isrRPM() {
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

  if (agora - lastPulseVel > 2000) velocidade = 0;
  if (agora - lastPulseRPM > 2000) rpm = 0;

  temperatura = lerTemperatura();
  bateria     = lerBateria();
  nivel       = lerNivel();

  nx("vel",    "val", String((int)velocidade));
  nx("rpm",    "val", String((int)rpm));
  nx("temp",   "val", String((int)temperatura));
  nx("bat",    "txt", "\"" + String(bateria, 1) + "V\"");
  nx("nivel",  "val", String(nivel));
  nx("odo",    "txt", "\"" + String(odoMetros / 1000) + "\"");
  nx("trip",   "txt", "\"" + String(tripMetros / 1000) + "\"");
  nx("marcha", "txt", "\"" + estimarMarcha(velocidade, rpm) + "\"");

  nx("ind_se", "pic", digitalRead(4)==LOW ? "1" : "0");
  nx("ind_sd", "pic", digitalRead(5)==LOW ? "1" : "0");
  nx("ind_n",  "pic", digitalRead(6)==LOW ? "1" : "0");
  nx("ind_hi", "pic", digitalRead(7)==LOW ? "1" : "0");
}
