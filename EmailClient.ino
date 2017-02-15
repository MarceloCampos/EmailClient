/*
  Email client - Envia Emails à partir de contas que não requerem SSL
  by Marcelo Campos - Garoa Hacker Club
  https://garoa.net.br/wiki/Usu%C3%A1rio:Marcelo_campos
  
  REV. 0 - inicial - 12/fev/17

Notas Importantes:
  1. O email e a senha devem ser enviados codificados em Base64, 
  pode ser usado este link p/ codificação: http://ostermiller.org/calc/encode.html
  2. À confirmar: o servidor de envio pode recusar conexões de endereços MAC
  desconhecidos / inválidos, ou repetidas conexões seguidas de um mesmo MAC addr

Se usando shield ethernet com o chip ENC28j60, inlcuir a lib UIPEthernet:
  https://github.com/ntruchsess/arduino_uip

  TODO: Timeout de respostas, detectação e tratamento de mesnagem de erros vindas do servidor
  
 */

#include <SPI.h>

#include <Ethernet.h>  // ethernet shield com chip W5100
//#include <UIPEthernet.h>  // ethernet shield com chip ENC28j60

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

//IPAddress server(74,125,232,128);  // unsando reduz código, fica sem dns, numeric IP Google (no DNS)

// ---- servidor Smart Radio
char server[] = "mail.smartradio.com.br";    // name address do servidor de mail
int port = 26;

int Sm_State = 0;

String Linha_Atual;

// static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 0, 177);

EthernetClient client;

void setup() 
{
  Serial.begin(115200);
  while (!Serial) 
  {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  Serial.println("Sistema Ligado");
     
  if (Ethernet.begin(mac) == 0) 
  {
    Serial.println("Falha Conec. por DHCP");   
    Ethernet.begin(mac, ip); // tenta por DHCP:
  }

  Serial.print("Rede conectada OK, IP Address: ");
  Serial.println( Ethernet.localIP() );

  
  delay(150);
  Serial.println("conectando servidor...");
  Serial.println("");
    
  // if you get a connection, report back via serial:
  if (client.connect(server, port)) 
  {   
    Sm_State++;  
  }
  else 
  {
    // Falhou :-(
    Serial.println("Falha na conexão ao servidor");
    Sm_State = -1; 
  }
}

void loop()
{

  while(1)
  {
    if ( client.available() ) 
      Processa_Entradas();

    switch (Sm_State)
    {
      case 1:
        // request:
        client.println("EHLO mail.smartradio.com.br");
        Sm_State++;
        break;
        
      case 4:
        client.println("AUTH LOGIN");
        Sm_State++;
        break;

      case 6:   // Autenticação: envia email codificado Base64
        client.println("YXV0b21haWxAc21hcnRyYWRpby5jb20uYnI=");   // smart
        Sm_State++;
        break;

      case 8:   // Autenticação: envia Senha codificada Base64  
        client.println("MTIzNDU2");                               // smart
        Sm_State++;
        break; 
        
      case 10:   // Authentication succeeded
        Send_Email_Message();
        Sm_State++;
        break;       
    }
    
    Monitor_Conec();
  }

}

void Send_Email_Message()
{
  Serial.println("[Enviando Mensagem email para o Server ...");
  client.println("MAIL FROM:automail@smartradio.com.br size=100");
  client.println("RCPT TO:XXXXXX@XXXXXXXXXX.com.br");  
  client.println("DATA");
  client.println("Subject:Enviando Email Arduino por Telnet");
  client.println(""); // (send blank line)
  client.println("Sistema de Email Automatizado Arduino");
  client.println("by Marcelo Campos - Fev/17");
  client.println(".");  // ( pra encerrar enviar só o ponto numa linha)
}

void Processa_Entradas()
{
  char c = client.read();

  if (c != '\n')
    Linha_Atual += c; 
  else
  {
   // Serial.println(Linha_Atual);
    Analisa_Linha();
    Linha_Atual = "";
  }
// Serial.print(c);
}

void Analisa_Linha()
{
  if ( Linha_Atual.startsWith("220") )
  {
    if (Sm_State == 2)
    {
      Sm_State++;
      Serial.println("[Server Avisos 1]");
    }
  }
  else if ( Linha_Atual.startsWith("250") )
  {
    if (Sm_State == 3)
    {
      Sm_State++;
      Serial.println("[Server Avisos 2]");
    }
    else if (Sm_State == 11)
    {
      Sm_State++;
      Serial.println("[ Mensagem Enviada OK ]");
    }    
  }
  else if ( Linha_Atual.startsWith("334 VXNlcm5hbWU6") )
  {
    if (Sm_State == 5)
    {
      Sm_State++;
      Serial.println("[Server Req Email]");
    }   
  }  
  else if ( Linha_Atual.startsWith("334 UGFzc3dvcmQ6") )
  {
    if (Sm_State == 7)
    {
      Sm_State++;
      Serial.println("[Server Req Pass]");
    }    
  }   
  else if ( Linha_Atual.startsWith("235") ) //  Authentication succeeded !!!!
  {
    if (Sm_State == 9)
    {
      Sm_State++;
      Serial.println("[ Authentication succeeded]");
    }    
  }
 
           
}

void Monitor_Conec()
{
  if (!client.connected()) 
  {
    Serial.println();
    Serial.println("Deconectado");
    client.stop();    // necessário stop the client:

    // TODO : ver pra reconectar quando necessário !!
    while (true);
  }
}
