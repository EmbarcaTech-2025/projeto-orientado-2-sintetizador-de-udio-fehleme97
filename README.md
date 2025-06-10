# 🎵 PROJETO: SINTETIZADOR DE ÁUDIO COM VISUALIZAÇÃO EM TEMPO REAL - BITDOGLAB (PICO W)

Autores:
- FELIPE LEME CORREA DA SILVA

Instituição: EmbarcaTech - HBr - Campinas

# 📦 SOBRE O PROJETO

Este projeto implementa um sistema de captura e reprodução de áudio no BitDogLab (Raspberry Pi Pico W), com visualização em tempo real no display OLED SSD1306. O sistema realiza:

- Captura de áudio via ADC no GPIO 28.
- Armazenamento temporário e reprodução via PWM no GPIO 21.
- Controle por botões físicos (GPIOs 5 e 6).
- Feedback visual com LED RGB (GPIOs 11, 12 e 13).
- Visualização gráfica das amostras no display OLED.
- Estrutura modular com separação entre captura, renderização e reprodução.

# 🎬 VIDEO EXPLICATIVO

LINK: 
```
https://youtu.be/oAQh0NYRnBo
```

# 🗂️ Estrutura do Projeto

```
/sintetizador_de_audio
│── inc/ # Bibliotecas personalizadas (ssd1306.h, ssd1306_font.h, etc.)
│── sintetizador_de_audio.c # Código fonte principal
│── CMakeLists.txt # Configuração de compilação
│── README.md # Este documento
│── LICENSE.txt # Licença do projeto
```

# 🛠️ Funcionalidades Implementadas

- ✅ Captura de áudio via ADC (GPIO 28)  
- ✅ Reprodução via PWM (GPIO 21)  
- ✅ Visualização em tempo real no OLED  
- ✅ Controle por botões A (gravar) e B (reproduzir)  
- ✅ Feedback visual via LED RGB  
- ✅ Filtro digital básico para redução de ruído  
- ✅ Interface limpa até o acionamento por botão  

# 🌐 Configurações de Hardware

- GPIO 28: Entrada de áudio (microfone)  
- GPIO 21: Saída PWM de reprodução  
- GPIOs 5 e 6: Botões A e B (Gravação e Reprodução)  
- GPIOs 11, 12, 13: LED RGB  
- I2C1 (GPIOs 14 e 15): OLED SSD1306  

# 📊 Fluxo de Funcionamento

1. Inicialização do sistema e periféricos.
2. Espera pela interação do usuário via botão A.
3. Início da gravação do sinal analógico.
4. Finalização com botão A novamente.
5. Reprodução do áudio captado ao pressionar botão B.
6. Visualização gráfica da captação via OLED.

# 🖥️ Execução e Gravação no Pico

1. Compilar com:

```
mkdir build
cd build
cmake ..
make
``` 

2. Copiar o ".uf2" gerado para o Raspberry Pi Pico W.

3. Monitorar o funcionamento via Serial Monitor ou visualmente no OLED.

# 📌 Tecnologias Utilizadas

- C (Pico SDK)  
- Display OLED SSD1306 (via I2C)  
- ADC e PWM da RP2040  
- BitDogLab Framework  

# 📜 Licença

- GNU GENERAL PUBLIC LICENSE
