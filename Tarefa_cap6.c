// Código para a tarefa do dia 03/02
// Tarefa 1 Capítulo 4 Unidade 6
// Aluna: Maryana Souza Silveira
// Este código lê um caractere digitado no monitor serial e o exibe do display SSD1306.
// OBS: Ele só pode mostrar um caractere por vez.
// Caso o caractere seja um número, ele também é exibido na matriz de leds WS2812.
// Além disso, também acende os leds rgb quando um botão é pressionado e imprime 
// essa mudança no display. Caso o botão A seja pressionado, o led verde é alterado.
// Caso o botão B seja pressionado, a alteração ocorre no led azul.
// As mudanças de estado do led também são notificadas no monitor serial.

#include <stdio.h> // Biblioteca para entrada e saída padrão
#include <stdlib.h> // Biblioteca padrão do C
#include "pico/stdlib.h" // Biblioteca do Pico para funções padrão
#include "hardware/i2c.h" // Inclui a biblioteca I2c para comunicação
#include "hardware/pio.h" // Biblioteca para controle de PIO
#include "auxiliares/ssd1306.h" // Biblioteca do display SSD1306
#include "auxiliares/font.h" //Biblioteca de definição para os caracteres no display
#include "Tarefa_cap6.pio.h" // Código do programa para controle de LEDs WS2812

#define I2C_PORT i2c1 // Define o barramento I2C a ser utilizado (i2c1)
#define I2C_SDA 14 // Define o pino SDA para I2C
#define I2C_SCL 15 // Define o pino SCL para I2C
#define endereco 0x3C // Define o endereço do dispositivo I2c

#define LED_GREEN 11 // Pino do LED verde RGB
#define LED_BLUE 12 // Pino do LED azul RGB
#define LED_RED 13 // Pino do LED vermelho RGB 
#define button_A 5 // Pino do botão A
#define button_B 6 // Pino do botão B

#define numLeds 25 // Número de LEDs WS2812
#define matriz_leds 7 // Pino de controle dos LEDs WS2812

// Variável global para armazenar a cor (Entre 0 e 255 para intensidade)
uint8_t led_r = 60; // Intensidade do vermelho
uint8_t led_g = 0; // Intensidade do verde
uint8_t led_b = 60; // Intensidade do azul

ssd1306_t ssd; // Inicializa a estrutura do display;

void initial_configs(); // Função para realizar as configurações iniciais do programa
void gpio_irq_handler(uint gpio, uint32_t events); // Função da interrupção
static inline void put_pixel(uint32_t pixel_grb); // Função para atualizar um LED
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b); // Função para converter RGB para uint32_t
void printnum(uint8_t r, uint8_t g, uint8_t b, char carac); // Função para imprimir o número digitado nos LEDs WS2812

int main()
{
  initial_configs(); // Realiza as inicializações dos pinos
  
  gpio_set_irq_enabled_with_callback(button_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler); // Habilita a interrupção no botão A
  gpio_set_irq_enabled_with_callback(button_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler); // Habilita a interrupção no botão B

  while (true)
  {
    if (stdio_usb_connected()){ // Checa se o USB está conectado 
      char c; // Variável para leitura dos caracteres
      if (scanf("%c", &c) == 1){ // Lê caractere da entrada padrão

        // Limpa o display.
        ssd1306_fill(&ssd, false);
        ssd1306_send_data(&ssd);

        ssd1306_draw_char(&ssd, c, 63, 31); // Desenha a letra  
        ssd1306_send_data(&ssd); // Atualiza o display
      }
      printnum(led_r, led_g, led_b, c); // Imprime na matriz de leds WS2812 caso o caracter digitado seja um número
    }
    sleep_ms(500); // Pausa de 500ms para evitar sobrecarga no processador
  }
}

void initial_configs(){ //Função para inicializar as configurações
  stdio_init_all(); // Inicializa a comunicação serial

  gpio_init(LED_GREEN); // Inicializa o pino do led verde
  gpio_set_dir(LED_GREEN, GPIO_OUT); // Define o pino do led verde como saída
  gpio_init(LED_BLUE); // Inicializa o pino do led azul
  gpio_set_dir(LED_BLUE, GPIO_OUT); // Define o pino do led azul como saída
  gpio_init(LED_RED); // Inicializa o pino do led vermelho
  gpio_set_dir(LED_RED, GPIO_OUT); // Define o pino do led vermelho como saída

  gpio_init(button_A); // Inicializa o pino do botão A
  gpio_set_dir(button_A, GPIO_IN); // Define o pino do botão A como entrada
  gpio_pull_up(button_A); // Habilita o resistor de pull-up do botão A

  gpio_init(button_B); // Inicializa o pino do botão B
  gpio_set_dir(button_B, GPIO_IN); // Define o pino do botão B como entrada
  gpio_pull_up(button_B); // Habilita o resistor de pull-up do botão B

  //Inicializa o PIO
  PIO pio = pio0; 
  int sm = 0; 
  uint offset = pio_add_program(pio, &ws2812_program);

  // Inicializa o programa para controle dos LEDs WS2812
  ws2812_program_init(pio, sm, offset, matriz_leds, 800000, false); // Inicializa o programa para controle dos LEDs WS2812
  
  // Inicialização I2C com 400Khz
  i2c_init(I2C_PORT, 400 * 1000);

  // Define a função dos pinos GPIO para I2C
  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); 
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

  gpio_pull_up(I2C_SDA); // Habilita o resistor de pull-up do SDA
  gpio_pull_up(I2C_SCL); // Habilita o resistor de pull-up do SCL

  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd); // Configura o display
  ssd1306_send_data(&ssd); // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);
}

void gpio_irq_handler(uint gpio, uint32_t events) { //Função de interrupção
  static uint32_t last_interrupt_time = 0;  // Variável para armazenar o último tempo de interrupção
  uint32_t current_time = get_absolute_time(); // Variável para armazenar o tempo atual

  // Verifica se o tempo entre interrupções é maior que 200ms
  if (current_time - last_interrupt_time > 200000) {  // 200ms de debounce
      printnum(led_r, led_g, led_b, 'a'); // Apaga os leds WS2812 caso estejam acesos
      // Limpa o display.
      ssd1306_fill(&ssd, false);
      ssd1306_send_data(&ssd);

      if (gpio == button_A) { // Caso a interrupção seja no botão A
          bool estado = gpio_get(LED_GREEN); // Verifica o estado do led e armazena em uma variável
          gpio_put(LED_GREEN, !estado); // Troca o estado do led

          if(estado == 0){ // Caso o led esteja inicialmente apagado
            // Informa que o led foi aceso
            ssd1306_draw_string(&ssd, "Led verde", 24,16);
            ssd1306_draw_string(&ssd, "ligado", 24,32);
            ssd1306_send_data(&ssd); // Atualiza o display
            printf("O led verde foi aceso!\n");
          }else{ // Caso o led esteja inicialmente aceso
            //Informa que o led foi apagado
            ssd1306_draw_string(&ssd, "Led verde", 24,16);
            ssd1306_draw_string(&ssd, "desligado", 24,32);
            ssd1306_send_data(&ssd); // Atualiza o display
            printf("O led verde foi apagado!\n");
          }
      }
      if (gpio == button_B) { // Caso a interrupção seja no botão B, realiza o mesmo processo acima para o led azul
        bool estado = gpio_get(LED_BLUE); 
        gpio_put(LED_BLUE, !estado);
        if(estado == 0){
          ssd1306_draw_string(&ssd, "Led azul", 24,16);
          ssd1306_draw_string(&ssd, "ligado", 24,32);
          ssd1306_send_data(&ssd); // Atualiza o display

          printf("O led azul foi aceso!\n");
        }else{
          ssd1306_draw_string(&ssd, "Led azul", 24,16);
          ssd1306_draw_string(&ssd, "desligado", 24,32);
          ssd1306_send_data(&ssd); // Atualiza o display
          printf("O led azul foi apagado!\n");
        }
      }
      last_interrupt_time = current_time;  // Atualiza o tempo da última interrupção
  }
}

static inline void put_pixel(uint32_t pixel_grb) // Função para atualizar um LED
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u); // Atualiza o LED com a cor fornecida
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) // Função para converter RGB para uint32_t
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b); // Retorna a cor em formato uint32_t 
}

void printnum(uint8_t r, uint8_t g, uint8_t b, char carac) // Função para imprimir o número digitado na matriz WS2812
{ //Esta função recebe o caracter que foi digitado e verifica se ele é um número ou não.
  //Caso seja, ela imprime o número na matriz WS2812, caso contrário, ela apaga dos leds WS2812.

    // Define a cor com base nos parâmetros fornecidos
    uint32_t color = urgb_u32(r, g, b);

    // Define os padrões de LEDs para os números de 0 a 9
    bool num_0[numLeds] = { 
        0, 1, 1, 1, 0, 
        0, 1, 0, 1, 0, 
        0, 1, 0, 1, 0, 
        0, 1, 0, 1, 0, 
        0, 1, 1, 1, 0
    };

    bool num_1[numLeds] = { 
        0, 1, 1, 1, 0, 
        0, 0, 1, 0, 0, 
        0, 0, 1, 0, 0, 
        0, 1, 1, 0, 0, 
        0, 0, 1, 0, 0
    };

    bool num_2[numLeds] = {
        0, 1, 1, 1, 0, 
        0, 1, 0, 0, 0, 
        0, 1, 1, 1, 0, 
        0, 0, 0, 1, 0, 
        0, 1, 1, 1, 0
    };

    bool num_3[numLeds] = {
        0, 1, 1, 1, 0, 
        0, 0, 0, 1, 0, 
        0, 1, 1, 1, 0, 
        0, 0, 0, 1, 0, 
        0, 1, 1, 1, 0
    };

    bool num_4[numLeds] = {
        0, 1, 0, 0, 0, 
        0, 0, 0, 1, 0, 
        0, 1, 1, 1, 0, 
        0, 1, 0, 1, 0, 
        0, 1, 0, 1, 0
    };

    bool num_5[numLeds] = {
        0, 1, 1, 1, 0, 
        0, 0, 0, 1, 0, 
        0, 1, 1, 1, 0, 
        0, 1, 0, 0, 0, 
        0, 1, 1, 1, 0
    };

    bool num_6[numLeds] = {
        0, 1, 1, 1, 0, 
        0, 1, 0, 1, 0, 
        0, 1, 1, 1, 0, 
        0, 1, 0, 0, 0, 
        0, 1, 1, 1, 0
    };

    bool num_7[numLeds] = {
        0, 1, 0, 0, 0, 
        0, 0, 0, 1, 0, 
        0, 1, 0, 1, 0, 
        0, 1, 0, 1, 0, 
        0, 1, 1, 1, 0
    };

    bool num_8[numLeds] = {
        0, 1, 1, 1, 0, 
        0, 1, 0, 1, 0, 
        0, 1, 1, 1, 0, 
        0, 1, 0, 1, 0, 
        0, 1, 1, 1, 0
    };

    bool num_9[numLeds] = {
        0, 1, 1, 1, 0, 
        0, 0, 0, 1, 0, 
        0, 1, 1, 1, 0, 
        0, 1, 0, 1, 0, 
        0, 1, 1, 1, 0
    };
//padrão de leds apagados
    bool apagado[numLeds] = { //será utilizado quando o caractere digitado não for um número
      0, 0, 0, 0, 0, 
      0, 0, 0, 0, 0, 
      0, 0, 0, 0, 0, 
      0, 0, 0, 0, 0, 
      0, 0, 0, 0, 0
  };

    // Array de ponteiros para os números de 0 a 9 e para apagar os leds
    bool *numeros[11] = {
        num_0, num_1, num_2, num_3, num_4, num_5, num_6, num_7, num_8, num_9, apagado
    };

    int numero; // Variável para identificar qual número foi digitado
    switch(carac){ // Verifica qual caractere foi digitado
      case '0': // Caso o número 0 seja digitado
        numero =  0; //Armazena o valor do número numa variável de auxílio para acender os leds
        break; // Interrompe o case e sai do switch
      //Repete o processo acima para o caso de outros números sejam digitados
      case '1':
        numero =  1;
        break;
      case '2':
        numero = 2;
        break;
      case '3':
        numero = 3;
        break;
      case '4':
        numero = 4;
        break;
      case '5':
        numero = 5;
        break;
      case '6':
        numero = 6;
        break;
      case '7':
        numero = 7;
        break;
      case '8':
        numero = 8;
        break;
      case '9':
        numero = 9;
        break;
      //Caso o caractere digitado não seja um número
      default:
        numero = 10; // Armazena o número 10 na variável, que será utilizada para apagar os leds
        break;
    }

    bool *num = numeros[numero];  // Seleciona o padrão de leds com base no número digitado
    // Atualiza os LEDs com base no padrão do número escolhido
    for (int i = 0; i < numLeds; i++)
    {
        if (num[i]) 
            put_pixel(color);  // Liga o LED
        else
            put_pixel(0);  // Desliga o LED
    }
}
