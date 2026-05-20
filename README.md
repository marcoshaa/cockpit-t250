# cockpit-t250

Painel digital para Yamaha Ténéré 250 (2011) — Arduino Nano + Nextion HMI 3.5".

## Hardware

| Componente        | Modelo/Detalhe             |
|-------------------|----------------------------|
| Microcontrolador  | Arduino Nano               |
| Tela              | Nextion NX4832F035 (480×320) |
| Regulador         | LM2596 step-down (12V → 5V) |
| Conector          | Fogopin 5 pinos            |

## Sensores monitorados

- Velocidade (reed switch via INT0)
- RPM (bobina via optoacoplador PC817 + INT1)
- Temperatura do motor (NTC 10k)
- Tensão da bateria (divisor resistivo 30k/10k)
- Nível de combustível (sensor flutuador original)
- Indicadores: seta esq/dir, neutro, farol alto

## Estrutura do projeto

```
firmware/cockpit/    → cockpit.ino (Arduino IDE)
hmi/                 → cockpit.HMI (Nextion Editor)
hardware/            → pinagem e esquemáticos
mockup/              → preview HTML da tela (480×320)
docs/                → log completo do projeto
```

## Como usar

1. Abrir `firmware/cockpit/cockpit.ino` no Arduino IDE
2. Selecionar placa: **Arduino Nano** / processador: **ATmega328P**
3. Gravar o firmware HMI na Nextion via cartão microSD (FAT32, ≤32GB)
4. Ajustar LM2596 para **5V** antes de conectar
5. Calibrar pulsos do velocímetro e ratios de marcha conforme a moto

## Próximos passos

- [ ] Testar LM2596 na bancada
- [ ] Montar proteção do RPM (PC817)
- [ ] Medir sensor de combustível (cheio/vazio) com multímetro
- [ ] Montar layout no Nextion Editor
- [ ] Calibrar velocímetro e estimativa de marcha
- [ ] Definir fixação física no guidão
