//Sistema de Automação produzido por: José Maria Junior, Assis Neto, Diego Rodrigues e Renilson.

/*
Equivalencia das saidas Digitais entre nodeMCU e ESP8266 (na IDE do Arduino)
NodeMCU ESP8266
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

const unsigned long int pinRele = 5; //Equivalente a D1 no NodeMCU.
const unsigned long int pinSensor = 4; //Equivalente a D2 no NodeMCU.
unsigned long int sensor = 0; //Variável que receberá o valor lido pelo sensor. 
unsigned long int estadoRele = 0; //Variável que auxiliará na mudança de estado On/Off na página Web.


#include <user_interface.h> //Biblioteca necessaria para acessar os Timer`s.
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>

os_timer_t tmr0;//Cria o Timer.

unsigned long int tempo; //Variável de controle "Timer".
unsigned long int *z; //Variável auxiliar de controle do "Timer".

unsigned long int timerEstado;
unsigned long int timerPresenca;
unsigned long int timerReconnect;

const char* ssid = "ZE MARIA"; // Nome da Rede Wifi.
const char* password = "09121944"; // Senha da Rede Wifi.

//IP do ESP.
IPAddress ip(192, 168, 0, 105);
//IP do roteador da sua rede wifi.
IPAddress gateway(192, 168, 0, 1);
//Mascara de rede da sua rede wifi.
IPAddress subnet(255, 255, 255, 0);

// Iniciando os sevidores (Web Service, Cliente Wifi, Protocolo MQTT).
ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);


void setup() {

  Serial.begin(115200); // Iniciando o monitor serial.

  os_timer_setfn(&tmr0, contagem, NULL); //Indica ao Timer qual sera sua Sub rotina.
  os_timer_arm(&tmr0, 1000, true); // Contagem a cada 1 segundo.

  // Declaração de que o relé será usado como saída.
  pinMode(pinRele, OUTPUT);

  // Declaração de que o sensor será usado como entrada.
  pinMode(pinSensor,INPUT);

  estadoRele = LOW;

  // Conectando a rede WiFi.
  WiFi.config(ip, gateway, subnet);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Caso haja perca de conexão, conectar-se novamente.
  if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, password); 
  }

  // Iniciando conexão com o servidor em nuvem.
  client.setServer("m10.cloudmqtt.com", 12146);
  client.setCallback(mqtt_callback);

  /*
   * Rotas do servidor.
   */
  server.on("/", HTTP_GET, homeHandler);        //Rota inicial, primeira página a ser acessada pelo usuário
  server.on("/auth", HTTP_GET, authHandler);    //Rota que realiza e controla a autenticação do usuário
  server.on("/login", HTTP_POST, authHandler);  //Rota que processa os dados de login do usuário
  server.on("/ar", HTTP_GET, arHandler);        //Rota que faz o controle do ar-condicionado
  server.onNotFound(notFoundHandler);           //Função para controlar o que acontece quando é acessado uma página inexistente

  /*
   * Configurações do servidor necessárias para fazer funcionar a autenticação.
   */
  const char * headerkeys[] = {"Cookie"};
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize);

  // Iniciando o servidor.
  server.begin();
  
}

void loop() {

  if (!client.connected()) {
    if (timerReconnect > 2) {
      // Attempt to reconnect
      Serial.println("[MQTT]Tentando conectar ao broker...");
      if (reconnect()) {
        Serial.println("[MQTT]Conectado");
        timerReconnect = 0;
      }
    }
  } else { 
    // Cliente conectado.
    client.loop();
  }
  
  server.handleClient(); //Fica aguardando uma requisição.

  sensor = digitalRead(pinSensor); // Leitura do sinal recebido pelo Sensor de Presença.

  //-------------------------(If Sensor)------------------------------- 

  //Liga.
  if(sensor == 1){ // Com presença no local.

    if(timerPresenca > 1) {
        client.publish("nodemcu/ar/presence", "1"); // Publicação a cada 2 segundos.
        timerPresenca = 0;
    }
      if(tempo < 300 || tempo > 390){ // Entrará em funcionamento somente se o tempo for menor que 5:00 minutos ou maior que 6:30 minutos.
        digitalWrite(pinRele, HIGH); // Ligamento do aparelho.
        estadoRele = HIGH; // Variável auxiliar para mudar o estado On/Off na página Web.
        tempo = 0; // Zeramento da variável tempo para não cair na condição que aciona o desligamento e "pausa" o sistema por 1:30 minutos.
        if(timerEstado > 1) {
          client.publish("nodemcu/ar/state", "ligado"); // Publicação a cada 2 segundos.
          Serial.println("Sensor = 1; ligou");
          timerEstado = 0;
        }
      }
  }

  //Desliga.
  if(sensor == 0){ // Sem presença no local.

    if(timerPresenca > 0) {
        client.publish("nodemcu/ar/presence", "0"); // Publicação a cada 2 segundos.
        timerPresenca = 0;
    }
    
    // Quando chegar aos 5 minutos sem presença, desligar o aparelho e impossibilitar o sensor de interferir no sistema por 1:30 minutos.        
    if(tempo > 299 && tempo < 303){ 
      digitalWrite(pinRele, LOW); // Desligamento do aparelho.
      estadoRele = LOW; // Variável auxiliar para mudar o estado On/Off na página Web.
      if(timerEstado > 1) {
        client.publish("nodemcu/ar/state", "desligou"); // Publicação a cada 2 segundos.
        Serial.println("Sensor = 0; tempo esgotado, desligou");
        timerEstado = 0;
      }
    }
    
  }

}

/*
 * Sub rotina ISR do Timer0 sera acessada a cada 1 Segundo.
 */
void contagem(void*z)
{

  // Caso a variável chegue ao seu tamanho máximo, o valor será 391 novamente.
  if(tempo == 4294967290){
    
    tempo = 391; // Valor escolhido para que não caia na condição de "intervalo" entre tempo 5:00 minutos e 6:30 minutos.
  }
  // Caso a variável chegue ao seu tamanho máximo, o valor será 3 novamente.
  if(timerReconnect == 4294967290){

    timerReconnect = 3; // Valor definido para que mesmo que não consiga se reconectar, não saia da condição de tentativa.
  }
  // Caso a variável chegue ao seu tamanho máximo, o valor será 3 novamente.
  if(timerEstado == 4294967290){

    timerEstado = 3;
  }
  // Caso a variável chegue ao seu tamanho máximo, o valor será 3 novamente.
  if(timerPresenca == 4294967290){
    
    timerPresenca = 3;
  }
 
  tempo++;
  timerEstado++;
  timerPresenca++;
  timerReconnect++;

}

/*
 * isAuth
 * Verifica se o usuário está logado, se estiver
 * retorna true, se não retorna false
 */
bool isAuth(){
  Serial.println("[isAuth] - Entrou");
  if (server.hasHeader("Cookie")) { //Verifica a existência de cookies
    Serial.println("[isAuth] - Tem o header Cookie");
    String cookie = server.header("Cookie");
    if (cookie.indexOf("ESPSESSIONID=1") != -1) { //Verifica se existe o cookie de usuário autenticado
      Serial.println("[isAuth] - Tem o cookie de sessao");
      return true;
    }
  }
  Serial.println("[isAuth] - Nao tem sessao");
  return false;
}

/*
 * homeHandler
 * Função que gerencia a rota '/'
 */
void homeHandler(){
  String msg = "";
  Serial.println("[homeHandler] - Entrou");
  if (!isAuth()){ //verifica se o usuário está logado, se não estiver redireciona para o login
    Serial.println("[homeHandler] - Nao ta logado");
    server.sendHeader("Location", "/auth");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }


  //Se estiver logado, é exibida a página de gerenciamento do ar condicionado
  Serial.println("[homeHandler] - Exibe o conteudo da pagina");
  String content = "<!DOCTYPE html><html lang='pt-br'><head><meta charset='UTF-8'><meta http-equiv='refresh' content='1'><title>Controle e Monitoramento</title>";
  content += "<style type='text/css'>*{margin: 0;padding: 0;}body{font-family: Arial;background-color: #333;color: #fff;}#wrap{width: 960px;margin: 0 auto;text-align: center;font-size: 20px;font-weight: bold;}p#title{font-size: 25px;font-weight: bold;margin-top: 10%;margin-bottom: 2.5%;}#wrap table{margin: 0 auto;text-align: left;border-spacing: 0;border: none;font-weight: normal;}#wrap td{border-bottom: 1px solid #7f8c8d;padding: 10px;}#wrap tr:hover td{border-color: #fff;}tr td:first-child{font-weight: bold;}td button{font-size: 16px;padding: 6px;border: none;border-radius: 4px;color: #fff;cursor: pointer;outline: none;}button#on{background-color: #1BC295;}button#on:hover{background-color: #169C77;}button#off{background-color: #FF3A37;}button#off:hover{background-color: #CF312F;}#messages{width: fit-content;margin: 0 auto;margin-bottom: 2.5%;border: 1px solid #FF3A37;padding: 7px;font-weight: normal;border-radius: 3px;text-align: center;color: #FF3A37;}#logout{width: 150px;margin-top: 15px;font-size: 16px;padding: 6px;border: none;background-color: #e67e22;color: #fff;border-radius: 50px;cursor: pointer;}#logout:hover{background-color: #ED5E00;}</style>";
  content += "</head><body><div id='wrap'><p id='title'>Sistema Web de Controle e Monitoramento</p>";
  content += (msg != "") ? "<div id='messages'>"+msg+"</div>" : "";
  content += "<table><tr><td>Sala: </td><td colspan='2'>C - 104</td></tr><tr><td>Estado: </td>";
  content += (estadoRele == HIGH) ? "<td colspan='2' style='color: #1BC295'>On</td>" : "<td colspan='2' style='color: #FF3A37'>Off</td>";
  content += "</tr><tr><td>Ações: </td><td><a href='/ar?state=on'><button id='on'>Ligar</button></a></td><td><a href='/ar?state=off'><button id='off'>Desligar</button></a></td></tr><tr><td>Presença: </td>";
  content += (sensor == 1) ? "<td colspan='2'>Sim</td>" : "<td colspan='2'>Não</td>";
  content += "</tr></table><a href='/auth?logout=true'><button id='logout'>Sair</button></a></div></body></html>";
  server.send(200, "text/html", content);
}

/*
 * authHandler
 * Função responsavel por gerenciar toda a autenticação do sistema.
 */
void authHandler(){
  Serial.println("[authHandler] - Entrou");
  String msg = "";
  //Faz o logout do usuário.
  if(server.hasArg("logout")){ //Verifica se existe o argumento para logout
    Serial.println("[authHandler] - Logout");
    server.sendHeader("Location", "/auth");
    server.sendHeader("Cache-Control", "no-cache");
    server.sendHeader("Set-Cookie", "ESPSESSIONID=0");
    server.send(301);
    return;
  }

  //Realiza o login do usuário.
  if(server.hasArg("username") && server.hasArg("password")){ //Verifica se tem dados de usuário e senha
    Serial.println("[authHandler] - Tem usuario e senha");
    if(server.arg("username") == "admin" && server.arg("password") == "admin") { //Verifica se são os dados corretos
      Serial.println("[authHandler] - Usuario e senha conferem");
      server.sendHeader("Location", "/");
      server.sendHeader("Cache-Control", "no-cache");
      server.sendHeader("Set-Cookie", "ESPSESSIONID=1");
      server.send(301);
      return;
    }
    Serial.println("[authHandler] - Usuario e senha incorreto");
    msg = "Usuário ou senha incorretos";
  }

  //Caso os dados estejam incorretos a pagina de login é exibida novamente.
  //Com uma mensagem para o usuário.
  Serial.println("[authHandler] - Exibindo pagina de login");
  String content = "<!DOCTYPE html><html lang='pt-br'><head><meta charset='UTF-8'><title>Sistema - Login</title>";
  content += "<style type='text/css'>*{margin: 0;padding: 0;}html, body {height: 100%;min-height: 100%;font-family: Arial;background-color: #333;}.label{width: 90%;display: block;margin: 0 auto;margin-left: 10px;color: #fff;}#wrap{height: 100%;min-height: 100%;display: -webkit-flex;display: flex;-webkit-align-items: center;align-items: center;-webkit-justify-content: center;justify-content: center;}#form-wrap{width: 310px;height: 200px;/*padding: 10px;*/margin: 0 auto;/*background-color: #1BC295;*//*border-radius: 3px;*/}.field{display: block;width: 90%;margin: 0 auto;padding: 7px;font-size: 14px;background: none;border: none;color: #fff;outline: none;border-bottom: 1px solid #0FA67F;margin-bottom: 15px;}.field:hover{border-color: #169651;}.field:focus{border-color: #10703C;}.button{display: block;width: 50%;margin: 0 auto;padding: 7px;background-color: #169C77;border: none;cursor: pointer;border-radius: 15px;color: #fff;font-size: 15px;}.button:hover{background-color: #128566;}form{padding: 15px;background-color: #1BC295;border-radius: 3px;}.error{background-color: #FF3A37;color: #fff;border-radius: 3px;padding: 7px;text-align: center; margin-bottom: 5px;}</style></head>";
  content += "<body><div id='wrap'><div id='form-wrap'>";
  content += (msg != "") ? "<div class='error'>"+msg+"</div>" : "";
  content += "<form action='/login' method='post'>";
  content += "<label for='username' class='label'>Nome de usuário:</label>";
  content += "<input type='text' name='username' class='field'>";
  content += "<label for='password' class='label'>Senha:</label>";
  content += "<input type='password' name='password' class='field'>";
  content += "<button type='submit' class='button'>Entrar</button>";
  content += "</form></div></div></body></html>";
  server.send(200, "text/html", content);
}

/*
 * arHandler
 * Controla o ar-condicionado
 */
void arHandler(){
  Serial.println("[arHandler] - Entrou");

  if(server.hasArg("state")){
    Serial.println("[arHandler] - Tem o argumento state");

    if(server.arg("state") == "on"){ // Recebimento da requisição do botão (Ligar).
      Serial.println("[arHandler] - Tem o argumento state=on");

      if(isAuth()){
        Serial.println("[arHandler] - Ligou aparelho");
        digitalWrite(pinRele, HIGH); // Ligamento do aparelho.
        estadoRele = HIGH; // Variável auxiliar para mudar o estado On/Off na página Web.
        tempo = 0; // Variável tempo zerada, mesma função de quando o sensor está detectando presença.
      }

      server.sendHeader("Location", "/");
      server.sendHeader("Cache-Control", "no-cache");
      server.send(301);
    }

    if(server.arg("state") == "off"){ // Recebimento da requisição do botão (Desligar).
      Serial.println("[arHandler] - Tem o argumento state=off");

      if(isAuth()){
        Serial.println("[arHandler] - Desligou aparelho");
        digitalWrite(pinRele, LOW); // Desligamento do aparelho.
        estadoRele = LOW; // Variável auxiliar para mudar o estado On/Off na página Web.
        tempo = 300; // Após a requisição (Desligar), Impossibilitar que o sensor altere o estado do sistema durante 1 minuto. Assumindo este valor, será levado a condição de "pausa".
      }

      server.sendHeader("Location", "/");
      server.sendHeader("Cache-Control", "no-cache");
      server.send(301);
    }

  }
}

/*
 * notFoundHandler
 * Função chamada quando é acessada alguma página que não existe.
 * Simplesmente redirecionado para a página inicial
 */
void notFoundHandler(){
  server.sendHeader("Location", "/");
  server.sendHeader("Cache-Control", "no-cache");
  server.send(301);
}

/* 
 *  Dados MQTT 
 */

boolean reconnect(){
  char *username = "wuucqzes"; // Usuário MQTT.
  char *password = "iiEZRNQPmVxI"; // Senha MQTT.
  char *topic = "nodemcu/ar/cmd"; // Tópico MQTT.
  if (client.connect("nodemcu_001", username, password)) {
    client.subscribe(topic, 1);
  }
  return client.connected(); // Cliente conectado com sucesso.
}

/* 
 * Função que receberá uma mensagem do broker, e fará a NodeMCU executar uma ação.
 */
void mqtt_callback(char* topic, byte* payload, unsigned int length){
    String msg;
 
    //Obtem a string do payload recebido.
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }

    if(msg == "liga"){ // Quando receber a mensagem "liga".
      Serial.println("[MQTT] - Ligou aparelho");
      digitalWrite(pinRele, HIGH); // Ligamento do aparelho.
      estadoRele = HIGH; // Variável auxiliar para mudar o estado On/Off na página Web.
      tempo = 0; // Variável tempo zerada, mesma função de quando o sensor está detectando presença.
      client.publish("nodemcu/ar/state", "ligado");
    } else if(msg == "desliga"){ // Quando receber a mensagem "desliga".
      Serial.println("[MQTT] - Desligou aparelho");
      digitalWrite(pinRele, LOW); // Desligamento do aparelho.
      estadoRele = LOW; // Variável auxiliar para mudar o estado On/Off na página Web.
      tempo = 300; // Após a requisição (Desligar), Impossibilitar que o sensor altere o estado do sistema durante 1 minuto. Assumindo este valor, será levado a condição de "pausa".
      client.publish("nodemcu/ar/state", "desligou");
    } else {
      Serial.println("Comando invalido");
    }
}

