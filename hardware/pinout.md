# Pinagem — cockpit-t250

## Conector Fogopin 5 pinos (moto → placa)

| Pino | Sinal       |
|------|-------------|
| 1    | GND         |
| 2    | 12V (chave) |
| 3    | Velocímetro |
| 4    | RPM         |
| 5    | Temperatura |

## Arduino Nano

| Pino | Função                              |
|------|-------------------------------------|
| D2   | Velocímetro (ISR INT0 — FALLING)    |
| D3   | RPM (ISR INT1 — FALLING via PC817)  |
| D4   | Seta esquerda (INPUT_PULLUP)        |
| D5   | Seta direita  (INPUT_PULLUP)        |
| D6   | Neutro        (INPUT_PULLUP)        |
| D7   | Farol alto    (INPUT_PULLUP)        |
| A0   | Temperatura (NTC 10k)               |
| A1   | Tensão da bateria (divisor 30k/10k) |
| A2   | Nível de combustível (flutuador)    |
| TX   | Nextion RX                          |
| RX   | Nextion TX                          |

## Alimentação — LM2596

| Conexão      | Detalhe                         |
|--------------|---------------------------------|
| IN+          | 12V (via chave geral da moto)   |
| IN–          | GND                             |
| OUT+         | 5V → Arduino Vin + Nextion 5V   |
| OUT–         | GND comum                       |

> Ajustar saída para 5V com multímetro ANTES de conectar qualquer componente.

## Proteção RPM

```
Bobina ──► R1 (10kΩ) ──► PC817 (LED) ──► GND
                              │
                         PC817 (transistor) ──► D3 (Arduino)
                              │
                             GND
```

## Divisor resistivo — bateria

```
Bateria (+) ──► R1 (30kΩ) ──┬──► A1
                             │
                           R2 (10kΩ)
                             │
                            GND
```
Fator: 0,25 — range: 0–15V → 0–3,75V (dentro dos 5V do ADC)
