#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_AHTX0.h>
#include <Wire.h>

// Configurações do display OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Reset não utilizado no I2C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Objeto do sensor AHT10 (temperatura e umidade)
Adafruit_AHTX0 aht;

// Definição de pinos
const int MQ135_PIN = 34;      // Pino analógico conectado ao MQ135
const int RED_PIN   = 25;      // Pino digital para LED vermelho (RGB)
const int GREEN_PIN = 26;      // Pino digital para LED verde (RGB)
const int BLUE_PIN  = 27;      // Pino digital para LED azul (RGB)

// Credenciais WiFi (substitua pelos dados da sua rede)
const char* ssid     = "Login";
const char* password = "Senha";

// Servidor web na porta 80
WebServer server(80);

void setup() {
  Serial.begin(115200);
  
  // Inicializa I2C para OLED e AHT10 (pinos I2C padrão do ESP32: SDA 21, SCL 22)
  Wire.begin(21, 22);
  
  // Inicializa o display OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C é o endereço I2C do OLED
    Serial.println("Falha ao inicializar o display OLED!");
    // Se falhar, podemos travar ou seguir sem exibir no OLED
  } else {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.display();
  }
  
  // Inicializa o sensor AHT10
  if (!aht.begin()) {
    Serial.println("Sensor AHT10 não encontrado, verifique as conexões.");
  }
  
  // Configura os pinos dos LEDs como saída
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  // Inicia com todos os LEDs apagados (cátodo comum: LOW = desligado, HIGH = ligado)
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
  
  // Conecta ao WiFi (para disponibilizar a API)
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  int maxAttempts = 20;
  // Aguarda alguns segundos para conectar
  while (WiFi.status() != WL_CONNECTED && maxAttempts-- > 0) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi conectado! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Não foi possível conectar ao WiFi. Continuando sem servidor web.");
  }
  
  // Define as rotas (endpoints) da API HTTP
  server.on("/mq135", []() {
    int value = analogRead(MQ135_PIN);
    String json = "{\"mq135\":" + String(value) + "}";
    server.send(200, "application/json", json);
  });
  
  server.on("/aht10", []() {
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    float t = temp.temperature;
    float h = humidity.relative_humidity;
    String json = "{\"temperature\":" + String(t) + ",\"humidity\":" + String(h) + "}";
    server.send(200, "application/json", json);
  });
  
  server.on("/red/on", []() {
    digitalWrite(RED_PIN, HIGH); // liga LED vermelho
    server.send(200, "application/json", "{\"red\":\"on\"}");
  });
  server.on("/red/off", []() {
    digitalWrite(RED_PIN, LOW);  // desliga LED vermelho
    server.send(200, "application/json", "{\"red\":\"off\"}");
  });
  
  server.on("/green/on", []() {
    digitalWrite(GREEN_PIN, HIGH); // liga LED verde
    server.send(200, "application/json", "{\"green\":\"on\"}");
  });
  server.on("/green/off", []() {
    digitalWrite(GREEN_PIN, LOW);  // desliga LED verde
    server.send(200, "application/json", "{\"green\":\"off\"}");
  });
  
  server.on("/blue/on", []() {
    digitalWrite(BLUE_PIN, HIGH); // liga LED azul
    server.send(200, "application/json", "{\"blue\":\"on\"}");
  });
  server.on("/blue/off", []() {
    digitalWrite(BLUE_PIN, LOW);  // desliga LED azul
    server.send(200, "application/json", "{\"blue\":\"off\"}");
  });
  
  // Inicia o servidor web se o WiFi estiver conectado
  if (WiFi.status() == WL_CONNECTED) {
    server.begin();
    Serial.println("Servidor web iniciado.");
  }
}

void loop() {
  // Atualiza continuamente o display OLED com os valores dos sensores
  static unsigned long lastDisplayUpdate = 0;
  const unsigned long displayInterval = 500; // intervalo de 500 ms (0,5 s) entre atualizações
  if (millis() - lastDisplayUpdate >= displayInterval) {
    lastDisplayUpdate = millis();
    // Lê os sensores
    int airValue = analogRead(MQ135_PIN);
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    float temperature = temp.temperature;
    float humidityVal = humidity.relative_humidity;
    // Exibe as leituras no OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Co2: ");
    display.print(airValue);
    display.setCursor(0,20);
    display.print("T: ");
    display.print(temperature, 2);
    display.print(" C");
    display.setCursor(0, 40);
    display.print("U: ");
    display.print(humidityVal, 2);
    display.print(" %");
    display.display();
  }
  
  // Atende requisições HTTP recebidas (se conectado ao WiFi)
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }
  
  // Pequena pausa para não sobrecarregar a CPU
  delay(1);
}
