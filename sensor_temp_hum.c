#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "st7789/st7789.h"

#define SENSOR_PORT i2c0
#define SDA_TEMP 0
#define SCL_TEMP 1
#define SENSOR_PORT_ADRESS 0x38
#define COLOR_BLACK 0x0000
#define COLOR_GREEN 0x07E0
#define COLOR_RED 0xF800

// Dimensões do display
const int lcd_width = 240;
const int lcd_height = 320;

// Configuração do display 
const struct st7789_config lcd_config = {
    .spi = spi0,
    .gpio_din = PICO_DEFAULT_SPI_TX_PIN,
    .gpio_clk = PICO_DEFAULT_SPI_SCK_PIN,
    .gpio_cs = -1,
    .gpio_dc  = 4,
    .gpio_rst = 20
};

// Cabeçalho das funções
void exibir_dados(float temperatura, float humidade);
bool aht10_read(float *temperature, float *humidity);
void aht10_trigger_measurement();
void aht10_init();
void setup();

int main() {
  setup();

  aht10_init();

  float temp, hum;

  while (true) {
    aht10_trigger_measurement();

    if (aht10_read(&temp, &hum)) {
      printf("Temperatura: %.2f °C, Umidade: %.2f %%\n", temp, hum);
      exibir_dados(temp, hum); // Exibe os dados no display OLED
    } else {
      printf("Erro ao ler AHT10\n");
    }

    sleep_ms(1000);
  }
}

// Configurações dos periféricos
void setup() {
    stdio_init_all();

    st7789_init(&lcd_config, lcd_width, lcd_height);
    st7789_fill(COLOR_BLACK);

    sleep_ms(4000);

    i2c_init(SENSOR_PORT, 400*1000);

    gpio_set_function(SDA_TEMP, GPIO_FUNC_I2C);
    gpio_set_function(SCL_TEMP, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_TEMP);
    gpio_pull_up(SCL_TEMP);

    st7789_clear();
}

// Inicializa o sensor AHT10
void aht10_init() {
    // Envia comando de inicialização
    uint8_t init_cmd[3] = {0xBE, 0x08, 0x00};
    i2c_write_blocking(SENSOR_PORT, SENSOR_PORT_ADRESS, init_cmd, 3, false);
    sleep_ms(10);
}

// Prepara o sensor para ler dados de temperatura e umidade
void aht10_trigger_measurement() {
    uint8_t trigger[3] = {0xAC, 0x33, 0x00};
    i2c_write_blocking(SENSOR_PORT, SENSOR_PORT_ADRESS, trigger, 3, false);
    sleep_ms(80);  
}

// Ler a temperatura e umidade do sensor
bool aht10_read(float *temperature, float *humidity) {
    uint8_t data[6] = {0};
    int result = i2c_read_blocking(SENSOR_PORT, SENSOR_PORT_ADRESS, data, 6, false);
    if (result != 6) return false;

    uint32_t raw_hum = ((uint32_t)(data[1]) << 12) | ((uint32_t)(data[2]) << 4) | ((data[3] & 0xF0) >> 4);
    uint32_t raw_temp = ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)(data[4]) << 8) | data[5];

    *humidity = ((float)raw_hum / (2 << 19)) * 100.0; 
    *temperature = ((float)raw_temp / (2 << 19)) * 200.0 - 50.0; 

    return true;
}

// Imprime os dados no display LCD
void exibir_dados(float temperatura, float umidade) {
    char buffer_temp[21];
    char buffer_hum[16];

    snprintf(buffer_temp, sizeof(buffer_temp), 
        "TEMPERATURA: %.2f C", temperatura);
    snprintf(buffer_hum, sizeof(buffer_hum), 
        "UMIDADE: %.2f%%", umidade);

    st7789_clear();

    if(temperatura >= 30.0 && umidade >= 44.0) {
      st7789_draw_text(20, 205, "TEMPERATURA ALTA", COLOR_RED, COLOR_BLACK, 2);
      st7789_draw_text(40, 180, "UMIDADE BAIXA", COLOR_RED, COLOR_BLACK, 2);
    } else if(temperatura >= 30.0) {
      st7789_draw_text(20, 205, "TEMPERATURA ALTA", COLOR_RED, COLOR_BLACK, 2);
    } else if(umidade >= 44.0) {
      st7789_draw_text(40, 180, "UMIDADE ALTA", COLOR_RED, COLOR_BLACK, 2);
    }

    st7789_draw_text(0, 120, buffer_temp, COLOR_GREEN, COLOR_BLACK, 2);
    st7789_draw_text(0, 145, buffer_hum, COLOR_GREEN, COLOR_BLACK, 2);
}