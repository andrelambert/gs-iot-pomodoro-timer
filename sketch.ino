/*
 * Projeto: SkillBridge - Timer Pomodoro IoT (Conectado)
 * Versão: 9.0 (Final + Timestamp Real NTP)
 * Descrição: Envia dados para o Firebase com Data e Hora (createdAt) corretos.
 * Hardware: ESP32, OLED, LED RGB, Buzzer, Botões.
 * Simulador: Wokwi
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> 
#include <time.h> // Biblioteca nativa para gerenciar hora

// ==================== CONFIGURAÇÕES FIREBASE (FIAP) ====================
const char* FIREBASE_PROJECT_ID = "fiap-mobile-8ca1d"; 
const char* FIREBASE_API_KEY    = "AIzaSyC63p4TMq26s3BNhE3lbPBzJSpFuFVXFAM";
const char* USER_ID_FICTICIO    = "estudante_iot_fiap_01"; 
const char* COLECAO_FIRESTORE   = "study_sessions";

// ==================== CONFIGURAÇÕES WIFI & HORA ====================
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";

// Configuração NTP (Hora da Internet)
// Usaremos UTC (0) para salvar no banco. O App React Native converte para o horário local do usuário.
const long  GMT_OFFSET_SEC = 0; 
const int   DAYLIGHT_OFFSET_SEC = 0;
const char* NTP_SERVER = "pool.ntp.org";

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

// --- Configuração de Tempo (Demonstração) ---
const long DURACAO_TRABALHO = 25; 
const long DURACAO_PAUSA    = 5;      
const int  LIMITE_ALERTA    = 5; 

// --- Estados ---
enum EstadoSistema { OCIOSO, TRABALHANDO, FINALIZANDO, DESCANSO, PAUSADO };
EstadoSistema estadoAtual = OCIOSO;
EstadoSistema estadoAnterior = OCIOSO; 

// --- Variáveis Globais ---
unsigned long millisAnterior = 0; 
long contadorTimer = DURACAO_TRABALHO;
int contadorCiclos = 0;
bool ledPiscando = false; 
unsigned long ultimoTempoDebounce = 0;
const unsigned long ATRASO_DEBOUNCE = 200;

// ==================== FUNÇÕES DE HARDWARE ====================

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

// ==================== CONECTIVIDADE E HTTP ====================

void conectarWiFi() {
  Serial.print(F("[WIFI] Conectando a "));
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("\n[WIFI] Conectado!"));
    
    // Inicia a sincronização de hora assim que conecta
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    Serial.println(F("[NTP] Sincronizando relogio com a internet..."));
  } else {
    Serial.println(F("\n[WIFI] Falha. Modo Offline."));
  }
}

/**
 * Envia dados para o Firestore via HTTP POST com TIMESTAMP (createdAt).
 */
void enviarDadosFirebase(int tempoFocoSegundos) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[HTTP] Erro: Sem Wi-Fi."));
    return;
  }

  String url = "https://firestore.googleapis.com/v1/projects/";
  url += FIREBASE_PROJECT_ID;
  url += "/databases/(default)/documents/";
  url += COLECAO_FIRESTORE;
  url += "?key=";
  url += FIREBASE_API_KEY;

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  // Construção do JSON
  StaticJsonDocument<768> doc; // Aumentei o buffer para caber a data
  JsonObject fields = doc.createNestedObject("fields");

  fields["userId"]["stringValue"] = USER_ID_FICTICIO;
  fields["sessionType"]["stringValue"] = "pomodoro_focus";
  fields["durationSeconds"]["integerValue"] = tempoFocoSegundos;
  fields["status"]["stringValue"] = "completed";
  fields["platform"]["stringValue"] = "ESP32_IoT";

  // --- OBTENDO DATA E HORA ATUAL (NTP) ---
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    // Se conseguiu pegar a hora, formata para ISO 8601 (ex: 2023-10-25T14:00:00Z)
    char timeStringBuff[30];
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    
    // 'timestampValue' é o tipo específico do Firestore para datas
    fields["createdAt"]["timestampValue"] = String(timeStringBuff);
    Serial.print(F("[DATA] Timestamp gerado: "));
    Serial.println(timeStringBuff);
  } else {
    Serial.println(F("[ERRO] Falha no NTP. Usando data padrao."));
    fields["createdAt"]["timestampValue"] = "2024-01-01T12:00:00Z";
  }

  String jsonOutput;
  serializeJson(doc, jsonOutput);

  Serial.println(F("[HTTP] Enviando..."));
  int httpResponseCode = http.POST(jsonOutput);

  if (httpResponseCode == 200) {
    Serial.print(F("[HTTP] SUCESSO! Codigo: 200."));
  } else {
    Serial.print(F("[HTTP] ERRO. Codigo: "));
    Serial.println(httpResponseCode);
    Serial.println(http.getString()); 
  }

  http.end();
}

// ==================== LÓGICA PRINCIPAL ====================

void executarChecagemSistema() {
  Serial.println(F("\n[SISTEMA] Iniciando SkillBridge IoT..."));
  definirCorLed(true, false, false); delay(150);
  definirCorLed(false, true, false); delay(150);
  definirCorLed(false, false, true); delay(150);
  tone(PIN_BUZZER, 1000, 100);
  definirCorLed(false, false, false);
  Serial.println(F("[SISTEMA] Hardware OK."));
}

void iniciarSessaoTrabalho() {
  Serial.println(F("[ESTADO] Foco Iniciado"));
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
  Serial.println(F("----------------------------------------"));
  Serial.println(F("[ESTADO] Ciclo Finalizado"));
  Serial.print(F("[DADOS] Ciclos: ")); Serial.println(contadorCiclos);
  
  atualizarDisplay("Salvando...", "Nuvem", "WiFi");
  // Envia para o Firebase com timestamp
  enviarDadosFirebase(DURACAO_TRABALHO); 

  Serial.println(F("----------------------------------------"));

  estadoAtual = DESCANSO;
  contadorTimer = DURACAO_PAUSA;
  noTone(PIN_BUZZER);
}

void resetarSistema() {
  estadoAtual = OCIOSO;
  contadorTimer = DURACAO_TRABALHO;
  contadorCiclos = 0;
  noTone(PIN_BUZZER);
  definirCorLed(false, false, true); 
  atualizarDisplay("SkillBridge", "Pronto?", "Ciclos: 0");
  Serial.println(F("[SISTEMA] Reset."));
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

  if (digitalRead(BTN_START) == LOW) {
    if (estadoAtual == OCIOSO) iniciarSessaoTrabalho();
    else if (estadoAtual == PAUSADO) {
      estadoAtual = estadoAnterior;
    }
    ultimoTempoDebounce = millis();
  }

  if (digitalRead(BTN_PAUSE) == LOW) {
    if (estadoAtual == TRABALHANDO || estadoAtual == FINALIZANDO || estadoAtual == DESCANSO) {
      estadoAnterior = estadoAtual;
      estadoAtual = PAUSADO;
      definirCorLed(true, true, false); 
      atualizarDisplay("PAUSADO", formatarTempo(contadorTimer), "Ciclos: " + String(contadorCiclos));
    } else if (estadoAtual == PAUSADO) {
      estadoAtual = estadoAnterior;
    }
    ultimoTempoDebounce = millis();
  }

  if (digitalRead(BTN_RESET) == LOW) {
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

    if (estadoAtual == TRABALHANDO) {
      definirCorLed(false, true, false); 
      atualizarDisplay("Foco Total!", formatarTempo(contadorTimer), "Ciclos: " + String(contadorCiclos));
      
      if (contadorTimer <= LIMITE_ALERTA) {
        estadoAtual = FINALIZANDO;
      }
    }
    else if (estadoAtual == FINALIZANDO) {
      ledPiscando = !ledPiscando;
      if (ledPiscando) {
        definirCorLed(true, true, false); tone(PIN_BUZZER, 1000, 200);
      } else {
        definirCorLed(false, false, false);
      }
      atualizarDisplay("Prepare-se", "RELAXAR!", formatarTempo(contadorTimer));

      if (contadorTimer <= 0) {
        iniciarSessaoDescanso();
      }
    }
    else if (estadoAtual == DESCANSO) {
      definirCorLed(false, true, true); 
      atualizarDisplay("Descanso!", formatarTempo(contadorTimer), "Ciclos: " + String(contadorCiclos));
      
      if (contadorTimer <= 0) {
        iniciarSessaoTrabalho(); 
      }
    }
  }
}

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
  conectarWiFi();

  if(!display.begin(SSD1306_SWITCHCAPVCC, ENDERECO_I2C)) { 
    Serial.println(F("[ERRO] Falha no OLED")); for(;;); 
  }
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  resetarSistema();
}

void loop() {
  gerenciarEntradas();
  processarLogicaTimer();
}