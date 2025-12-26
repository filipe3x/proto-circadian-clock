# Auto Solar - Documentacao Tecnica

Modulo de calculo de gradiente solar para simulacao de luz natural circadiana.

## Visao Geral

O modulo `auto_solar` calcula a posicao do sol em tempo real e mapeia essa informacao para cores e intensidades luminosas que simulam a luz natural ao longo do dia. Esta abordagem baseia-se em evidencias cientificas sobre o impacto da luz no ritmo circadiano humano.

## Algoritmos Utilizados

### 1. Calculo da Elevacao Solar (NOAA Solar Calculator)

A funcao `calculateSolarElevation()` implementa o algoritmo astronomico da NOAA (National Oceanic and Atmospheric Administration) para calcular a posicao exata do sol.

#### 1.1 Data Juliana

```
JDN = dia + (153 * m + 2) / 5 + 365 * y + y/4 - y/100 + y/400 - 32045
JD = JDN + (hora - 12) / 24 + minuto / 1440 + segundo / 86400
```

Onde:
- **JDN**: Numero do Dia Juliano (Julian Day Number)
- **JD**: Data Juliana com fracao do dia
- O valor -12 ajusta para o meio-dia solar como referencia

**Referencia**: Meeus, J. (1991). *Astronomical Algorithms*. Willmann-Bell.

#### 1.2 Seculo Juliano

```
T = (JD - 2451545.0) / 36525.0
```

- **2451545.0**: Data Juliana de J2000.0 (1 Jan 2000, 12:00 TT)
- **36525.0**: Dias num seculo Juliano

#### 1.3 Longitude Media do Sol e Anomalia Media

```
L0 = 280.46646 + T * (36000.76983 + T * 0.0003032) mod 360
M = 357.52911 + T * (35999.05029 - T * 0.0001537) mod 360
```

- **L0**: Longitude media geometrica do sol
- **M**: Anomalia media do sol

#### 1.4 Equacao do Centro

```
C = (1.914602 - T * 0.004817) * sin(M)
  + (0.019993 - T * 0.000101) * sin(2M)
  + 0.000289 * sin(3M)
```

A equacao do centro corrige a orbita eliptica da Terra.

#### 1.5 Declinacao Solar

```
theta = L0 + C  (longitude verdadeira)
epsilon = 23.439291 - T * 0.0130042  (obliquidade da ecliptica)
delta = arcsin(sin(epsilon) * sin(theta))
```

- **delta**: Declinacao solar (angulo do sol em relacao ao equador celeste)
- **epsilon**: Inclinacao do eixo terrestre (~23.44 graus)

#### 1.6 Tempo Solar Verdadeiro

```
E = 4 * (L0 - 0.0057183 - atan2(tan(theta), cos(epsilon)) * 180/PI + C)
solarTime = hora * 60 + minuto + segundo/60 + E + 4 * longitude - 60 * timezone
```

- **E**: Equacao do tempo (diferenca entre tempo solar medio e verdadeiro)

#### 1.7 Elevacao Solar Final

```
H = (solarTime / 4 - 180) * PI / 180  (angulo horario)
phi = latitude * PI / 180
elevation = arcsin(sin(phi) * sin(delta) + cos(phi) * cos(delta) * cos(H))
```

**Precisao**: Este algoritmo tem precisao de aproximadamente 0.01 graus, suficiente para aplicacoes circadianas.

**Referencia**: NOAA Solar Calculator - https://gml.noaa.gov/grad/solcalc/

---

### 2. Conversao de Temperatura de Cor (Tanner Helland)

A funcao `colorTempToRGB()` converte temperatura de cor em Kelvin para valores RGB.

#### Algoritmo

Para temperaturas <= 6600K:
```
R = 255
G = 99.4708025861 * ln(temp/100) - 161.1195681661
B = 138.5177312231 * ln(temp/100 - 10) - 305.0447927307
```

Para temperaturas > 6600K:
```
R = 329.698727446 * (temp/100 - 60)^(-0.1332047592)
G = 288.1221695283 * (temp/100 - 60)^(-0.0755148492)
B = 255
```

**Referencia**: Helland, T. (2012). *How to Convert Temperature (K) to RGB*. https://tannerhelland.com/2012/09/18/convert-temperature-color-rgb.html

Este algoritmo e uma aproximacao empirica aos dados do corpo negro (blackbody radiation), validado contra tabelas de cores Planckianas.

---

### 3. Mapeamento Elevacao-Cor

A funcao `mapSolarElevationToColor()` traduz angulos de elevacao solar em temperatura de cor e brilho, baseando-se em medicoes da luz natural.

#### Fases da Luz Natural

| Elevacao | Fase | Temp. Cor | Brilho | Descricao |
|----------|------|-----------|--------|-----------|
| < -18 | Noite astronomica | 0K | 0% | Escuridao total |
| -18 a -12 | Crepusculo astronomico | 1500K | 0-16% | Primeiros sinais de luz |
| -12 a -6 | Crepusculo nautico | 1500-2000K | 16-31% | Horizonte visivel |
| -6 a -0.833 | Crepusculo civil | 2000-2800K | 31-55% | Leitura ao ar livre |
| -0.833 a 6 | Nascer/Por | 2800-3800K | 55-78% | Cores quentes intensas |
| 6 a 15 | Manha/Tarde | 3800-4800K | 78-90% | Transicao para dia |
| 15 a 30 | Manha alta | 4800-5500K | 90-98% | Luz diurna quente |
| 30 a 50 | Dia | 5500-5900K | 100% | Luz diurna plena |
| > 50 | Meio-dia | 6500K | 100% | Zenit solar |

#### Base Cientifica

Estes valores baseiam-se em:

1. **Definicoes Astronomicas Oficiais**:
   - -18: Crepusculo astronomico (IAU)
   - -12: Crepusculo nautico
   - -6: Crepusculo civil
   - -0.833: Nascer/por do sol (correcao para refracao atmosferica)

2. **Medicoes de CCT (Correlated Color Temperature)**:
   - Nascer/por do sol: 2000-3000K
   - Luz diurna: 5500-6500K
   - Ceu azul: 10000-20000K

**Referencias**:
- CIE (Commission Internationale de l'Eclairage) - Colorimetria
- Hernandez-Andres, J. et al. (2001). *Color and spectral analysis of daylight*. Applied Optics.

---

## Impacto no Ritmo Circadiano

### Melatonina e Luz Azul

A producao de melatonina (hormona do sono) e suprimida pela luz azul (460-480nm). O modulo simula isso atraves de:

- **Noite/Crepusculo**: Cores quentes (1500-2800K) com pouco azul
- **Dia**: Cores frias (5500-6500K) com mais azul

### Fotorreceptores ipRGC

Os fotorreceptores intrinsecamente fotossensiveis (ipRGCs) da retina sao mais sensiveis a luz por volta de 480nm. O gradiente de temperatura de cor acompanha a sensibilidade circadiana.

**Referencias**:
- Brainard, G.C. et al. (2001). *Action spectrum for melatonin regulation*. Journal of Neuroscience.
- Lucas, R.J. et al. (2014). *Measuring and using light in the melanopsin age*. Trends in Neurosciences.

---

## Melhorias Futuras

### 1. Ajuste de Intensidade por Long Press

**Proposta**: Permitir aumentar/diminuir a intensidade base com pressao longa do botao.

```cpp
// Constante de multiplicador de brilho (0.25 a 2.0)
float brightnessMultiplier = 1.0;

// No handler do botao:
if (longPressDetected) {
  if (currentMode == AUTO_SOLAR) {
    // Long press direita: aumentar
    // Long press esquerda: diminuir
    brightnessMultiplier = constrain(brightnessMultiplier + delta, 0.25, 2.0);
    saveToNVS("brightness_mult", brightnessMultiplier);
  }
}

// No mapSolarElevationToColor():
color.brightness = (uint8_t)(color.brightness * brightnessMultiplier);
```

**Beneficio**: Permite personalizar a intensidade sem alterar o gradiente circadiano.

### 2. Correcao Atmosferica

A luz natural varia com:
- Nebulosidade
- Altitude
- Poluicao atmosferica

**Proposta**: Adicionar fator de correcao:

```cpp
float atmosphericCorrection = 1.0;  // 0.5 = nublado, 1.5 = alta altitude
color.brightness *= atmosphericCorrection;
```

### 3. Curva de Melanopic Lux

Em vez de CCT linear, usar melanopic equivalent daylight illuminance (EDI):

```cpp
float melanopicFactor = calculateMelanopicRatio(colorTemp);
// Ajustar brilho baseado em impacto melanopico real
```

**Referencia**: CIE S 026:2018 - *System for Metrology of Optical Radiation for ipRGC-Influenced Responses to Light*

### 4. Ajuste por Idade

A sensibilidade circadiana diminui com a idade devido ao amarelecimento do cristalino:

```cpp
// Utilizadores mais velhos podem precisar de:
// - Maior intensidade de luz azul durante o dia
// - Reducao mais agressiva de luz azul a noite
float ageCompensation = calculateAgeCompensation(userAge);
```

### 5. Modo "Jet Lag"

Deslocar gradualmente o ciclo solar para ajudar na adaptacao a novos fusos horarios:

```cpp
// Shift de +1h por dia ate atingir o fuso desejado
int jetLagOffset = calculateJetLagShift(targetTimezone, currentDay);
solarTime = now + TimeSpan(0, jetLagOffset, 0, 0);
```

### 6. Integracao com Sensores Externos

- **Sensor de luz ambiente**: Ajustar intensidade para complementar luz natural
- **Sensor de temperatura**: Ajustar CCT baseado na temperatura ambiente
- **API meteorologica**: Prever nebulosidade para ajuste proactivo

### 7. Perfis Personalizados

Guardar multiplos perfis de utilizador com preferencias:

```cpp
struct UserProfile {
  float brightnessBase;
  int morningOffset;  // Acordar mais cedo/tarde
  int eveningOffset;  // Dormir mais cedo/tarde
  bool nightShiftMode;
};
```

---

## Estrutura do Codigo

```
auto_solar.h     - Declaracoes e struct SolarColor
auto_solar.cpp   - Implementacao dos algoritmos
```

### Dependencias

- `Arduino.h`: Funcoes base (constrain, map)
- `RTClib.h`: Estrutura DateTime
- `math.h`: Funcoes trigonometricas (sin, cos, asin, log, pow)

---

## Referencias Bibliograficas

1. Meeus, J. (1991). *Astronomical Algorithms*. Willmann-Bell.
2. NOAA Earth System Research Laboratories. *Solar Calculator*. https://gml.noaa.gov/grad/solcalc/
3. Helland, T. (2012). *How to Convert Temperature (K) to RGB*.
4. CIE 015:2018. *Colorimetry, 4th Edition*.
5. Brainard, G.C. et al. (2001). *Action spectrum for melatonin regulation in humans*. J. Neuroscience.
6. Lucas, R.J. et al. (2014). *Measuring and using light in the melanopsin age*. Trends in Neurosciences.
7. CIE S 026:2018. *System for Metrology of Optical Radiation for ipRGC-Influenced Responses to Light*.
