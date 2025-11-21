/*
 * Projeto: SkillBridge - Timer Pomodoro IoT
 * Versão: 5.0 (Com Logs de Debugging Detalhados)
 * Descrição: Dispositivo inteligente para gestão de produtividade.
 * Hardware: ESP32, OLED, LED RGB, Buzzer, Botões.
 * Simulador: Wokwi
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Configuração do OLED ---
#define LARGURA_TELA 128
#define ALTURA_TELA  64
#define ENDERECO_I2C 0x3C 
Adafruit_SSD1306 display(LARGURA_TELA, ALTURA_TELA, &Wire, -1);

// --- Definição dos Pinos ---
const int PIN_BUZZER = 25;
const int PIN_LED_R  = 13; 
const int PIN_LED_G  = 12; 
const int PIN_LED_B  = 14; 
const int BTN_START  = 26; 
const int BTN_PAUSE  = 27; 
const int BTN_RESET  = 33; 

// --- Configuração de Tempo (Modo Demonstração) ---
const long DURACAO_TRABALHO = 25; 
const long DURACAO_PAUSA    = 5;      
const int  LIMITE_ALERTA    = 5; 

// --- Estados ---
enum EstadoSistema { 
  OCIOSO,         
  TRABALHANDO,    
  FINALIZANDO,    
  DESCANSO,       
  PAUSADO         
};

EstadoSistema estadoAtual = OCIOSO;
EstadoSistema estadoAnterior = OCIOSO; 

// --- Variáveis Globais ---
unsigned long millisAnterior = 0; 
long contadorTimer = DURACAO_TRABALHO;
int contadorCiclos = 0;
bool ledPiscando = false; 

// Debounce
unsigned long ultimoTempoDebounce = 0;
const unsigned long ATRASO_DEBOUNCE = 200;

// ==================== FUNÇÕES AUXILIARES ====================

void definirCorLed(bool r, bool g, bool b) {
  digitalWrite(PIN_LED_R, r ? HIGH : LOW);
  digitalWrite(PIN_LED_G, g ? HIGH : LOW);
  digitalWrite(PIN_LED_B, b ? HIGH : LOW);
}

void atualizarDisplay(String textoTopo, String textoCentro, String textoBaixo) {
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(textoTopo);

  if(textoBaixo.startsWith("Ciclos")) {
     display.setCursor(70, 0);
     display.print(textoBaixo);
  }

  display.setTextSize(2);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(textoCentro, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((LARGURA_TELA - w) / 2, 25);
  display.print(textoCentro);
  
  display.display();
}

void executarChecagemSistema() {
  Serial.println(F("\n[SISTEMA] Iniciando Autoteste de Hardware..."));

  Serial.print(F("[TESTE] LED Vermelho... "));
  definirCorLed(true, false, false);
  delay(200);
  Serial.println(F("OK"));

  Serial.print(F("[TESTE] LED Verde...    "));
  definirCorLed(false, true, false);
  delay(200);
  Serial.println(F("OK"));

  Serial.print(F("[TESTE] LED Azul...     "));
  definirCorLed(false, false, true);
  delay(200);
  Serial.println(F("OK"));

  Serial.print(F("[TESTE] Buzzer...       "));
  tone(PIN_BUZZER, 1000, 100);
  delay(100);
  Serial.println(F("OK"));

  definirCorLed(false, false, false);
  Serial.println(F("[SISTEMA] Autoteste Completo. Sistema Pronto.\n"));
}

// ==================== LÓGICA DE CONTROLE E LOGS ====================

void iniciarSessaoTrabalho() {
  Serial.println(F("[ESTADO] Alterado para: TRABALHANDO (Foco Iniciado)"));
  estadoAtual = TRABALHANDO;
  contadorTimer = DURACAO_TRABALHO;
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(30, 25);
  display.print(F("INICIAR!"));
  display.display();
  delay(1000); 
  
  millisAnterior = millis();
}

void iniciarSessaoDescanso() {
  contadorCiclos++; 
  
  // LOG DETALHADO DA MUDANÇA
  Serial.println(F("----------------------------------------"));
  Serial.println(F("[ESTADO] Alterado para: DESCANSO"));
  Serial.print(F("[DADOS]  Ciclo Completado! Total Acumulado: "));
  Serial.println(contadorCiclos);
  Serial.println(F("----------------------------------------"));

  estadoAtual = DESCANSO;
  contadorTimer = DURACAO_PAUSA;
  noTone(PIN_BUZZER);
}

void resetarSistema() {
  Serial.println(F("[SISTEMA] Solicitado RESET total do dispositivo."));
  estadoAtual = OCIOSO;
  contadorTimer = DURACAO_TRABALHO;
  contadorCiclos = 0;
  noTone(PIN_BUZZER);
  definirCorLed(false, false, true); 
  atualizarDisplay("SkillBridge", "Pronto?", "Ciclos: 0");
  Serial.println(F("[ESTADO] Sistema aguardando inicio (OCIOSO)"));
}

String formatarTempo(long segundos) {
  long min = segundos / 60;
  long sec = segundos % 60;
  String sMin = (min < 10) ? "0" + String(min) : String(min);
  String sSec = (sec < 10) ? "0" + String(sec) : String(sec);
  return sMin + ":" + sSec;
}

void gerenciarEntradas() {
  if (millis() - ultimoTempoDebounce < ATRASO_DEBOUNCE) return;

  // Botão START
  if (digitalRead(BTN_START) == LOW) {
    Serial.println(F("[ENTRADA] Botao START pressionado."));
    if (estadoAtual == OCIOSO) {
      iniciarSessaoTrabalho();
    } else if (estadoAtual == PAUSADO) {
      Serial.println(F("[ESTADO] Retomando da Pausa..."));
      estadoAtual = estadoAnterior; 
    } else {
      Serial.println(F("[AVISO] Botao ignorado (Timer ja esta rodando)."));
    }
    ultimoTempoDebounce = millis();
  }

  // Botão PAUSE
  if (digitalRead(BTN_PAUSE) == LOW) {
    Serial.println(F("[ENTRADA] Botao PAUSE pressionado."));
    if (estadoAtual == TRABALHANDO || estadoAtual == FINALIZANDO || estadoAtual == DESCANSO) {
      estadoAnterior = estadoAtual;
      estadoAtual = PAUSADO;
      definirCorLed(true, true, false); 
      atualizarDisplay("PAUSADO", formatarTempo(contadorTimer), "Ciclos: " + String(contadorCiclos));
      Serial.println(F("[ESTADO] Sistema PAUSADO pelo usuario."));
    } else if (estadoAtual == PAUSADO) {
      Serial.println(F("[ESTADO] Despausando..."));
      estadoAtual = estadoAnterior; 
    }
    ultimoTempoDebounce = millis();
  }

  // Botão RESET
  if (digitalRead(BTN_RESET) == LOW) {
    Serial.println(F("[ENTRADA] Botao RESET pressionado."));
    resetarSistema();
    ultimoTempoDebounce = millis();
  }
}

void processarLogicaTimer() {
  if (estadoAtual == PAUSADO || estadoAtual == OCIOSO) return;

  unsigned long millisAtual = millis();
  
  if (millisAtual - millisAnterior >= 1000) {
    millisAnterior = millisAtual;
    contadorTimer--;

    // LOG DE CONTAGEM REGRESSIVA (Opcional - pode poluir o console, descomente se quiser ver os segundos)
    // Serial.print("[TIMER] "); Serial.println(contadorTimer);

    if (estadoAtual == TRABALHANDO) {
      definirCorLed(false, true, false); 
      atualizarDisplay("Foco Total!", formatarTempo(contadorTimer), "Ciclos: " + String(contadorCiclos));
      
      if (contadorTimer <= LIMITE_ALERTA) {
        Serial.println(F("[ALERTA] Entrando nos ultimos 5 segundos!"));
        estadoAtual = FINALIZANDO;
      }
    }
    else if (estadoAtual == FINALIZANDO) {
      ledPiscando = !ledPiscando;
      if (ledPiscando) {
        definirCorLed(true, true, false); 
        tone(PIN_BUZZER, 1000, 200);
      } else {
        definirCorLed(false, false, false);
      }
      atualizarDisplay("Prepare-se", "RELAXAR!", formatarTempo(contadorTimer));

      if (contadorTimer <= 0) {
        Serial.println(F("[SISTEMA] Tempo de foco esgotado."));
        iniciarSessaoDescanso();
      }
    }
    else if (estadoAtual == DESCANSO) {
      definirCorLed(false, true, true); 
      atualizarDisplay("Descanso!", formatarTempo(contadorTimer), "Ciclos: " + String(contadorCiclos));
      
      if (contadorTimer <= 0) {
        Serial.println(F("[SISTEMA] Tempo de descanso esgotado. Reiniciando Foco..."));
        iniciarSessaoTrabalho(); 
      }
    }
  }
}

// ==================== LOOP MAIN ====================

void setup() {
  Serial.begin(115200);

  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_PAUSE, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);

  executarChecagemSistema();

  if(!display.begin(SSD1306_SWITCHCAPVCC, ENDERECO_I2C)) { 
    Serial.println(F("[ERRO CRITICO] Falha no OLED SSD1306"));
    for(;;); 
  }
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  atualizarDisplay("SkillBridge", "Pronto?", "Ciclos: 0");
  
  definirCorLed(false, false, true); 
}

void loop() {
  gerenciarEntradas();
  processarLogicaTimer();
}