#ifndef AUTO_SOLAR_H
#define AUTO_SOLAR_H

#include <Arduino.h>
#include <RTClib.h>

// ============= ESTRUTURA DE COR SOLAR =============
struct SolarColor {
  uint8_t r, g, b;        // Valores RGB (0-255)
  uint8_t brightness;     // Nivel de brilho (0-255)
  int colorTemp;          // Temperatura de cor em Kelvin
};

// ============= FUNCOES =============

/**
 * Converte temperatura de cor (Kelvin) para valores RGB
 * Usa o algoritmo de Tanner Helland
 * @param kelvin Temperatura de cor (1500-6500K tipico)
 * @param r Referencia para valor vermelho (0-255)
 * @param g Referencia para valor verde (0-255)
 * @param b Referencia para valor azul (0-255)
 */
void colorTempToRGB(int kelvin, uint8_t &r, uint8_t &g, uint8_t &b);

/**
 * Calcula a elevacao solar usando o algoritmo NOAA
 * @param now Data/hora atual
 * @param latitude Latitude em graus (-90 a 90)
 * @param longitude Longitude em graus (-180 a 180)
 * @param timezoneOffset Offset do fuso horario em horas
 * @return Elevacao solar em graus (-90 a 90)
 */
float calculateSolarElevation(DateTime now, float latitude, float longitude, int timezoneOffset);

/**
 * Mapeia a elevacao solar para uma cor e brilho apropriados
 * Simula a luz natural do sol ao longo do dia
 *
 * Intervalos de elevacao:
 *   < -18 graus: Noite (desligado)
 *   -18 a -12: Crepusculo astronomico (vermelho profundo)
 *   -12 a -6: Crepusculo nautico (vermelho a laranja)
 *   -6 a -0.833: Crepusculo civil (laranja a quente)
 *   -0.833 a 6: Nascer/por do sol (branco quente)
 *   6 a 15: Manha (branco brilhante quente)
 *   15 a 30: Manha alta (branco brilhante)
 *   30 a 50: Dia (luz do dia)
 *   > 50: Meio-dia (luz do dia total)
 *
 * @param elevation Elevacao solar em graus
 * @return SolarColor com RGB, brilho e temperatura de cor
 */
SolarColor mapSolarElevationToColor(float elevation);

#endif // AUTO_SOLAR_H
