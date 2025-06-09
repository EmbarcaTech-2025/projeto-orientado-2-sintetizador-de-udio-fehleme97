#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/time.h"
#include "ssd1306.h"
#include <string.h>
#include "ssd1306_font.h"
#include <math.h>

// Definição dos pinos usados no projeto
#define MICROFONE_ADC_GPIO 28         // GPIO 28 atribuido ao microfone
#define MICROFONE_ADC_INPUT 2         // Canal ADC correspondente ao GPIO 28
#define BUZZER_GPIO 21                // GPIO de saída de áudio (PWM)
#define BOTAO_A_GPIO 5                // Botão A: iniciar gravação
#define BOTAO_B_GPIO 6                // Botão B: reproduzir gravação
#define LED_R_GPIO 11                 // LED RGB - vermelho
#define LED_G_GPIO 12                 // LED RGB - verde
#define LED_B_GPIO 13                 // LED RGB - azul

const uint I2C_SDA = 14;              // Pino SDA do OLED
const uint I2C_SCL = 15;              // Pino SCL do OLED

// Parâmetros do sistema
#define SCREEN_WIDTH 128              // Largura da tela OLED
#define SCREEN_HEIGHT 64              // Altura da tela OLED
#define TAXA_AMOSTRAGEM 41000         // Taxa de amostragem do áudio em Hz
#define DURACAO 3                     // Duração máxima da gravação em segundos
#define TAMANHO_BUFFER (TAXA_AMOSTRAGEM * DURACAO) // Tamanho total do buffer de gravação

volatile uint16_t ultima_amostra = 0;


// Buffer de áudio
uint16_t buffer_audio[TAMANHO_BUFFER];
volatile uint buffer_index = 0;
bool amostrando = false;
bool aguardando_acao = true;
float volume = 1.60;                 // Fator de ganho do áudio reproduzido
uint32_t pwm_wrap = 0;

// Instâncias do display
ssd1306_t oled;
uint8_t oled_frame[1024];

// Inicializa o LED RGB
void configurar_led_rgb() {
    gpio_init(LED_R_GPIO);
    gpio_init(LED_G_GPIO);
    gpio_init(LED_B_GPIO);
    gpio_set_dir(LED_R_GPIO, GPIO_OUT);
    gpio_set_dir(LED_G_GPIO, GPIO_OUT);
    gpio_set_dir(LED_B_GPIO, GPIO_OUT);
}

// Define a cor do LED RGB
void set_rgb(bool r, bool g, bool b) {
    gpio_put(LED_R_GPIO, r);
    gpio_put(LED_G_GPIO, g);
    gpio_put(LED_B_GPIO, b);
}

// Inicializa o ADC para leitura do microfone
void init_microfone_adc() {
    adc_init();
    adc_gpio_init(MICROFONE_ADC_GPIO);
    adc_select_input(MICROFONE_ADC_INPUT);
}

// Lê uma amostra do microfone via ADC
uint16_t ler_microfone_adc() {
    return adc_read();
}

// Inicializa os botões A e B
void init_botoes() {
    gpio_init(BOTAO_A_GPIO);
    gpio_set_dir(BOTAO_A_GPIO, GPIO_IN);
    gpio_pull_up(BOTAO_A_GPIO);

    gpio_init(BOTAO_B_GPIO);
    gpio_set_dir(BOTAO_B_GPIO, GPIO_IN);
    gpio_pull_up(BOTAO_B_GPIO);
}

// Configura o PWM para reprodução de áudio
void configurar_pwm_saida() {
    gpio_set_function(BUZZER_GPIO, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_GPIO);

    uint freq_pwm = 55000;                          // Define uma frequência de PWM alta (62 kHz) para minimizar ruído audível e melhorar fidelidade.
    uint sys_clk = clock_get_hz(clk_sys);           // Obtém a frequência atual do clock do sistema
    pwm_wrap = (sys_clk / freq_pwm)-1;              // Calcula o valor máximo do contador PWM (wrap) para obter a frequência desejada:

    pwm_set_wrap(slice_num, pwm_wrap);              // Define duty inicial como 0 para evitar chiado
    pwm_set_clkdiv(slice_num, 1.0f);

    pwm_set_gpio_level(BUZZER_GPIO, 0);             // PREVENÇÃO DO CHIADO
    pwm_set_enabled(slice_num, true);
}


// Desliga a saída PWM
void desligar_pwm_saida() {
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_GPIO);

    pwm_set_gpio_level(BUZZER_GPIO, 0);     // Zera o sinal PWM
    pwm_set_enabled(slice_num, false);      // Desliga o PWM

    gpio_set_function(BUZZER_GPIO, GPIO_FUNC_SIO); // Alterna pino para GPIO comum
    gpio_set_dir(BUZZER_GPIO, GPIO_OUT);
    gpio_put(BUZZER_GPIO, 0);               // Força nível baixo no pino
}


// Função chamada periodicamente para capturar uma amostra
int64_t amostrar_callback(alarm_id_t id, void *user_data) {
    if (!amostrando) return 0;

    if (buffer_index < TAMANHO_BUFFER) {
        uint16_t amostra = ler_microfone_adc();
        ultima_amostra = amostra;                       // <-- Atualiza a amostra mais recente
        buffer_audio[buffer_index++] = amostra;
        return 1000000 / TAXA_AMOSTRAGEM;               // Agenda próxima chamada com base na taxa de amostragem (ex: a cada 24.39 us para 41 kHz)
    } else {
        amostrando = false;
        return 0;
    }
}

// Reproduz o áudio com interpolacao linear entre amostras
void reproduzir_buffer_pwm_simples() {
    set_rgb(false, true, false);        // LED azul: reprodução em andamento
    printf("Reproduzindo...\n");

    configurar_pwm_saida();             // wrap e freq já definidos corretamente

    for (int i = 0; i < TAMANHO_BUFFER; i++) {
        float amostra = buffer_audio[i] * volume;
        if (amostra > 4095.0f) amostra = 4095.0f;

        // Aplica compressão suave (normalizador dinâmico simples)
        amostra = sqrtf(amostra) * sqrtf(4095.0f);

        // Mapeamento direto linear
        uint16_t duty = (uint16_t)((amostra * pwm_wrap) / 4095.0f);

        // Ignora pulsos muito curtos
        if (duty < 3) duty = 0;

        pwm_set_gpio_level(BUZZER_GPIO, duty);
        sleep_us(1000000 / TAXA_AMOSTRAGEM);
    }

    desligar_pwm_saida(); // GPIO resetado para 0
    set_rgb(false, false, false);
    printf("Reprodução concluída.\n");
}


// Função para aplicar debounce em botão
bool botao_pressionado(uint gpio) {
    if (gpio_get(gpio) == 0) {           // Detecta pressionado (nível baixo)
        sleep_ms(20);                    
        if (gpio_get(gpio) == 0) {       // Confirma que ainda está pressionado
            while (gpio_get(gpio) == 0); // Aguarda soltar o botão
            return true;
        }
    }
    return false;
}

// Função principal do programa
int main() {
    stdio_init_all();
    sleep_ms(2000);

    // Inicialização do display OLED
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init();

    struct render_area frame_area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };

    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
    calculate_render_area_buffer_length(&frame_area);

    // Exibe mensagens iniciais no display
    ssd1306_draw_string(ssd, 0, 16, "    SISTEMA  ");
    ssd1306_draw_string(ssd, 0, 32, "   INICIANDO.. ");
    render_on_display(ssd, &frame_area);
    sleep_ms(5000);
    printf("Sistema iniciado.\n");

    init_microfone_adc();
    init_botoes();
    configurar_led_rgb();

    // Exibe mensagens de instruções iniciais
    memset(ssd, 0, ssd1306_buffer_length);
    ssd1306_draw_string(ssd, 0, 0, "   PRESSIONE ");
    ssd1306_draw_string(ssd, 0, 32, "A | Gravar ");
    ssd1306_draw_string(ssd, 0, 48, "B | Reproduzir ");
    render_on_display(ssd, &frame_area);

    printf("Pressione o botão A para gravar ou B para reproduzir.\n");

    while (1) {
        // Gravação ao pressionar o botão A
        if (botao_pressionado(BOTAO_A_GPIO)) {
            aguardando_acao = false;
            printf("Botão A pressionado! Iniciando gravação em 3 segundos...\n");
            sleep_ms(2000);

            set_rgb(false, false, true); // VERMELHO
            buffer_index = 0;
            amostrando = true;

            add_alarm_in_us(0, amostrar_callback, NULL, true);

            while (amostrando) {
                // Captura a amostra e armazena no buffer
               // uint16_t amostra = ler_microfone_adc();
               // buffer_audio[buffer_index++] = amostra;

                // Converte a amostra (0–4095) em altura de 0 a 64 pixels
                // Converte ultima_amostra (0–4095) em altura de 0 a 64 pixels
                int altura = (ultima_amostra * SCREEN_HEIGHT) / 4095;
                if (altura > 64) altura = 64;


                // Limpa tela e escreve "GRAVANDO!"
                memset(ssd, 0, ssd1306_buffer_length);
                ssd1306_draw_string(ssd, 0, 0, "    GRAVANDO!");

                // Desenha uma barra vertical no centro da tela com a altura proporcional
                int coluna_x = 64; // centro da tela
                for (int y = 63; y >= 64 - altura; y--) {
                    ssd[coluna_x + (y / 8) * SCREEN_WIDTH] |= (1 << (y % 8));
                }

                render_on_display(ssd, &frame_area);

                // Finaliza se o buffer estiver cheio
                if (buffer_index >= TAMANHO_BUFFER) {
                    amostrando = false;
                }

                tight_loop_contents();
            }

            set_rgb(true, false, false); // VERDE
            sleep_ms(2000);
            set_rgb(false, false, false);
            aguardando_acao = true;

        }

        // Reprodução ao pressionar o botão B
        if (botao_pressionado(BOTAO_B_GPIO)) {
            aguardando_acao = false;
            printf("Botão B pressionado! Iniciando reprodução.\n");

            memset(ssd, 0, ssd1306_buffer_length);
            ssd1306_draw_string(ssd, 0, 32, "  REPRODUZINDO! ");
            render_on_display(ssd, &frame_area);

            reproduzir_buffer_pwm_simples();
            aguardando_acao = true;
        }

        memset(ssd, 0, ssd1306_buffer_length);
        ssd1306_draw_string(ssd, 0, 0, "   PRESSIONE ");
        ssd1306_draw_string(ssd, 0, 32, "A | Gravar ");
        ssd1306_draw_string(ssd, 0, 48, "B | Reproduzir ");
        render_on_display(ssd, &frame_area);

        // Visualização em tempo real APENAS quando não estiver aguardando interação
        if (!amostrando && !aguardando_acao && gpio_get(BOTAO_A_GPIO) && gpio_get(BOTAO_B_GPIO)) {
            uint16_t amostra = ler_microfone_adc();
        }

        sleep_ms(10);
    }
}
