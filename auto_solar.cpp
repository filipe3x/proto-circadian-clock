#include "auto_solar.h"

// ============= CONVERSAO TEMPERATURA DE COR =============
// Algoritmo de Tanner Helland para converter Kelvin para RGB
void colorTempToRGB(int kelvin, uint8_t &r, uint8_t &g, uint8_t &b) {
  float temp = kelvin / 100.0;
  float red, green, blue;

  // Calcular componente vermelho
  if (temp <= 66) {
    red = 255;
  } else {
    red = temp - 60;
    red = 329.698727446 * pow(red, -0.1332047592);
    red = constrain(red, 0, 255);
  }

  // Calcular componente verde
  if (temp <= 66) {
    green = temp;
    green = 99.4708025861 * log(green) - 161.1195681661;
  } else {
    green = temp - 60;
    green = 288.1221695283 * pow(green, -0.0755148492);
  }
  green = constrain(green, 0, 255);

  // Calcular componente azul
  if (temp >= 66) {
    blue = 255;
  } else if (temp <= 19) {
    blue = 0;
  } else {
    blue = temp - 10;
    blue = 138.5177312231 * log(blue) - 305.0447927307;
    blue = constrain(blue, 0, 255);
  }

  r = (uint8_t)red;
  g = (uint8_t)green;
  b = (uint8_t)blue;
}

// ============= CALCULO ELEVACAO SOLAR =============
// Algoritmo NOAA para calcular a posicao do sol
float calculateSolarElevation(DateTime now, float latitude, float longitude, int timezoneOffset) {
  // Calcular o Numero do Dia Juliano (JDN)
  int a = (14 - now.month()) / 12;
  int y = now.year() + 4800 - a;
  int m = now.month() + 12 * a - 3;

  int jdn = now.day() + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;

  // Calcular Data Juliana (JD) com fracao do dia
  double jd = jdn + (now.hour() - 12.0) / 24.0 + now.minute() / 1440.0 + now.second() / 86400.0;

  // Calcular seculo Juliano
  double t = (jd - 2451545.0) / 36525.0;

  // Calcular longitude media do sol (L0) e anomalia media (M)
  double L0 = fmod(280.46646 + t * (36000.76983 + t * 0.0003032), 360.0);
  double M = fmod(357.52911 + t * (35999.05029 - t * 0.0001537), 360.0);
  double M_rad = M * PI / 180.0;

  // Calcular equacao do centro (C)
  double C = (1.914602 - t * (0.004817 + t * 0.000014)) * sin(M_rad) +
             (0.019993 - t * 0.000101) * sin(2 * M_rad) +
             0.000289 * sin(3 * M_rad);

  // Calcular longitude verdadeira do sol (theta) e obliquidade da ecliptica (epsilon)
  double theta = L0 + C;
  double epsilon = 23.439291 - t * 0.0130042;
  double epsilon_rad = epsilon * PI / 180.0;
  double theta_rad = theta * PI / 180.0;

  // Calcular declinacao solar (delta)
  double delta = asin(sin(epsilon_rad) * sin(theta_rad));

  // Calcular equacao do tempo (E)
  double E = 4 * (L0 - 0.0057183 - atan2(tan(theta_rad), cos(epsilon_rad)) * 180.0 / PI + C);

  // Calcular tempo solar verdadeiro
  double solarTime = now.hour() * 60.0 + now.minute() + now.second() / 60.0 +
                     E + 4 * longitude - 60 * timezoneOffset;

  // Calcular angulo horario (H)
  double H = (solarTime / 4.0 - 180.0) * PI / 180.0;

  // Calcular latitude em radianos
  double phi = latitude * PI / 180.0;

  // Calcular elevacao solar
  double elevation = asin(sin(phi) * sin(delta) + cos(phi) * cos(delta) * cos(H));

  // Converter para graus e retornar
  return elevation * 180.0 / PI;
}

// ============= MAPEAMENTO ELEVACAO PARA COR =============
// Mapeia a elevacao solar para cor e brilho que simulam luz natural
SolarColor mapSolarElevationToColor(float elevation) {
  SolarColor color;
  int colorTemp;

  // Noite profunda - desligado
  if (elevation < -18.0) {
    colorTemp = 0;
    color.r = 0; color.g = 0; color.b = 0;
    color.brightness = 0;
  }
  // Crepusculo astronomico - vermelho muito suave
  else if (elevation < -12.0) {
    colorTemp = 1500;
    float t = map((int)(elevation * 10), -180, -120, 0, 40);
    colorTempToRGB(colorTemp, color.r, color.g, color.b);
    color.brightness = t;
  }
  // Crepusculo nautico - vermelho a laranja
  else if (elevation < -6.0) {
    colorTemp = map((int)(elevation * 10), -120, -60, 1500, 2000);
    float t = map((int)(elevation * 10), -120, -60, 40, 80);
    colorTempToRGB(colorTemp, color.r, color.g, color.b);
    color.brightness = t;
  }
  // Crepusculo civil - laranja a branco quente
  else if (elevation < -0.833) {
    colorTemp = map((int)(elevation * 10), -60, -8, 2000, 2800);
    float t = map((int)(elevation * 10), -60, -8, 80, 140);
    colorTempToRGB(colorTemp, color.r, color.g, color.b);
    color.brightness = t;
  }
  // Nascer/por do sol - branco quente
  else if (elevation < 6.0) {
    colorTemp = map((int)(elevation * 10), -8, 60, 2800, 3800);
    float t = map((int)(elevation * 10), -8, 60, 140, 200);
    colorTempToRGB(colorTemp, color.r, color.g, color.b);
    color.brightness = t;
  }
  // Manha - branco brilhante quente
  else if (elevation < 15.0) {
    colorTemp = map((int)(elevation * 10), 60, 150, 3800, 4800);
    float t = map((int)(elevation * 10), 60, 150, 200, 230);
    colorTempToRGB(colorTemp, color.r, color.g, color.b);
    color.brightness = t;
  }
  // Manha alta - branco brilhante
  else if (elevation < 30.0) {
    colorTemp = map((int)(elevation * 10), 150, 300, 4800, 5500);
    float t = map((int)(elevation * 10), 150, 300, 230, 250);
    colorTempToRGB(colorTemp, color.r, color.g, color.b);
    color.brightness = t;
  }
  // Dia - luz do dia
  else if (elevation < 50.0) {
    colorTemp = map((int)(elevation * 10), 300, 500, 5500, 5900);
    color.brightness = 255;
    colorTempToRGB(colorTemp, color.r, color.g, color.b);
  }
  // Meio-dia - luz do dia total
  else {
    colorTemp = 6500;
    color.brightness = 255;
    colorTempToRGB(colorTemp, color.r, color.g, color.b);
  }

  color.colorTemp = colorTemp;
  return color;
}
