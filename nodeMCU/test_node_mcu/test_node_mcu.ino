//Sistema de Automação produzido por: José Maria Junior, Assis Neto, Renilson e Diego Rodrigues.
//SISTEMA DE AUTOMAÇÃO//


const int pinRele = 5; //Equivalente a D1 no NodeMCU.
const int pinSensor = 4; //Equivalente a D2 no NodeMCU.
int sensor = 0; //Variável que receberá o valor lido pelo sensor. 
int estadoRele; //Variável que auxiliará na mudança de estado On/Off na página Web.


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


#include <user_interface.h> //Biblioteca necessaria para acessar os Timer`s.

os_timer_t tmr0;//Cria o Timer.

unsigned long int tempo; //Variável de controle "Timer".
unsigned long int *z; //Variável auxiliar de controle do "Timer".

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
 
const char* ssid = ""; // Nome da Rede Wifi.
const char* password = ""; // Senha da Rede Wifi.

//IP do ESP
IPAddress ip(192, 168, 0, 113);
//IP do roteador da sua rede wifi
IPAddress gateway(192, 168, 0, 1);
//Mascara de rede da sua rede wifi
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80);


void setup() {

  Serial.begin(115200); //Iniciando comunicação serial para debug

  os_timer_setfn(&tmr0, contagem, NULL); //Indica ao Timer qual sera sua Sub rotina.
  os_timer_arm(&tmr0, 1000, true); // Contagem a cada 1 segundo.

  // Declaração de que o relé será usado como saída.
  pinMode(pinRele, OUTPUT);

  // Declaração de que o sensor será usado como entrada.
  pinMode(pinSensor,INPUT);


  // Conectando a rede WiFi
  WiFi.config(ip, gateway, subnet);
  WiFi.mode(WIFI_STA); // Configurar NodeMCU como estação
  WiFi.begin(ssid, password);

  // Caso haja perca de conexão, conectar-se novamente.
  if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, password); 
  }

  /*
   * Método "on" da classe ESP8266WebServer
   * Esse método registra uma URL e o que será feito quando
   * essa url for acessada. A assinatura do método é a seguinte:
   * on(string URL, HTTPMethod metodo, Handler)
   * URL -> URL do sistema que será controlada
   * metodo -> Método HTTP que será usada para acessar a URL. (É um ENUM da classe ESP8266WebServer)
   * handler -> Função que será executada quando a URL for acessada.
   */
  server.on("/", HTTP_GET, homeHandler);        //Rota inicial, primeira página a ser acessada pelo usuário
  server.on("/auth", HTTP_GET, authHandler);    //Rota que realiza e controla a autenticação do usuário
  server.on("/login", HTTP_POST, authHandler);  //Rota que processa os dados de login do usuário
  server.on("/ar", HTTP_GET, arHandler);        //Rota que faz o controle do ar-condicionado
  server.onNotFound(notFoundHandler);           //Esse método indica qual função será executada quando uma URL não registrada for acessada

  /*
   * Configurações do servidor
   * Necessárias para fazer funcionar a autenticação
   */
  const char * headerkeys[] = {"Cookie"}; // Nome do cabeçalho que será coletado a cada requisição
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*); // Memoria a ser alocada.
  server.collectHeaders(headerkeys, headerkeyssize); // Coleta dos cookies

  // Iniciando o servidor.
  server.begin();
  
}

void loop() {
  

  server.handleClient(); //Método do servidor que fica aguardando requisições.

  sensor = digitalRead(pinSensor); // Leitura do sinal recebido pelo Sensor de Presença PIR

  //-------------------------(If Sensor)------------------------------- 

  //Liga.
  if(sensor == 1){ // Com presença no local.

    if(tempo < 300 || tempo > 330){ // Entrará em funcionamento somente se o tempo for menor que 5:00 minutos ou maior que 5:30 minutos.
      digitalWrite(pinRele, LOW); // Ligamento do aparelho.
      estadoRele = LOW; // Variável auxiliar para mudar o estado On/Off na página Web.
      tempo = 0; // Zeramento da variável tempo para não cair na condição que aciona o desligamento e "pausa" o sistema por 30 segundos.    
    }
    
  }

  //Desliga.
  if(sensor == 0){ // Sem presença no local.

    if ( tempo > 0 && tempo < 300){ // Intervalo de tempo no qual apesar do sensor estiver captando zero, o aparelho continuará com o estado ligado.
      estadoRele = LOW;
    }

    // Quando chegar aos 5 minutos sem presença, desligar o aparelho e impossibilitar o sensor de interferir no sistema por 30segundos.        
    if(tempo > 299 && tempo < 331){ 
      digitalWrite(pinRele, HIGH); // Desligamento do aparelho.
      estadoRele = HIGH; // Variável auxiliar para mudar o estado On/Off na página Web.        
    }
    
    if(tempo > 330){ // Após o tempo de descanso de 30seg do aparelho, se sensor captar zero, o estado permanece desligado e continuará a contagem do tempo.
      digitalWrite(pinRele, HIGH); // Desligamento do aparelho.
      estadoRele = HIGH; // Variável auxiliar para mudar o estado On/Off na página Web.
    }
    
  }

}

/*
 * Sub rotina ISR do Timer sera acessada a cada 1 Segundo.
 */
void contagem(void*z)
{

  // Caso a variável chegue ao seu tamanho máximo, o valor será 331 novamente.
  if(tempo == 4294967290){
    
    tempo = 331; // Valor escolhido para que não caia na condição de "intervalo" entre valor 5:00 minutos e 5:30 minutos.
  }
 
   tempo++;

}

/*
 * isAuth()
 * Verifica se o usuário está logado, se estiver
 * retorna true, se não retorna false
 * 
 * .hasHeader() -> Método da classe ESP8266WebServer que verifica a existência de um cabeçalho HTTP na requisição atual.
 * .header() -> Método da classe ESP8266WebServer que retorna um cabeçalho HTTP.
 */
bool isAuth(){
  Serial.println("[isAuth] - Entrou");
  if (server.hasHeader("Cookie")) { //Verifica a existência de cookies
    Serial.println("[isAuth] - Tem o header Cookie");
    String cookie = server.header("Cookie"); //Pega o valor do cabeçalho HTTP Cookie
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
 * Essa função é executada quando é acessada a URL '/'.
 * A função verifica se o usuário está autenticado, se não estiver
 * o usuário simplesmente é redirecionado para o login, através de cabeçalhos HTTP.
 * 
 * sendHeader(cabeçalho, valor)
 */
void homeHandler(){
  String msg = "";
  Serial.println("[homeHandler] - Entrou");
  if (!isAuth()){ //verifica se o usuário está logado, se não estiver redireciona para o login
    Serial.println("[homeHandler] - Nao ta logado");
    server.sendHeader("Location", "/auth"); // envio do cabeçalho que indica redirecionamento.
    server.sendHeader("Cache-Control", "no-cache"); // cabeçalho para indicar para não fazer cache
    server.send(301); //envio do código HTTP que indica redirecionamento permanente
    return; //para a execução da função.
  }

  /*
   * Condição para exibição de alguma erro ou messagem
   * enviada ao usuário
   */
  if(server.hasArg("op") && server.arg("op") == "deny"){
    msg = "Sala Ocupada ! Operação não permitida.";
  }

  // Se o usuário estiver logado, será mostrado a tela de controle do aparelho.
  Serial.println("[homeHandler] - Exibe o conteudo da pagina");
  String content = "<!DOCTYPE html><html lang='pt-br'><head><meta charset='UTF-8'><meta http-equiv='refresh' content='3'><title>Controle e Monitoramento</title>";
  content += "<style type='text/css'>*{margin: 0;padding: 0;}body{font-family: Arial;background-color: #333;color: #fff;}#wrap{width: 960px;margin: 0 auto;text-align: center;font-size: 20px;font-weight: bold;}p#title{font-size: 25px;font-weight: bold;margin-top: 10%;margin-bottom: 2.5%;}#wrap table{margin: 0 auto;text-align: left;border-spacing: 0;border: none;font-weight: normal;}#wrap td{border-bottom: 1px solid #7f8c8d;padding: 10px;}#wrap tr:hover td{border-color: #fff;}tr td:first-child{font-weight: bold;}td button{font-size: 16px;padding: 6px;border: none;border-radius: 4px;color: #fff;cursor: pointer;outline: none;}button#on{background-color: #1BC295;}button#on:hover{background-color: #169C77;}button#off{background-color: #FF3A37;}button#off:hover{background-color: #CF312F;}#messages{width: fit-content;margin: 0 auto;margin-bottom: 2.5%;border: 1px solid #FF3A37;padding: 7px;font-weight: normal;border-radius: 3px;text-align: center;color: #FF3A37;}#logout{width: 150px;margin-top: 15px;font-size: 16px;padding: 6px;border: none;background-color: #e67e22;color: #fff;border-radius: 50px;cursor: pointer;}#logout:hover{background-color: #ED5E00;}</style>";
  content += "</head><body><div id='wrap'><p id='title'>Sistema Web de Controle e Monitoramento</p>";
  content += (msg != "") ? "<div id='messages'>"+msg+"</div>" : "";
  content += "<table><tr><td>Sala: </td><td colspan='2'>A-211</td></tr><tr><td>Estado: </td>";
  content += (estadoRele == LOW) ? "<td colspan='2' style='color: #1BC295'>On</td>" : "<td colspan='2' style='color: #FF3A37'>Off</td>";
  content += "</tr><tr><td>Ações: </td><td><a href='/ar?state=on'><button id='on'>Ligar</button></a></td><td><a href='/ar?state=off'><button id='off'>Desligar</button></a></td></tr><tr><td>Presença: </td>";
  content += (sensor == 1) ? "<td colspan='2'>Com presença</td>" : "<td colspan='2'>Sem presença</td>";
  content += "</tr></table><a href='/auth?logout=true'><button id='logout'>Sair</button></a></div></body></html>";
  server.send(200, "text/html", content); //Aqui é enviado ao cliente o código HTTP 200, que significa OK, o tipo da resposta, e o conteudo da resposta  
}

/*
 * authHandler
 * Função responsavel por gerenciar toda a autenticação do sistema
 * -Realiza o logout do usuário(setando o cookie de autenticação como 0, para indicar que não está logado)
 * -Previne que o usuário já autenticado acesse a página de login
 * -Realiza o login do usuário
 * -Exibe a página de login
 */
void authHandler(){
  Serial.println("[authHandler] - Entrou");
  String msg = "";
  //Faz o logout do usuário
  if(server.hasArg("logout")){ //Verifica se existe o argumento para logout
    Serial.println("[authHandler] - Logout");
    server.sendHeader("Location", "/auth");
    server.sendHeader("Cache-Control", "no-cache");
    server.sendHeader("Set-Cookie", "ESPSESSIONID=0");
    server.send(301);
    return;
  }

  //Caso o usuário logado tente acessar a página de login
  if(isAuth()){
    server.sendHeader("Location", "/");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }

  //Realiza o login do usuário
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

  //Caso os dados estejam incorretos a pagina de login é exibida novamente
  //com uma mensagem para o usuário
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
        digitalWrite(pinRele, LOW); // Ligamento do aparelho.
        estadoRele = LOW; // Variável auxiliar para mudar o estado On/Off na página Web.
        tempo = 0; // Variável tempo zerada, mesma função de quando o sensor está detectando presença.
      }

      server.sendHeader("Location", "/");
      server.sendHeader("Cache-Control", "no-cache");
      server.send(301);
      return;
    }

    if(server.arg("state") == "off"){ // Recebimento da requisição do botão (Desligar).
      Serial.println("[arHandler] - Tem o argumento state=off");

      if(isAuth()){
        if(sensor != 1){
          Serial.println("[arHandler] - Desligou aparelho");
          digitalWrite(pinRele, HIGH); // Desligamento do aparelho.
          estadoRele = HIGH; // Variável auxiliar para mudar o estado On/Off na página Web.
          tempo = 300; // Após a requisição (Desligar), Impossibilitar que o sensor altere o estado do sistema durante 30 segundos. Assumindo este valor, será levado a condição de "pausa".
        } else {
          Serial.println("[arHandler] - Tentou desligar um aparelho com presença na sala");
          server.sendHeader("Location", "/op=deny");
          server.sendHeader("Cache-Control", "no-cache");
          server.send(301);
          return;
        }
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
