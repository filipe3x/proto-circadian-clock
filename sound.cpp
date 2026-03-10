/**
 * ============================================================
 * SOUND.CPP — Motor Chiptune LSDJ-inspirado para ESP32
 * ============================================================
 *
 * Mapeamento hardware (clockv7 PCB):
 *   BUZZER_PIN → R6 (1kΩ) → Q1 Base (MMBT2222A)
 *   Q1 Collector → 5V_OUT → BZ1 (+)
 *   BZ1 (-) → GND
 *   D5 (MMBD4148SE) protege back-EMF
 *
 * Geração de áudio:
 *   - LEDC channel 0, timer 0, resolução 8-bit
 *   - Frequência = frequência da nota
 *   - Duty cycle = 128/255 (50%) para square wave padrão
 *   - Volume: modificar duty cycle (64/128/255) — efeito limitado em piezo
 *
 * Frame sequencer (como Game Boy):
 *   - Timer ISR a 512 Hz
 *   - Ticks para BPM: ticks_per_step = (512 * 60) / (bpm * steps_per_beat)
 *     Ex: 120 BPM, 4 steps/beat → ticks = (512*60)/(120*4) = 64 ticks/step
 *
 * Arpeggio (polifonia simulada):
 *   CMD_ARP 0xXY → cicla 3 notas: base, base+X semitons, base+Y semitons
 *   Ciclo a cada 2 ticks (256 Hz) — audível como acorde
 *
 * LFSR Noise (Game Boy CH4):
 *   - LFSR 15 bits: bit_out = bit14 XOR bit13
 *   - Quando bit_out=1: LEDC activo na freq_noise
 *   - Quando bit_out=0: LEDC mudo
 *   - Clocked a ~4 kHz (próximo da ressonância do piezo)
 *
 * ============================================================
 */

#include "sound.h"
#include "board_config.h"

// ============================================================
// TABELA DE FREQUÊNCIAS — notas MIDI 36-107 (C2-B9)
// Fórmula: f(n) = 440 * 2^((n-69)/12)
// Índice sound.h: note 1 = MIDI 36 (C2)
// ============================================================
static const uint32_t NOTE_FREQ[97] = {
    0,      // 0 = REST
    // Oitava 2 (C2-B2) — MIDI 36-47
    65,   69,   73,   78,   82,   87,   93,   98,   104,  110,  117,  123,
    // Oitava 3 (C3-B3) — MIDI 48-59
    131,  139,  147,  156,  165,  175,  185,  196,  208,  220,  233,  247,
    // Oitava 4 (C4-B4) — MIDI 60-71
    262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494,
    // Oitava 5 (C5-B5) — MIDI 72-83
    523,  554,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988,
    // Oitava 6 (C6-B6) — MIDI 84-95
    1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
    // Oitava 7 (C7-B7) — MIDI 96-107
    2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
    // Oitava 8 (C8-B8) — MIDI 108-119
    4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
    // Nota noise especial (índice 97 = NOTE_NOISE)
    4000  // Frequência base para noise (~ressonância piezo)
};

// Duty cycles (de 256 para 8-bit LEDC: 0-255)
static const uint8_t DUTY_TABLE[4] = {
    32,   // 12.5% — 32/256
    64,   // 25%   — 64/256
    128,  // 50%   — 128/256
    192   // 75%   — 192/256
};

// ============================================================
// ESTADO INTERNO DO APU
// ============================================================

// --- Volume global ---
static uint8_t  g_volume    = 4;   // 0-4; 4 = máximo (duty 50%)

// --- Estado do oscilador principal ---
static uint32_t g_freq      = 0;   // Frequência actual (Hz). 0=mudo
static uint8_t  g_duty_idx  = DUTY_50;

// --- Envelope de volume (simplificado) ---
static uint8_t  g_env_vol   = 15;  // 0-15 como GB (15=máximo)
static uint8_t  g_env_timer = 0;
static int8_t   g_env_dir   = 0;   // +1 crescente, -1 decrescente, 0 estático

// --- Vibrato ---
static uint8_t  g_vib_speed = 0;
static uint8_t  g_vib_depth = 0;
static uint8_t  g_vib_timer = 0;
static int8_t   g_vib_phase = 0;

// --- Arpeggio ---
static uint8_t  g_arp_en    = 0;
static uint8_t  g_arp_semi1 = 0;  // semitom 1 (nibble alto de val)
static uint8_t  g_arp_semi2 = 0;  // semitom 2 (nibble baixo de val)
static uint8_t  g_arp_step  = 0;  // 0, 1, 2
static uint8_t  g_arp_timer = 0;

// --- Portamento ---
static uint32_t g_por_target = 0;
static uint8_t  g_por_speed  = 0;

// --- LFSR Noise (15 bits — Game Boy CH4) ---
static uint16_t g_lfsr      = 0x7FFF;  // seed != 0
static uint8_t  g_noise_mode = 0;       // 0=off, 1=on
static uint8_t  g_lfsr_div  = 0;        // clock divider counter

// --- One-shot note timer ---
static volatile int32_t g_note_ticks = 0;   // Ticks restantes. -1=infinito

// ============================================================
// SEQUENCIADOR (LSDJ-inspirado)
// ============================================================
static const SndSong   *g_song         = nullptr;
static volatile bool    g_playing      = false;
static uint8_t          g_song_pos     = 0;   // Posição na chain
static uint8_t          g_chain_pos    = 0;   // Posição dentro da chain
static uint8_t          g_pattern_pos  = 0;   // Step dentro do padrão
static uint16_t         g_tick_count   = 0;   // Ticks desde o último step
static uint16_t         g_ticks_per_step = 64;// Default 120 BPM
static uint8_t          g_cur_pattern  = 0xFF;
static uint8_t          g_cur_chain    = 0xFF;
static bool             g_hop_pending  = false;
static uint8_t          g_hop_target   = 0;

// ============================================================
// TIMER HARDWARE
// ============================================================
static hw_timer_t *g_timer = nullptr;
static portMUX_TYPE g_mux  = portMUX_INITIALIZER_UNLOCKED;

// ============================================================
// HELPERS INTERNOS
// ============================================================

/** Aplica frequência e duty ao LEDC. freq=0 desliga. */
static inline void ledc_apply(uint32_t freq, uint8_t duty_raw) {
    if (freq == 0 || duty_raw == 0) {
        ledcWrite(BUZZER_LEDC_CHANNEL, 0);
        return;
    }
    ledcSetup(BUZZER_LEDC_CHANNEL, freq, BUZZER_LEDC_BITS);
    ledcAttachPin(BUZZER_PIN, BUZZER_LEDC_CHANNEL);
    // Escalar duty pelo volume (0-4): 0=0%, 4=duty_raw
    uint8_t scaled = (uint8_t)((uint32_t)duty_raw * g_volume / 4);
    ledcWrite(BUZZER_LEDC_CHANNEL, scaled);
}

/** Lookup de frequência com semitons adicionados. */
static uint32_t freq_shift(uint32_t base_note_idx, int8_t semitones) {
    int16_t idx = (int16_t)base_note_idx + semitones;
    if (idx < 1)   idx = 1;
    if (idx > 96)  idx = 96;
    return NOTE_FREQ[idx];
}

/** Avança LFSR um passo. Retorna bit de saída (0 ou 1). */
static inline uint8_t lfsr_step() {
    uint8_t bit = ((g_lfsr >> 14) ^ (g_lfsr >> 13)) & 1;
    g_lfsr = ((g_lfsr << 1) | bit) & 0x7FFF;
    return bit;
}

/** Carrega um step do sequenciador e aplica nota+efeito. */
static void sequencer_load_step(const SndStep *step) {
    // Reset efeitos
    g_arp_en    = 0;
    g_vib_speed = 0;
    g_vib_depth = 0;
    g_por_speed = 0;
    g_noise_mode = 0;

    // Nota
    if (step->note == NOTE_REST) {
        g_freq = 0;
    } else if (step->note == NOTE_NOISE) {
        g_noise_mode = 1;
        g_freq = NOTE_FREQ[97];
    } else {
        g_freq = NOTE_FREQ[step->note];
    }
    g_duty_idx = step->inst & 0x03;

    // Comando
    switch (step->cmd) {
        case CMD_ARP:
            g_arp_en    = 1;
            g_arp_semi1 = (step->val >> 4) & 0x0F;
            g_arp_semi2 = step->val & 0x0F;
            g_arp_step  = 0;
            g_arp_timer = 0;
            break;
        case CMD_VIB:
            g_vib_speed = (step->val >> 4) & 0x0F;
            g_vib_depth = step->val & 0x0F;
            g_vib_timer = 0;
            g_vib_phase = 0;
            break;
        case CMD_DUTY:
            g_duty_idx = step->val & 0x03;
            break;
        case CMD_ENV:
            g_env_vol   = 15;
            g_env_timer = (step->val >> 4) & 0x0F;  // attack steps
            g_env_dir   = ((step->val & 0x0F) > 0) ? -1 : 1;
            break;
        case CMD_POR:
            g_por_speed  = step->val;
            g_por_target = g_freq;
            break;
        case CMD_HOP:
            g_hop_pending = true;
            g_hop_target  = step->val & 0x0F;
            break;
        default:
            break;
    }
}

/** Avança o sequenciador um step. */
static void sequencer_advance() {
    if (!g_playing || g_song == nullptr) return;

    if (g_hop_pending) {
        g_hop_pending  = false;
        g_pattern_pos  = g_hop_target;
    }

    // Encontrar chain e padrão actuais
    if (g_cur_chain == 0xFF) {
        // Início: carregar chain[song_pos]
        if (g_song_pos >= g_song->num_chains) {
            // Song terminada — loop
            g_song_pos = 0;
        }
        g_cur_chain   = g_song_pos;
        g_chain_pos   = 0;
        g_pattern_pos = 0;
    }

    const SndChain *chain = &g_song->chains[g_cur_chain];
    if (g_chain_pos >= chain->len || chain->patterns[g_chain_pos] == 0xFF) {
        // Chain terminada
        g_song_pos++;
        if (g_song_pos >= g_song->num_chains) g_song_pos = 0;
        g_cur_chain   = g_song_pos;
        g_chain_pos   = 0;
        g_pattern_pos = 0;
        chain = &g_song->chains[g_cur_chain];
    }

    g_cur_pattern = chain->patterns[g_chain_pos];
    if (g_cur_pattern >= g_song->num_patterns) {
        g_pattern_pos = 0;
        g_chain_pos++;
        return;
    }

    const SndPattern *pat = &g_song->patterns[g_cur_pattern];
    uint8_t step_idx = g_pattern_pos % pat->len;
    const SndStep *step = &pat->steps[step_idx];

    sequencer_load_step(step);

    g_pattern_pos++;
    if (g_pattern_pos >= pat->len) {
        g_pattern_pos = 0;
        g_chain_pos++;
    }
}

// ============================================================
// TIMER ISR (512 Hz — como Game Boy frame sequencer)
// ============================================================
void IRAM_ATTR snd_timer_isr() {
    portENTER_CRITICAL_ISR(&g_mux);

    static uint32_t base_note_idx = 0;  // Para arpeggio
    static uint32_t current_freq  = 0;

    // --- Sequenciador ---
    if (g_playing) {
        g_tick_count++;
        if (g_tick_count >= g_ticks_per_step) {
            g_tick_count = 0;
            // Avançar sequenciador (fora de ISR idealmente, mas é rápido)
            sequencer_advance();
            // Recalcular note index para arpeggio
            // (usar g_freq como frequência base)
            current_freq = g_freq;
            base_note_idx = 0;
            // Encontrar índice mais próximo na tabela
            for (int i = 1; i <= 96; i++) {
                if (NOTE_FREQ[i] >= g_freq) { base_note_idx = i; break; }
            }
        }
    }

    // --- One-shot note timer ---
    if (g_note_ticks > 0) {
        g_note_ticks--;
        if (g_note_ticks == 0) {
            g_freq = 0;
        }
    }

    // --- Calcular frequência de saída ---
    uint32_t out_freq = g_freq;

    // Arpeggio (cicla a cada 2 ticks = 256 Hz)
    if (g_arp_en && base_note_idx > 0) {
        g_arp_timer++;
        if (g_arp_timer >= 2) {
            g_arp_timer = 0;
            g_arp_step = (g_arp_step + 1) % 3;
        }
        switch (g_arp_step) {
            case 0: out_freq = NOTE_FREQ[base_note_idx]; break;
            case 1: out_freq = freq_shift(base_note_idx, g_arp_semi1); break;
            case 2: out_freq = freq_shift(base_note_idx, g_arp_semi2); break;
        }
    }

    // Vibrato (modulação de frequência periódica)
    if (g_vib_speed > 0 && g_vib_depth > 0 && out_freq > 0) {
        g_vib_timer++;
        if (g_vib_timer >= (8 - g_vib_speed)) {
            g_vib_timer = 0;
            g_vib_phase = (g_vib_phase + 1) & 0x07;
        }
        // Offset ±depth Hz (simplificado)
        int32_t vib_offset = (g_vib_phase < 4 ? 1 : -1) * (int32_t)(out_freq * g_vib_depth / 64);
        out_freq = (uint32_t)((int32_t)out_freq + vib_offset);
    }

    // Portamento (glide para target)
    if (g_por_speed > 0 && g_freq != g_por_target && g_por_target > 0) {
        uint32_t step = g_por_target / (uint32_t)(128 / g_por_speed + 1);
        if (step == 0) step = 1;
        if (g_freq < g_por_target) {
            g_freq = (g_freq + step > g_por_target) ? g_por_target : g_freq + step;
        } else {
            g_freq = (g_freq < step + g_por_target) ? g_por_target : g_freq - step;
        }
        out_freq = g_freq;
    }

    // Noise mode: LFSR clocked a ~4 kHz / 8 = 512 Hz
    if (g_noise_mode) {
        uint8_t bit = lfsr_step();
        // Noise: alternar entre freq_noise e silêncio conforme LFSR
        out_freq = bit ? NOTE_FREQ[97] : 0;
    }

    // --- Aplicar ao LEDC ---
    ledc_apply(out_freq, DUTY_TABLE[g_duty_idx & 0x03]);

    portEXIT_CRITICAL_ISR(&g_mux);
}

// ============================================================
// API PÚBLICA
// ============================================================

void snd_init() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    ledcSetup(BUZZER_LEDC_CHANNEL, 1000, BUZZER_LEDC_BITS);
    ledcAttachPin(BUZZER_PIN, BUZZER_LEDC_CHANNEL);
    ledcWrite(BUZZER_LEDC_CHANNEL, 0);

    // Timer a 512 Hz (período = 1953 µs)
    g_timer = timerBegin(1, 80, true);   // Timer 1, prescaler 80 (1 MHz), auto-reload
    timerAttachInterrupt(g_timer, &snd_timer_isr, true);
    timerAlarmWrite(g_timer, 1953, true);  // 1 000 000 / 1953 ≈ 512 Hz
    timerAlarmEnable(g_timer);
}

void snd_set_volume(uint8_t vol) {
    g_volume = (vol > 4) ? 4 : vol;
}

void snd_tone(uint32_t freq_hz, uint32_t duration_ms) {
    snd_note_on(freq_hz, DUTY_50, 0);
    delay(duration_ms);
    snd_stop();
}

void snd_note_on(uint32_t freq_hz, uint8_t duty_idx, uint16_t ticks) {
    portENTER_CRITICAL(&g_mux);
    g_freq       = freq_hz;
    g_duty_idx   = duty_idx & 0x03;
    g_noise_mode = 0;
    g_arp_en     = 0;
    g_vib_speed  = 0;
    g_note_ticks = (ticks == 0) ? -1 : (int32_t)ticks;
    portEXIT_CRITICAL(&g_mux);
}

void snd_stop() {
    portENTER_CRITICAL(&g_mux);
    g_freq       = 0;
    g_note_ticks = 0;
    g_noise_mode = 0;
    g_arp_en     = 0;
    portEXIT_CRITICAL(&g_mux);
    ledcWrite(BUZZER_LEDC_CHANNEL, 0);
}

void snd_play_song(const SndSong *song) {
    if (song == nullptr) return;
    snd_stop_song();

    portENTER_CRITICAL(&g_mux);
    g_song       = song;
    g_playing    = true;
    g_song_pos   = 0;
    g_chain_pos  = 0;
    g_pattern_pos = 0;
    g_tick_count = 0;
    g_cur_chain  = 0xFF;
    g_cur_pattern = 0xFF;
    g_hop_pending = false;
    // ticks_per_step = (512 * 60) / (bpm * 4)  — 4 steps por beat
    g_ticks_per_step = (uint16_t)((512UL * 60UL) / ((uint32_t)song->tempo * 4UL));
    if (g_ticks_per_step < 1) g_ticks_per_step = 1;
    portEXIT_CRITICAL(&g_mux);
}

void snd_stop_song() {
    portENTER_CRITICAL(&g_mux);
    g_playing = false;
    g_song    = nullptr;
    g_freq    = 0;
    portEXIT_CRITICAL(&g_mux);
    ledcWrite(BUZZER_LEDC_CHANNEL, 0);
}

bool snd_is_playing() {
    return g_playing;
}

uint32_t snd_note_to_hz(uint8_t midi_note) {
    // MIDI 36 = C2 = index 1 na nossa tabela
    if (midi_note < 36) return 0;
    uint8_t idx = midi_note - 35;  // MIDI 36 → idx 1
    if (idx > 96) idx = 96;
    return NOTE_FREQ[idx];
}

// ============================================================
// MELODIA DE ARRANQUE — "Proto Clock Boot"
// Inspirado no Power-On do Game Boy (escala ascendente + arpejo)
// ============================================================
void snd_play_startup() {
    // Escala pentatónica C maior ascendente
    static const uint32_t melody[] = {
        523, 659, 784, 880, 1047, 880, 784, 659,
        523, 0
    };
    static const uint16_t durations[] = {
        80, 80, 80, 80, 160, 80, 80, 80,
        200, 0
    };

    for (int i = 0; melody[i] != 0; i++) {
        snd_tone(melody[i], durations[i]);
        delay(10);
    }
    // Arpejo final C-E-G
    snd_tone(523, 60);
    snd_tone(659, 60);
    snd_tone(784, 120);
    snd_stop();
}

// ============================================================
// ALARME CIRCADIANO — "Aurora"
// Começa piano, sobe gradualmente (simula nascer do sol)
// Três fases: pulso suave → melodia → alerta
// ============================================================
void snd_play_alarm() {
    // Fase 1: Pulso suave (baixo volume, freq na ressonância do piezo)
    snd_set_volume(1);
    for (int i = 0; i < 4; i++) {
        snd_tone(4000, 200);
        delay(400);
    }

    // Fase 2: Melodia crescente (volume 2-3)
    snd_set_volume(2);
    static const uint32_t wake_mel[] = {
        440, 494, 523, 587, 659, 698, 784, 880
    };
    for (int i = 0; i < 8; i++) {
        snd_tone(wake_mel[i], 150);
        delay(20);
    }

    // Fase 3: Alerta (volume máximo, arpejo)
    snd_set_volume(4);
    for (int rep = 0; rep < 3; rep++) {
        snd_tone(880, 100);
        snd_tone(1109, 100);
        snd_tone(1319, 200);
        delay(100);
    }

    snd_set_volume(4);
    snd_stop();
}

// ============================================================
// EXEMPLO DE SONG LSDJ-STYLE
// "Circadian" — tema de demonstração
// ============================================================

// Padrões
static const SndPattern DEMO_PATTERNS[] = {
    // Padrão 0: Melodia principal (C maior)
    {
        {
            // step: note,         cmd,      val,   inst (duty)
            {37,  CMD_NONE, 0x00, DUTY_50},  // C4
            {37,  CMD_ARP,  0x47, DUTY_50},  // C4 + arpejo C-E-G
            {39,  CMD_NONE, 0x00, DUTY_50},  // D4
            {39,  CMD_VIB,  0x24, DUTY_50},  // D4 + vibrato
            {41,  CMD_NONE, 0x00, DUTY_50},  // E4
            {41,  CMD_ARP,  0x47, DUTY_50},  // E4 + arpejo
            {42,  CMD_NONE, 0x00, DUTY_50},  // F4
            {NOTE_REST, CMD_NONE, 0x00, DUTY_50},
            {44,  CMD_NONE, 0x00, DUTY_50},  // G4
            {44,  CMD_ARP,  0x47, DUTY_50},  // G4 + arpejo
            {46,  CMD_NONE, 0x00, DUTY_25},  // A4
            {46,  CMD_VIB,  0x12, DUTY_25},  // A4 + vibrato
            {48,  CMD_NONE, 0x00, DUTY_50},  // B4
            {49,  CMD_NONE, 0x00, DUTY_50},  // C5
            {49,  CMD_ARP,  0x47, DUTY_50},  // C5 + arpejo
            {NOTE_REST, CMD_NONE, 0x00, DUTY_50},
        },
        16
    },
    // Padrão 1: Percussão (noise channel via LFSR)
    {
        {
            {NOTE_NOISE, CMD_ENV, 0xF4, DUTY_50},  // Kick
            {NOTE_REST,  CMD_NONE, 0x00, DUTY_50},
            {NOTE_REST,  CMD_NONE, 0x00, DUTY_50},
            {NOTE_REST,  CMD_NONE, 0x00, DUTY_50},
            {NOTE_NOISE, CMD_ENV, 0x82, DUTY_25},  // Snare
            {NOTE_REST,  CMD_NONE, 0x00, DUTY_50},
            {NOTE_NOISE, CMD_ENV, 0x42, DUTY_12},  // Hi-hat
            {NOTE_NOISE, CMD_ENV, 0x42, DUTY_12},  // Hi-hat
            {NOTE_NOISE, CMD_ENV, 0xF4, DUTY_50},  // Kick
            {NOTE_REST,  CMD_NONE, 0x00, DUTY_50},
            {NOTE_NOISE, CMD_ENV, 0x82, DUTY_25},  // Snare
            {NOTE_REST,  CMD_NONE, 0x00, DUTY_50},
            {NOTE_NOISE, CMD_ENV, 0x42, DUTY_12},  // Hi-hat
            {NOTE_NOISE, CMD_ENV, 0x42, DUTY_12},  // Hi-hat
            {NOTE_NOISE, CMD_ENV, 0x42, DUTY_12},  // Hi-hat
            {NOTE_REST,  CMD_NONE, 0x00, DUTY_50},
        },
        16
    },
    // Padrão 2: Baixo (oitava mais baixa)
    {
        {
            {25, CMD_NONE, 0x00, DUTY_25},  // C3
            {NOTE_REST, CMD_NONE, 0x00, DUTY_25},
            {25, CMD_NONE, 0x00, DUTY_25},  // C3
            {NOTE_REST, CMD_NONE, 0x00, DUTY_25},
            {32, CMD_NONE, 0x00, DUTY_25},  // G3
            {NOTE_REST, CMD_NONE, 0x00, DUTY_25},
            {30, CMD_NONE, 0x00, DUTY_25},  // F3
            {NOTE_REST, CMD_NONE, 0x00, DUTY_25},
            {25, CMD_NONE, 0x00, DUTY_25},  // C3
            {NOTE_REST, CMD_NONE, 0x00, DUTY_25},
            {25, CMD_NONE, 0x00, DUTY_25},  // C3
            {NOTE_REST, CMD_NONE, 0x00, DUTY_25},
            {28, CMD_NONE, 0x00, DUTY_25},  // E3
            {30, CMD_NONE, 0x00, DUTY_25},  // F3
            {32, CMD_NONE, 0x00, DUTY_25},  // G3
            {NOTE_REST, CMD_NONE, 0x00, DUTY_25},
        },
        16
    },
};

// Chains
static const SndChain DEMO_CHAINS[] = {
    // Chain 0: melodia principal, loop
    { {0, 0, 0, 0, 0xFF}, 4 },
    // Chain 1: baixo, loop
    { {2, 2, 2, 2, 0xFF}, 4 },
    // Chain 2: percussão (noise), loop
    { {1, 1, 1, 1, 0xFF}, 4 },
};

// Song
const SndSong SONG_CIRCADIAN = {
    .chains      = DEMO_CHAINS,
    .patterns    = DEMO_PATTERNS,
    .tempo       = 128,
    .num_chains  = 3,
    .num_patterns = 3,
};
