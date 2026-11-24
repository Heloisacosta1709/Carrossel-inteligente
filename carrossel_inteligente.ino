#include <Wire.h>  // Biblioteca para comunicação I2C
#include <Servo.h> // Biblioteca do servo motor

Servo motor;

// Definição dos pinos dos componentes
const byte ligar = 2;
const byte parar = 3;
const byte servo = 6;
const byte piezo = 8;
const byte trig = 9;
const byte echo = 10;

// Notas musicais da música Jingle Bells (Bate o Sino)
const int notas[] = {
  659, 659, 659,
  659, 659, 659,
  659, 784, 523, 587, 659,
  698, 698, 698, 698, 698,
  659, 659, 659, 659,
  587, 587, 659, 587, 784
};

// Tempo de duração de cada nota em milissegundos
const int tempos[] = {
  250, 250, 500,
  250, 250, 500,
  250, 250, 250, 250, 500,
  250, 250, 250, 250, 250,
  250, 250, 250, 250,
  250, 250, 250, 250, 500
};

// Variáveis de controle
bool ativo = false;       // Informa se o carrossel está ligado
bool perto = false;       // Informa se existe alguém próximo
bool ligarAntes = false;  // Verifica se o botão foi pressionado

byte nota = 0;            // Posição da nota musical
int tela = -1;            // Controla as mensagens do LCD

unsigned long medida = 0;
unsigned long proximo = 0;


// Envia os dados para o display LCD
void pulsoLCD(byte valor) {
  for (byte fase = 0; fase < 3; fase++) {
    Wire.beginTransmission(0x27);
    Wire.write(valor | 8 | (fase == 1 ? 4 : 0));
    Wire.endTransmission();
  }

  delayMicroseconds(50);
}


// Envia um comando ou caractere para o LCD
void lcd(byte valor, byte modo) {
  pulsoLCD((valor & 0xF0) | modo);
  pulsoLCD(((valor << 4) & 0xF0) | modo);
}


// Escreve uma mensagem de até 16 caracteres em uma linha
void linha(byte endereco, const char *texto) {
  lcd(endereco, 0);

  for (byte i = 0; i < 16; i++) {
    lcd(*texto ? *texto++ : ' ', 1);
  }
}


// Verifica se existe alguém a menos de 15 cm
bool alvoPerto() {

  // Envia um sinal pelo sensor ultrassônico
  digitalWrite(trig, LOW);
  delayMicroseconds(2);

  digitalWrite(trig, HIGH);
  delayMicroseconds(10);

  digitalWrite(trig, LOW);

  // Recebe o tempo de retorno do sinal
  unsigned long duracao = pulseIn(echo, HIGH, 20000UL);

  // Calcula a distância e verifica se está abaixo de 15 cm
  return duracao && duracao * 0.01715 < 15.0;
}


// Configuração inicial dos componentes
void setup() {

  // Configura os botões com resistor interno
  pinMode(ligar, INPUT_PULLUP);
  pinMode(parar, INPUT_PULLUP);

  // Configura os pinos do sensor
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);

  // Conecta o servo motor ao pino 6
  motor.attach(servo);

  // Mantém o servo parado inicialmente
  motor.writeMicroseconds(1500);

  // Inicia a comunicação com o LCD
  Wire.begin();
  delay(50);

  // Inicialização do display LCD
  pulsoLCD(0x30);
  delay(5);

  pulsoLCD(0x30);
  delay(1);

  pulsoLCD(0x30);
  pulsoLCD(0x20);

  lcd(0x28, 0);
  lcd(0x0C, 0);
  lcd(0x06, 0);
}


// Executa continuamente o funcionamento do projeto
void loop() {

  // Verifica se o botão de ligar foi pressionado
  bool apertou = digitalRead(ligar) == LOW;

  // Se o botão de parar for pressionado, desliga o carrossel
  if (digitalRead(parar) == LOW) {
    ativo = false;
  }

  // Liga o carrossel quando o botão é pressionado
  else if (apertou && !ligarAntes) {
    ativo = true;
  }

  // Guarda o estado anterior do botão
  ligarAntes = apertou;

  // Controla a velocidade do servo motor
  // 1650 = movimento
  // 1500 = parado
  motor.writeMicroseconds(ativo ? 1650 : 1500);

  // Obtém o tempo atual
  unsigned long agora = millis();

  // Se o carrossel estiver desligado, não considera aproximação
  if (!ativo) {
    perto = false;
  }

  // Verifica a distância a cada 70 milissegundos
  else if (agora - medida >= 70) {
    medida = agora;
    perto = alvoPerto();
  }

  // Define o estado atual do sistema
  // 0 = desligado
  // 1 = ligado e sem ninguém próximo
  // 2 = ligado e com alguém próximo
  int estado = !ativo ? 0 : perto ? 2 : 1;

  // Atualiza as mensagens do LCD quando o estado muda
  if (estado != tela) {

    // Interrompe qualquer som anterior
    noTone(piezo);

    nota = 0;
    proximo = agora;

    // Mostra as mensagens de acordo com o estado
    linha(0x80, !ativo ? "BEM-VINDO AO" :
          perto ? "ATENCAO!" : "CARROSSEL");

    linha(0xC0, !ativo ? "CARROSSEL!" :
          perto ? "AFASTE-SE" : "EM MOVIMENTO");

    tela = estado;
  }

  // Toca a música enquanto o carrossel estiver ligado
  if (ativo && (long)(agora - proximo) >= 0) {

    // Se alguém estiver muito próximo, toca o alerta
    if (perto) {

      tone(piezo, 1400, 120);
      proximo = agora + 300;

    }

    // Caso contrário, toca a música de Natal
    else {

      // Reproduz a nota musical atual
      tone(piezo, notas[nota], tempos[nota] - 25);

      // Define quando a próxima nota será tocada
      proximo = agora + tempos[nota];

      // Avança para a próxima nota
      nota = (nota + 1) % (sizeof(notas) / sizeof(notas[0]));
    }
  }
}
