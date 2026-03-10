#pragma once
/**
 * ============================================================
 * SOUND.H — Motor Chiptune LSDJ-inspirado para ESP32
 * ============================================================
 *
 * Arquitectura:
 *   Hardware:  BZ1 (piezo passivo 4-5kHz) via Q1 (MMBT2222A) em GPIO18
 *   Geração:   LEDC PWM (square wave) + timer ISR a 512 Hz
 *   Polifonia: Arpeggio por time-division (simula acordes como LSDJ)
 *   Noise:     LFSR de 15 bits (igual ao Game Boy CH4)
 *
 * Formato de dados (inspirado em LSDJ):
 *   Song    → sequência de Chain[] por canal
 *   Chain   → até 16 referências a Pattern[]
 *   Pattern → 16 Steps de {nota, comando, valor}
 *   Step    → {uint8_t note, uint8_t cmd, uint8_t val}
 *
 * Notas: índice 0 = silêncio | 1-96 = C2 a B9 | 97+ = noise/special
 * BPM:   60-240 | padrão = 120 BPM
 *
 * Comandos de efeito (cmd byte — estilo LSDJ):
 *   CMD_NONE   0x00  — Sem efeito
 *   CMD_ARP    0x01  — Arpeggio: val = 0xXY (semitom1 | semitom2)
 *   CMD_VIB    0x02  — Vibrato:  val = 0xSD (speed | depth)
 *   CMD_DUTY   0x03  — Duty cycle: val = 0-3 (12/25/50/75%)
 *   CMD_ENV    0x04  — Volume envelope: val = 0xAD (attack | decay)
 *   CMD_POR    0x05  — Portamento: val = velocidade
 *   CMD_HOP    0x0F  — Hop (salta para step): val = step destino
 *
 * ============================================================
 */

#include <Arduino.h>
#include <stdint.h>

// ============================================================
// LIMITES DO SEQUENCIADOR
// ============================================================
#define SND_MAX_PATTERNS   32   // Padrões por song
#define SND_MAX_CHAINS     16   // Chains por canal
#define SND_PATTERN_LEN    16   // Steps por padrão
#define SND_CHAIN_LEN      16   // Padrões por chain
#define SND_SONG_LEN       16   // Chains por song (posições)
#define SND_NUM_CHANNELS    2   // CH1 (pulse) + CH4 (noise)

// ============================================================
// COMANDOS DE EFEITO
// ============================================================
#define CMD_NONE  0x00
#define CMD_ARP   0x01   // Arpeggio
#define CMD_VIB   0x02   // Vibrato
#define CMD_DUTY  0x03   // Duty cycle
#define CMD_ENV   0x04   // Envelope
#define CMD_POR   0x05   // Portamento
#define CMD_HOP   0x0F   // Hop / loop

// ============================================================
// NOTAS
// ============================================================
#define NOTE_REST  0x00
#define NOTE_C2    1
// ... até NOTE_B9 = 96
// Noise trigger
#define NOTE_NOISE 0x61   // 97 — activa modo LFSR

// Instrumento (duty cycle)
#define DUTY_12   0
#define DUTY_25   1
#define DUTY_50   2
#define DUTY_75   3

// ============================================================
// ESTRUTURAS DE DADOS
// ============================================================

/** Um passo de um padrão */
typedef struct {
    uint8_t note;   ///< Nota (0=rest, 1-96=C2-B9, 97=noise)
    uint8_t cmd;    ///< Comando de efeito (CMD_*)
    uint8_t val;    ///< Valor do comando
    uint8_t inst;   ///< Instrumento (duty cycle DUTY_*)
} SndStep;

/** Um padrão de 16 steps */
typedef struct {
    SndStep steps[SND_PATTERN_LEN];
    uint8_t len;    ///< Número de steps activos (1-16)
} SndPattern;

/** Uma chain: sequência de padrões */
typedef struct {
    uint8_t patterns[SND_CHAIN_LEN];   ///< Índices de padrão (0xFF = fim)
    uint8_t len;
} SndChain;

/** Uma song completa */
typedef struct {
    const SndChain  *chains;    ///< Array de chains
    const SndPattern *patterns; ///< Array de padrões
    uint8_t          tempo;     ///< BPM (60-240)
    uint8_t          num_chains;
    uint8_t          num_patterns;
} SndSong;

// ============================================================
// API PÚBLICA
// ============================================================

/** Inicializa o motor de som (LEDC + timer ISR). Chamar em setup(). */
void snd_init();

/** Define o volume global (0-4). 0=mudo, 4=máximo. */
void snd_set_volume(uint8_t vol);

/** Toca uma nota simples (frequência Hz, duração ms). Bloqueante. */
void snd_tone(uint32_t freq_hz, uint32_t duration_ms);

/** Toca uma nota de forma não-bloqueante (duração em ticks de 512Hz). */
void snd_note_on(uint32_t freq_hz, uint8_t duty_idx, uint16_t ticks);

/** Para o som imediatamente. */
void snd_stop();

/** Toca uma song LSDJ-style (não-bloqueante, usa timer ISR). */
void snd_play_song(const SndSong *song);

/** Para a song a tocar. */
void snd_stop_song();

/** Retorna true se uma song está a tocar. */
bool snd_is_playing();

/** Toca a melodia de arranque predefinida. */
void snd_play_startup();

/** Toca o som de alarme circadiano (aurora — gradual). */
void snd_play_alarm();

/** Converte nota MIDI (0-127) para frequência em Hz. */
uint32_t snd_note_to_hz(uint8_t midi_note);

/** ISR interna do timer — NÃO chamar directamente. */
void IRAM_ATTR snd_timer_isr();
