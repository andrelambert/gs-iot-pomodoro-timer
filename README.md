# SkillBridge IoT — Smart Pomodoro Timer

> Dispositivo IoT integrado à plataforma SkillBridge para gestão de tempo e foco (Pomodoro), com sincronização de dados em nuvem via HTTP/Firestore.

---

## 👥 Equipe

Este projeto foi desenvolvido por:

- **André Lambert** - RM 99148
- **Felipe Cortez** - RM 99750
- **Guilherme Morais** - RM 551981

---

## 🎥 Demonstrações e Links

| Recurso | Link |
| :--- | :--- |
| **Simulação Online (Wokwi)** | [Acessar Projeto no Wokwi](https://wokwi.com/projects/448238750741426177) |
| **Vídeo: Plataforma Mobile** | [Assistir no YouTube](https://youtu.be/Np-I8Djucbk) |
| **Vídeo: Solução IoT** | [INSIRA_SEU_LINK_DO_YOUTUBE_AQUI] |

---

## 🎯 Visão Geral e Problema

A plataforma **SkillBridge** (App Mobile) oferece trilhas de aprendizado para *upskilling* e *reskilling*. No entanto, um dos maiores desafios para quem estuda online é a **gestão do tempo e a manutenção do foco**.

A solução **SkillBridge IoT** é um dispositivo físico de mesa que auxilia o estudante a aplicar a técnica **Pomodoro** (ciclos de foco e pausa) longe das distrações do celular.

**O diferencial:** O dispositivo não é isolado. Ao finalizar um ciclo de estudo com sucesso, ele conecta-se via Wi-Fi e envia os dados da sessão (tempo, data e status) diretamente para o banco de dados da SkillBridge (Firebase), permitindo que o app mobile gere estatísticas de produtividade para o aluno.

---

## 🔌 Arquitetura de Hardware

O projeto foi desenvolvido utilizando o simulador Wokwi com base na arquitetura ESP32.

### **Lista de Componentes**

* **ESP32 DevKit V1:** Microcontrolador principal com Wi-Fi integrado.
* **Display OLED SSD1306 (I2C):** Exibe o timer, mensagens motivacionais e status da conexão.
* **LED RGB:** Feedback visual do estado atual (Foco, Pausa, Alerta, Erro).
* **Buzzer:** Feedback sonoro para fim de ciclos e alertas.
* **Pushbuttons (x3):**
    * `Start`: Inicia o ciclo.
    * `Pause`: Pausa/Retoma o timer.
    * `Reset`: Reinicia o sistema para o estado ocioso.

### **Diagrama de Conexões**

| Componente | Pino ESP32 | Observação |
| :--- | :--- | :--- |
| **OLED SDA** | GPIO 21 | Protocolo I2C |
| **OLED SCL** | GPIO 22 | Protocolo I2C |
| **LED Vermelho** | GPIO 13 | Canal R |
| **LED Verde** | GPIO 12 | Canal G |
| **LED Azul** | GPIO 14 | Canal B |
| **Buzzer** | GPIO 25 | Saída PWM/Tone |
| **Btn Start** | GPIO 26 | Input Pull-up |
| **Btn Pause** | GPIO 27 | Input Pull-up |
| **Btn Reset** | GPIO 33 | Input Pull-up |

![Imagem do Circuito](https://raw.githubusercontent.com/andrelambert/gs-iot-pomodoro-timer/refs/heads/main/Circuit.png)

---

## ☁️ Comunicação e Integração (HTTP & Firebase)

Para cumprir o requisito de troca de dados, o dispositivo atua como um cliente HTTP REST. Não utilizamos bibliotecas pesadas do Firebase Client, mas sim chamadas diretas à **REST API do Firestore**.

### **Fluxo de Dados**

1.  **Sincronização de Hora (NTP):** Ao ligar, o ESP32 conecta ao Wi-Fi e busca a hora exata no servidor `pool.ntp.org` para gerar o *timestamp* correto (`createdAt`).
2.  **Ciclo de Estudo:** O usuário completa o timer de foco + timer de descanso.
3.  **Envio (HTTP POST):** O dispositivo monta um payload JSON e envia para o endpoint do Firestore.

### **Detalhes do Endpoint**

* **Protocolo:** HTTP / 1.1
* **Método:** `POST`
* **Host:** `firestore.googleapis.com`
* **Rota:** `/v1/projects/fiap-mobile-8ca1d/databases/(default)/documents/study_sessions`
* **Autenticação:** Via Query Param (`?key=API_KEY`) e Regras de Segurança do Firestore configuradas para aceitar criações públicas na coleção `study_sessions`.

### **Estrutura do JSON Enviado**

Exemplo do payload gerado pelo ESP32 usando a biblioteca `ArduinoJson`:

```json
{
  "fields": {
    "userId": { "stringValue": "estudante_iot_fiap_01" },
    "sessionType": { "stringValue": "pomodoro_cycle_complete" },
    "durationSeconds": { "integerValue": 25 },
    "status": { "stringValue": "completed_with_break" },
    "platform": { "stringValue": "ESP32_IoT" },
    "createdAt": { "timestampValue": "2025-10-25T14:30:00Z" }
  }
}

## 💻 Lógica do Sistema (Máquina de Estados)

O firmware foi desenvolvido em C++ e opera baseado em uma máquina de estados finita (FSM) para garantir estabilidade:

1.  **OCIOSO (Azul):** Aguarda o usuário apertar Start.
2.  **TRABALHANDO (Verde):** Contagem regressiva do tempo de foco.
3.  **FINALIZANDO (Laranja + Buzzer):** Últimos 5 segundos de atenção.
4.  **DESCANSO (Ciano):** Contagem do tempo de pausa.
5.  **PAUSADO (Amarelo):** Congela o tempo até nova ação.

*Nota: O envio para a nuvem ocorre automaticamente na transição do fim do DESCANSO para o novo ciclo.*

---

## 🛠️ Como Rodar a Simulação

1.  Acesse o link do projeto no **Wokwi**: [https://wokwi.com/projects/448238750741426177](https://wokwi.com/projects/448238750741426177)
2.  Certifique-se de que as seguintes bibliotecas estão instaladas na aba **Library Manager**:
    * `Adafruit GFX Library`
    * `Adafruit SSD1306`
    * `ArduinoJson` (Essencial para o formato de dados do Firebase)
3.  Clique no botão **Play** (verde).
4.  **Aguarde a conexão:** O Console Serial mostrará a conexão Wi-Fi e a sincronização NTP.
5.  **Interaja:**
    * Aperte o botão **Verde (Start)** para iniciar.
    * Aguarde o ciclo terminar (configurado para 25s de demonstração).
    * Observe o LED mudar de cor e o Buzzer tocar.
    * No final do descanso, observe no **Serial Monitor** a mensagem `[HTTP] SUCESSO!`.

---

## 📄 Dependências

O projeto depende das seguintes bibliotecas C++ no ambiente Arduino/PlatformIO:

* **WiFi.h**: Conexão de rede.
* **HTTPClient.h**: Requisições REST para o Firebase.
* **ArduinoJson.h**: Serialização dos dados para formato JSON.
* **Adafruit_SSD1306.h**: Controle do display OLED.
* **time.h**: Sincronização de relógio (NTP) para timestamps reais.

---

**Global Solution - FIAP 2025**