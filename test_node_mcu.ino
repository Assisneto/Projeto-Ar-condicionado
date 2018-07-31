//Sistema de Automação produzido por: José Maria Junior, Assis Neto, Renilson e Diego.

//Pino do NodeMCU que estara conectado ao rele
const int pinRele = 5; //Equivalente a D1 no NodeMCU
const int pinSensor = 4; //Equivalente a D2 no NodeMCU
int sensor = 0;
int value;
int calibracao = 10;
unsigned long tempo;

/*
Equivalencia das saidas Digitais entre nodeMCU e ESP8266 (na IDE do Arduino)
NodeMCU � ESP8266
D0 = 16;
D1 = 5;
D2 = 4;
D3 = 0;
D4 = 2;
D5 = 14;
D6 = 12;
D7 = 13;
D8 = 15;
D9 = 3;
D10 = 1;
*/

#include <ESP8266WiFi.h>
 
const char* ssid = "ZE MARIA";
const char* password = "09121944";


//IP do ESP
IPAddress ip(192, 168, 0, 113);
//IP do roteador da sua rede wifi
IPAddress gateway(192, 168, 0, 1);
//Mascara de rede da sua rede wifi
IPAddress subnet(255, 255, 255, 0); 

WiFiServer server(80);


void setup() {

//------------------------------------------------------------------------------------------
  Serial.begin(9600);

  
  // Declaração de que o relé será usado como saída.
  pinMode(pinRele, OUTPUT);

  // Declaração de que o sensor será usado como entrada.
  pinMode(pinSensor,INPUT); 


  Serial.print("Calibrando o sensor...");
  for(int i = 0; i < calibracao; i++){
  Serial.print(".");
  }
  Serial.println("   Sensor Ativado!   ");
//------------------------------------------------------------------------------------------

    //IP do ESP
  IPAddress ip(192, 168, 0, 113);
  //IP do roteador da sua rede wifi
  IPAddress gateway(192, 168, 0, 1);
  //Mascara de rede da sua rede wifi
  IPAddress subnet(255, 255, 255, 0);
  
  //Conectando a rede Wifi
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);

  }
 
  // Start the server
  server.begin();
 
 
}
 
void loop() {


// SE NÃO HAVER REQUISIÇÃO, EXECUTA ISSO !

  

  sensor = digitalRead(pinSensor); // Leitura do sinal recebido pelo Sensor de Presença PIR

  Serial.print("Valor do Sensor PIR: ");
  Serial.print(sensor);

  Serial.print("   Valor variavel TEMPO: ");
  Serial.println(tempo);

//-------------------------(If Sensor)----------------------------- 

  //Liga
  if(sensor == 1){ // Com presença no local.
    
    digitalWrite(pinRele, LOW); // Ligamento do aparelho.
    tempo = 0; // Zeramento da variável tempo para não cair na condição que aciona o desligamento.    
  }
  
  //Desliga
  if(sensor == 0){ // Sem presença no local.
    
  tempo++; // Incrementação da variável tempo a cada segundo. 

    if (tempo == 20){ // Tempo de contagem para que o aparelho possa ser definitivamente desligado.(5min)
    
    digitalWrite(pinRele, HIGH); // Desligamento do aparelho.
    delay(30000); // Tempo de espera para poder ligar novamente caso haja presença, Ar condicionado recém desligado precisa de um tempo para ligar novamente à fim de evitar danos.(30seg)
 
    }   
  
  }

//------------------------------------------------------------------------------------------
  delay(1000); // Detecção a cada 1 segundo.
//------------------------------------------------------------------------------------------


// Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
 
  // Wait until the client sends some data
  while(!client.available()){

  int count=0;
  count++;
  if(count==1){
    
    count=0;  
  }
    //delay(500);
    
  }  

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  
  client.flush();



// SE HAVER REQUISIÇÃO, EXECUTA ISSO !



//----------------------(If Web)-----------------------------------

  
  // Liga
  if (request.indexOf("/AR=ON") != -1)  {
  digitalWrite(pinRele, LOW);
  value = LOW;
  }

  // Desliga
  if (request.indexOf("/AR=OFF") != -1)  {
  digitalWrite(pinRele, HIGH);
  value = HIGH;
  }

//------------------------------------------------------------------------------------------
  delay(500);
//------------------------------------------------------------------------------------------

 

  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<br><br><br>");
 
  client.println("<center><h1><b><font size='40'>Sala </font><font color='BLUE' size='40'>A211</font></b></h1></center>");
 
  if(value == LOW) {
    client.print("<center><b><font size='40'>Ar - Condicionado: (<font color='GREEN' size='40'>On</font>)</font></b></center>");
  } else {
    client.print("<center><b><font size='40'>Ar - Condicionado: ( <font color='RED' size='40'>Off</font> ) </font></b></center>");
  }
  client.println("<br><br>");
  client.println("<center><a href=\"/AR=ON\"\"><button><b><font size='25'>&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp LIGA &nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp&nbsp</font></b></button></a></center>");
  client.println("<br>");
  client.println("<center><a href=\"/AR=OFF\"\"><button><b><font size='25'>&nbsp&nbsp&nbsp&nbsp DESLIGA &nbsp&nbsp&nbsp&nbsp</font></b></button></a></center>");  
  client.println("</html>");

  client.flush();
  
}

/*  
    sensor1 = digitalRead(pinoSensor); // Leitura do sinal recebido pelo Sensor de Presença PIR 1
    sensor2 = digitalRead(pinoSensor); // Leitura do sinal recebido pelo Sensor de Presença PIR 2
    sensor3 = digitalRead(pinoSensor); // Leitura do sinal recebido pelo Sensor de Presença PIR 3
    sensor4 = digitalRead(pinoSensor); // Leitura do sinal recebido pelo Sensor de Presença PIR 4


    //Declaração da variável que irá receber os valores de todos os sensores.
    int sensores;
    //Recebe os valores detectados dos 4 sensores e atribui a uma única variável.
    sensores = sensor1 + sensor2 + sensor3 + sensor 4. 

  if (sensores == 0){ // Sem presença no local.
    
    tempo++; // Incrementação da variável tempo a cada segundo.
    } else{
    
      digitalWrite(rele, LOW); // Ligamento do aparelho.
      tempo = 0; // Zeramento da variável tempo para não cair na condição que aciona o desligamento.
      }
      
  if (tempo == 300){ // Tempo de contagem para que o aparelho possa ser definitivamente desligado.(5min)
    
    digitalWrite(rele, HIGH); // Desligamento do aparelho.
    delay(30000); // Tempo de espera para poder ligar novamente caso haja presença, Ar condicionado recém desligado precisa de um tempo para ligar novamente à fim de evitar danos.(30seg)
 
    }

    delay(1000); // Detecção a cada segundo.

}
*/

