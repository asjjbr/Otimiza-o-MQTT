
//#define MQTT_SOCKET_TIMEOUT 0

#include <SPI.h>
#include <UIPEthernet.h>
//#include <utility/logging.h>
#include <PubSubClient2.h>

#define BLINKLED 3
#define LIGHT 6 // O led que simula a lampada está na porta 6
#define STATUSLEDRED 7 // O led vermelho de status da conexão MQTT está no pino 7
#define STATUSLEDGREEN 9 // O led verde de status da conexão MQTT está no pino 9
#define STATUSLEDBLUE 8 // O led azul de status da conexão MQTT está no pino 8

#define OFF LOW // Mapeamento do texto OFF como nivel baixo
#define ON HIGH // Mapeamento do texto ON como nivel alto

#define NUM_PISCADAS 10

#define CONNECTED 0x00
#define DISCONNECTED 0x01


// Update these with values suitable for your network.
byte mac[] = {0xDE, 0xED, 0xBA, 0xFE, 0xF1, 0xff};

int lightStatus = OFF;
int botaoLuzStatus = LOW;

int mqttLuz = 0;

int mqttMachineState;

int blinkMQTTLed = LOW;
int contador = NUM_PISCADAS;
unsigned long tempo = NUM_PISCADAS;
unsigned long tempoinicial = 0;
int blinkMQTTLedStatus = LOW;

int blinkLed = LOW;
unsigned long t1 = 0;

int sendLuzStatus = 0;

// Callback function header
void callback(char *topic, byte *payload, unsigned int length);

EthernetClient ethClient;

// Dados do MQTT Cloud
PubSubClient2 client("m10.cloudmqtt.com", 16367, callback, ethClient);

// Funcçao que irá receber o retorno do servidor.
void callback(char *topic, byte *payload, unsigned int length)
{
  String sTopic = String(topic); 
  
  if(sTopic == "lampada"){
    mqttLuz = 1;
   }
  blinkMQTTLed = HIGH;
  tempoinicial = millis();
  contador += NUM_PISCADAS;
  //byte *p = (byte *)malloc(length);
  //memcpy(p, payload, length);
  //free(p);
}

void setup() {
  pinMode(LIGHT, OUTPUT);
  pinMode(STATUSLEDRED, OUTPUT);
  pinMode(STATUSLEDGREEN, OUTPUT);
  pinMode(STATUSLEDBLUE, OUTPUT);
  pinMode(BLINKLED, OUTPUT);

  Serial.begin(9600);
  //Serial.println("Iniciando...");
  Ethernet.begin(mac);

  digitalWrite(STATUSLEDBLUE, ON);
  if (client.connect("Magal", "coiktbwj", "zAhaklL2atGf"))
  {
    client.subscribe("lampada");
    client.subscribe("lampadastatus");
    
    client.publish("lampada","0");
    mqttMachineState = CONNECTED;
  }else{
    mqttMachineState = DISCONNECTED;
  }
  digitalWrite(STATUSLEDBLUE, OFF);
}

void loop() {

  client.loop();
  
  int botaoLuz = comandoLuz();//digitalRead(LIGHTBUTTON); // Verifica se existe um comando de acionamento da luz

  // Se houver comando de acionamento da luz, o estado dela é comutado (acende se estiver apagada, apaga se estiver acesa)
  if((botaoLuz != botaoLuzStatus)&&(botaoLuz)){
    comutaLuz();
  }
  botaoLuzStatus = botaoLuz;

  if (millis() - t1 > 500){
    blinkLed = !blinkLed;
    digitalWrite(BLINKLED, blinkLed);
    t1 = millis();
  }
  
  switch(mqttMachineState){
    case CONNECTED:
      acendeLed(STATUSLEDGREEN);
      apagaLed(STATUSLEDRED);
      if(!client.connected()){
        mqttMachineState = DISCONNECTED;
      }
      break;
    case DISCONNECTED:
      apagaLed(STATUSLEDGREEN);
      acendeLed(STATUSLEDRED);
      if (client.connect("Magal", "coiktbwj", "zAhaklL2atGf")){
        // Conecta no topic para receber mensagens
        client.subscribe("lampada");
        client.subscribe("lampadastatus");

        mqttMachineState = CONNECTED;
      }
      else{
        mqttMachineState = DISCONNECTED;
      }
      break;
  }

  if(blinkMQTTLed){
    if(contador<=0){
      blinkMQTTLed = LOW;
      contador = NUM_PISCADAS;
      apagaLed(STATUSLEDBLUE);
    }
    else{
      if(millis() - tempoinicial > tempo){
        if(blinkMQTTLedStatus){
          apagaLed(STATUSLEDBLUE);
          blinkMQTTLedStatus = LOW;
        }
        else{
          acendeLed(STATUSLEDBLUE);
          blinkMQTTLedStatus = HIGH;
        }
        tempoinicial = millis();
        contador--;
      }
    }
  }
  else
    apagaLed(STATUSLEDBLUE);

  if(sendLuzStatus){
    boolean flag;
    if(lightStatus)
      flag = client.publish("lampadastatus", "100", true);
    else
      flag = client.publish("lampadastatus", "0", true);
    if(flag){
      blinkMQTTLed = HIGH;
      tempoinicial = millis();
      contador += NUM_PISCADAS;
      sendLuzStatus = 0;
      
    }
  }
  
}

// A função checa qual o estado atual da luz da garagem e o alterna (se estiver desligado, liga. se estiver ligado, desliga)
void comutaLuz(){
  if(lightStatus){
    digitalWrite(LIGHT, OFF);
    lightStatus = OFF;
  }
  else{
    digitalWrite(LIGHT, ON);
    lightStatus = ON;
  }
  sendLuzStatus = 1;
  Serial.println("comutaLuz");
}

// A função acende a luz da garagem
void switchOnLight(){
  digitalWrite(LIGHT, ON);
  lightStatus = ON;
  sendLuzStatus = 1;
}

// A função apaga a luz da garagem
void switchOffLight(){
  digitalWrite(LIGHT, OFF);
  lightStatus = OFF;
  sendLuzStatus = 1;
}

void acendeLed(int pin){
  digitalWrite(pin, HIGH);
}
void apagaLed(int pin){
  digitalWrite(pin, LOW);
}

int comandoLuz(){
  int botao = 0;
  botao |= mqttLuz;
  mqttLuz = 0;
  
  return botao; 
}
