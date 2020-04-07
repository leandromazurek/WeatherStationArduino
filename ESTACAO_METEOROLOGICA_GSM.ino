// Thingspeak  
String statusChWriteKey = "VAVMIAK3LD5LFOKQ";  // Status Channel id: 999999

#include <SdFat.h>
#include <Timing.h>

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define SEALEVELPRESSURE_HPA (1013.25)//BME280

#include <DS3231.h>

#include <OneWire.h>           //DS18B20
#include <DallasTemperature.h> //DS18B20
#define DS18B20 7 //DEFINE O PINO DIGITAL DS18B20

#include <SoftwareSerial.h> 

//PLUVIOMETRO
const int REED = 8;              //The reed switch outputs to digital pin 9
int val = 0;                    //Current value of reed switch
int old_val = 0;                //Old value of reed switch
int REEDCOUNT = 0;              //This is the variable that hold the count of switching
String chuva = "0";

//SIM800L
SoftwareSerial SIM800L(10, 11); // Rx,  Tx - SIM800L
String sendThinkspeak, sendGeoclimamt, statusThinkspeak ="", statusGeoclimamt="";

//SD CARD
const int chipSelect = 53;// Pino ligado ao CS do modulo 
String cartaosd = "";

//RTC DS3231
// --- Comunicação I2C ---
DS3231  rtc(20 , 21);//sda, scl
Time t;
String datahora_leitura = "";
String data_rtc = "";
String hora_rtc = "";
String min_rtc = "";
String data = "";
String data_old = "";
String min_old = "";

//WIND_DIR
int pin=1; //pino A1
float direcao_vento =0;
int Winddir =0;

//BME280
Adafruit_BME280 bme; 
float airTemp = 0;
float airHum = 0;
float pressao = 0;
float altitude = 0;
double pontoOrvalho = 0;
String pulverizar = "";

//ANEMOMETRO
float anemoVentokm = 0;
float anemoVentoms = 0;
// Constants definitions
const float pi = 3.14159265;           // Numero pi
int period = 5000;               // Tempo de medida(miliseconds)
int radius = 147;      // Aqui ajusta o raio do anemometro em milimetros  **************
// Variable definitions
unsigned int Sample = 0;   // Sample number
unsigned int counter = 0; // magnet counter for sensor
unsigned int RPM = 0;          // Revolutions per minute
float speedwind = 0;         // Wind speed (m/s)
float windspeed = 0;           // Wind speed (km/h)

//RADIAÇÃO ULTRA VIOLETA
int pino_sensor_UV = A0;
int valor_sensor = 0;
String UV_index = "0";
//String uvValorTabela = "0";
int tensao_UV = 0;
//int uvTensao = 0;


// Variáveis a serem usadas com timers
Timing timerSensores; // Um timer para cada
Timing timerWriteSend;
Timing timerPluviometro;

// Variáveis a serem usadas para soma
float somaTemp = 0;
float somaUmid = 0;
float somaVentokm = 0;
float somaVentoms = 0;
float somaVentodir = 0;
float somaPressao = 0;
float somaAltitude = 0;
float somaUVtensao = 0;

// Variáveis a serem usadas como média
float mediaTemp = 0;
float mediaUmid = 0;
float mediaVentoms = 0;
float mediaVentokm = 0;
float mediaVentodir = 0;
float mediaPressao = 0;
float mediaAltitude = 0;
float mediaUVtensao = 0;
int contMin = 1;

void setup() { 
  
  Serial.begin(9600);
    
  //DS3231
  rtc.begin();
  //Descomente as linhas a seguir para configurar o horário, após comente e faça o upload novamente para o Arduino
  //rtc.setDOW(SUNDAY);     // Set Day-of-Week to SUNDAY
  //rtc.setTime(17, 52, 00);     // Set the time to 12:00:00 (24hr format)
  //rtc.setDate(28, 10, 2018);   // Set the date to January 1st, 2014

  //PLUVIOMETRO
  // initializa o pino do switch como entrada
   pinMode (REED, INPUT_PULLUP); //This activates the internal pull up resistor
   digitalWrite(2,HIGH);

  //BME280
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }else{
    Serial.println("Sensor BME280 OK!");
  }    
  
  //DS18B20
  //ds18b20.begin();  

 //ANEMOMETRO
  pinMode(2, INPUT); // Set the pins
  digitalWrite(2, HIGH);     //internall pull-up active    

 //RADIAÇÃO ULTRA VIOLETA
  pinMode(pino_sensor_UV, INPUT); 

  //SIM800L
  SIM800L.begin(9600);

  
}

void loop() {

  //LER SENSORES
  if (timerSensores.onTimeout(5000)) {
    readSensors();
  } 
      
  //GRAVAR DADOS NO CARTAO DE MEMÓRIA
  if (timerWriteSend.onTimeout(60000)) {
    writeSD();
    sendGSM();
    bluetooth("TEMP|", mediaTemp, "UMID|", mediaUmid, "PONTOORVALHO|",pontoOrvalho, "VENTOKM|",mediaVentokm, "VENTODIR|",mediaVentodir, "VENTOMS|", mediaVentoms, "PULVERIZAR|",pulverizar, "PRESSAO|",mediaPressao, "ALTITUDE|",mediaAltitude, "UVTENSAO|",mediaUVtensao, "UVINDEX|",UV_index, "CHUVA|",chuva, "DATAHORA|",datahora_leitura, "CARTAOSD|",cartaosd, "THINKSPEAK|",statusThinkspeak, "GEOCLIMAMT|",statusGeoclimamt);  
  }
   
  //DADOS PLUVIOMETRO
  readPluviometro(); 
   
}

/*********************************************
 * FUNÇÃO PARA ENVIAR DADOS AO THINKSPEAK E GEOCLIMAMT
 *********************************************/
void sendGSM(){
   command("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"",1000);
   command("AT+SAPBR =3,1,\"APN\",\"gprs.oi.com.br\"",1000);
   command("AT+SAPBR =3,1,\"USER\",\"oi\"",1000);
   command("AT+SAPBR =3,1,\"PWD\",\"oi\"",1000);
   command("AT+SAPBR=1,1",2000);
   command("AT+SAPBR=2,1",1000);
   command("AT+HTTPINIT",1000);
   command("AT+HTTPPARA =\"CID\",1",1000);
   sendThinkspeak = "AT+HTTPPARA=\"URL\",\"http://api.thingspeak.com/update?api_key=VAVMIAK3LD5LFOKQ&field1=";
   sendThinkspeak += String(mediaTemp);
   sendThinkspeak +="&field2=";
   sendThinkspeak += String(mediaUmid);
   sendThinkspeak +="&field3=";
   sendThinkspeak += String(mediaVentokm);
   sendThinkspeak +="&field4=";
   sendThinkspeak += String(mediaVentoms);
   sendThinkspeak +="&field5=";
   sendThinkspeak += String(mediaUVtensao);
   sendThinkspeak +="&field6=";
   sendThinkspeak += String(UV_index);
   sendThinkspeak +="&field7=";
   sendThinkspeak += String(chuva);
   sendThinkspeak +="&field8=";
   sendThinkspeak += String(mediaVentodir);
   sendThinkspeak += String("\"""");
   command(sendThinkspeak, 1000);
    
   command("AT+HTTPACTION=0",1000);
   command("AT+HTTPREAD",1000);
   command("AT+HTTPTERM",1000);

   command("AT+HTTPINIT",1000);
   command("AT+HTTPPARA =\"CID\",1",1000);
   //command("AT+HTTPPARA=\"URL\",\"pesquisa.unemat.br/geoclimamt/enviardadosestacao.php?temp=2&umid=1&ventokm=2&ventoms=2&uvtensao=2&uvindex=2&chuva=2&data=2&hora=2\"",2000);
   sendGeoclimamt = "AT+HTTPPARA=\"URL\",\"pesquisa.unemat.br/geoclimamt/enviardadosestacao.php?temp=";
   sendGeoclimamt += String(mediaTemp);
   sendGeoclimamt +="&umid=";
   sendGeoclimamt += String(mediaUmid);
   sendGeoclimamt +="&pontodeorvalho=";
   sendGeoclimamt += String(pontoOrvalho);
   sendGeoclimamt +="&ventokm=";
   sendGeoclimamt += String(mediaVentokm);
   sendGeoclimamt +="&ventoms=";
   sendGeoclimamt += String(mediaVentoms);
   sendGeoclimamt +="&ventodir=";
   sendGeoclimamt += String(mediaVentodir);
   sendGeoclimamt +="&uvtensao=";
   sendGeoclimamt += String(mediaUVtensao);
   sendGeoclimamt +="&uvindex=";
   sendGeoclimamt += String(UV_index);
   sendGeoclimamt +="&chuva=";
   sendGeoclimamt += String(chuva);
   sendGeoclimamt +="&pressao=";
   sendGeoclimamt += String(mediaPressao);
   sendGeoclimamt +="&altitude=";
   sendGeoclimamt += String(mediaAltitude);  
   sendGeoclimamt +="&data=";
   sendGeoclimamt += String(data_rtc);
   sendGeoclimamt +="&hora=";
   sendGeoclimamt += String(hora_rtc);
   sendGeoclimamt += String("\"""");
   command(sendGeoclimamt, 1000);
   command("AT+HTTPACTION=0",1000);
   command("AT+HTTPREAD",1000);
   command("AT+HTTPTERM",1000);
   
   command("AT+SAPBR=0,1",2000);
}

/*********************************************
 * FUNÇÃO PARA COMANDOS AT
 *********************************************/
String command(String toSend, unsigned long milliseconds) {
  String result;
  //Serial.print("Sending: ");
  //Serial.println(toSend);
  SIM800L.println(toSend);
  unsigned long startTime = millis();
  //Serial.print("Received: ");
  while (millis() - startTime < milliseconds) {
    readPluviometro();
    if (SIM800L.available()) {
      readPluviometro();
      char c = SIM800L.read();
       if(sendThinkspeak == toSend){
        readPluviometro();
        if(SIM800L.find((char *)"OK")){
          statusThinkspeak = " OK";
          readPluviometro();
        }else if(SIM800L.find((char *)"ERROR"))
          statusThinkspeak = " ERRO";
          readPluviometro();
       }
       if(sendGeoclimamt == toSend){
        readPluviometro();
        if(SIM800L.find((char *)"OK")){
          statusGeoclimamt = " OK";
          readPluviometro();
        }else if(SIM800L.find((char *)"ERROR"))
          statusGeoclimamt = "ERRO";
          readPluviometro();
       }
      //Serial.write(c);      
      result += c;  // append to the result string
    }
  }
//Serial.println();  // new line after timeout.
return result;
}

/*********************************************
 * FUNÇÃO BLUETOOTH
 *********************************************/
void bluetooth(String sensor1, float valorsensor1, String sensor2, float valorsensor2, String sensor3, float valorsensor3, String sensor4, float valorsensor4, String sensor5, float valorsensor5, String sensor6, float valorsensor6, String sensor7, String valorsensor7, String sensor8, float valorsensor8, String sensor9, float valorsensor9, String sensor10, float valorsensor10, String sensor11, String valorsensor11, String sensor12, String valorsensor12, String sensor13, String valorsensor13, String sensor14, String valorsensor14, String sensor15, String valorsensor15, String sensor16, String valorsensor16) {
  unsigned long startTime = millis();
  Serial.print(sensor1);
  Serial.println(valorsensor1);  
  while (millis() - startTime < 1000) {
    readPluviometro();    
  }
  Serial.print(sensor2);
  Serial.println(valorsensor2);  
  while (millis() - startTime < 2000) {
    readPluviometro();    
  }
  Serial.print(sensor3);
  Serial.println(valorsensor3);  
  while (millis() - startTime < 3000) {
    readPluviometro();    
  }
  Serial.print(sensor4);
  Serial.println(valorsensor4);  
  while (millis() - startTime < 4000) {
    readPluviometro();    
  }
  Serial.print(sensor5);
  Serial.println(valorsensor5);  
  while (millis() - startTime < 5000) {
    readPluviometro();    
  }
  Serial.print(sensor6);
  Serial.println(valorsensor6);  
  while (millis() - startTime < 6000) {
    readPluviometro();    
  }
  Serial.print(sensor7);
  Serial.println(valorsensor7);  
  while (millis() - startTime < 7000) {
    readPluviometro();    
  }
  Serial.print(sensor8);
  Serial.println(valorsensor8);  
  while (millis() - startTime < 8000) {
    readPluviometro();    
  }
  Serial.print(sensor9);
  Serial.println(valorsensor9);  
  while (millis() - startTime < 9000) {
    readPluviometro();    
  }
  Serial.print(sensor10);
  Serial.println(valorsensor10);  
  while (millis() - startTime < 10000) {
    readPluviometro();    
  }
  Serial.print(sensor11);
  Serial.println(valorsensor11);  
  while (millis() - startTime < 11000) {
    readPluviometro();    
  }
  Serial.print(sensor12);
  Serial.println(valorsensor12);  
  while (millis() - startTime < 12000) {
    readPluviometro();    
  }
  Serial.print(sensor13);
  Serial.println(valorsensor13);  
  while (millis() - startTime < 13000) {
    readPluviometro();    
  }
  Serial.print(sensor14);
  Serial.println(valorsensor14);  
  while (millis() - startTime < 14000) {
    readPluviometro();    
  }
  Serial.print(sensor15);
  Serial.println(valorsensor15);  
  while (millis() - startTime < 15000) {
    readPluviometro();    
  }
  Serial.print(sensor16);
  Serial.println(valorsensor16);  
  while (millis() - startTime < 16000) {
    readPluviometro();    
  }
}



/*********************************************
 * Funções do Pluviometro
 *********************************************/
void readPluviometro(){
  //PLUVIOMETRO
  val = digitalRead(REED);      //Read the status of the Reed swtich
  data = rtc.getDateStr();
  if(data_old != data){
    REEDCOUNT = 0;
    old_val = 0;
    val = 0;
  }
  if ((val == LOW) && (old_val == HIGH)){
    if (timerPluviometro.onTimeout(10)) {    
      REEDCOUNT = REEDCOUNT + 1;   //Add 1 to the count of bucket tips
      old_val = val;              //Make the old value equal to the current value
      chuva = ((REEDCOUNT*0.254)); 
      Serial.println(chuva);  
      data_old = data;
    } 
  }else {   
      old_val = val;              //If the status hasn't changed then do nothing
      data_old = data;   
  }
}
/*********************************************
 * Funções do anemometro
 *********************************************/
 // Measure wind speed
void windvelocity(){
  speedwind = 0;
  windspeed = 0;
  
  counter = 0;  
  attachInterrupt(0, addcount, RISING);
  unsigned long millis();       
  long startTime = millis();
  while(millis() < startTime + period) {
    readPluviometro();
  }
}

void RPMcalc(){
  RPM=((counter)*60)/(period/1000);  // Calculate revolutions per minute (RPM)
}

void WindSpeed(){
  windspeed = ((4 * pi * radius * RPM)/60) / 1000;  // Calculate wind speed on m/s 
}

void SpeedWind(){
  speedwind = (((4 * pi * radius * RPM)/60) / 1000)*3.6;  // Calculate wind speed on km/h 
}

void addcount(){
  counter++;
} 

/*********************************************
 * Funções do Sensor RADIAÇÃO ULTRA VIOLETA
 *********************************************/
 void Calcula_nivel_UV()
{
  valor_sensor = analogRead(pino_sensor_UV);
  //Calcula tensao em milivolts
  int tensao = (valor_sensor * (3.3 / 1023.0)) * 1000;
  tensao_UV = tensao;
  //Compara com valores tabela UV_Index
  if (tensao > 0 && tensao < 50)
  {
    UV_index = "0";
  }
  else if (tensao > 50 && tensao <= 227)
  {
    UV_index = "0";
  }
  else if (tensao > 227 && tensao <= 318)
  {
    UV_index = "1";
  }
  else if (tensao > 318 && tensao <= 408)
  {
    UV_index = "2";
  }
  else if (tensao > 408 && tensao <= 503)
  {
    UV_index = "3";
  }
  else if (tensao > 503 && tensao <= 606)
  {
    UV_index = "4";
  }
  else if (tensao > 606 && tensao <= 696)
  {
    UV_index = "5";
  }
  else if (tensao > 696 && tensao <= 795)
  {
    UV_index = "6";
  }
  else if (tensao > 795 && tensao <= 881)
  {
    UV_index = "7";
  }
  else if (tensao > 881 && tensao <= 976)
  {
    UV_index = "8";
  }
  else if (tensao > 976 && tensao <= 1079)
  {
    UV_index = "9";
  }
  else if (tensao > 1079 && tensao <= 1170)
  {
    UV_index = "10";
  }
  else if (tensao > 1170)
  {
    UV_index = "11";
  }
}

/*********************************************
 * Funções do Sensor Biruta - DIREÇÃO DO VENTO
 *********************************************/
 void Calcula_Biruta()
{
  direcao_vento = analogRead(pin)* (5.0 / 1023.0); //leitura do sensor
    
  if (direcao_vento <= 0.28) {
  Winddir = 315;
  }
  else if (direcao_vento <= 0.32) { 
  Winddir = 270;
  }
  else if (direcao_vento <= 0.38) { 
  Winddir = 225;
  }
  else if (direcao_vento <= 0.45) { 
  Winddir = 180;
  }
  else if (direcao_vento <= 0.57) { 
  Winddir = 135;
  }
  else if (direcao_vento <= 0.75) { 
  Winddir = 90;
  }
  else if (direcao_vento <= 1.25) {  
  Winddir = 45;
  }
  else {  
  Winddir = 000;
  }
}

/********* FUNÇÃO PONTO DE ORVALHO *************/
//  CÁLCULO DE PONTO DE ORVALHO
//  Funções de ponto de orvalho obtidas em http://playground.arduino.cc/Main/DHT11Lib
//  A fazer: Verificar e referenciar o código apropriadamente
//  O código abaixo foi copiado e usado sem alterações
//-----------------------------------------------------------

// dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm

double dewPoint(double celsius, double humidity)
{
  // (1) Saturation Vapor Pressure = ESGG(T)
  double RATIO = 373.15 / (273.15 + celsius);
  double RHS = -7.90298 * (RATIO - 1);
  RHS += 5.02808 * log10(RATIO);
  RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / RATIO ))) - 1) ;
  RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
  RHS += log10(1013.246);

  // factor -3 is to adjust units - Vapor Pressure SVP * humidity
  double VP = pow(10, RHS - 3) * humidity;

  // (2) DEWPOINT = F(Vapor Pressure)
  double T = log(VP / 0.61078); // temp var
  return (241.88 * T) / (17.558 - T);
}

/********* Read Sensors value *************/
void readSensors(void)
{ 
    
  t = rtc.getTime();
  min_rtc = t.min,DEC;
  if(min_old != min_rtc){
    somaTemp = 0;
    somaUmid = 0;
    somaVentokm = 0;
    somaVentoms = 0;
    somaVentodir = 0;
    somaPressao = 0;
    somaAltitude = 0;
    somaUVtensao = 0;
    mediaTemp = 0;
    mediaUmid = 0;
    mediaVentokm = 0;
    mediaVentoms = 0;
    mediaVentodir = 0;
    mediaPressao = 0;
    mediaAltitude = 0;
    mediaUVtensao = 0;
    pontoOrvalho = 0;
    contMin = 1;
  }
  /*Serial.print("min_old|");
  Serial.println(min_old);
  Serial.print("min_rtc|");
  Serial.println(min_rtc);*/
  
  //BME280
  airTemp = bme.readTemperature();
  airHum = bme.readHumidity(); 
  pressao = bme.readPressure() / 100.0F;
  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  somaTemp = somaTemp + airTemp;
  mediaTemp = somaTemp/contMin;  
  
  somaUmid = somaUmid + airHum;
  mediaUmid = somaUmid/contMin;

  somaAltitude = somaAltitude + altitude;
  mediaAltitude = somaAltitude/contMin;
  
  somaPressao = somaPressao + pressao;
  mediaPressao = somaPressao/contMin;

  pontoOrvalho = dewPoint(mediaTemp, mediaUmid);

  //ANEMOMETRO    
  windvelocity();
  RPMcalc();
//print m/s  
  WindSpeed();
//print km/h  
  SpeedWind();
  anemoVentokm = speedwind;
  anemoVentoms = windspeed;
  
  somaVentokm = somaVentokm + anemoVentokm;
  mediaVentokm = somaVentokm/contMin;
  
  somaVentoms = somaVentoms + anemoVentoms;
  mediaVentoms = somaVentoms/contMin;

  //DIREÇÃO DO VENTO
  Calcula_Biruta();
  somaVentodir = somaVentodir + Winddir;
  mediaVentodir = somaVentodir/contMin;

  //PULVERIZAR ?
  if((mediaTemp <= 30)&&(mediaUmid >=50)&&(mediaVentokm >=3)&&(mediaVentokm <=10)){
    pulverizar = "SIM";
  }else{
    pulverizar = "NÃO";
  }
  
//uv_index
  Calcula_nivel_UV();
  //uvTensao = tensao_UV;
  //uvValorTabela = UV_index;
  somaUVtensao = somaUVtensao + tensao_UV;
  mediaUVtensao = somaUVtensao/contMin;
  
  //rtc data e hora de leitura
   data_rtc = rtc.getDateStr(); 
   hora_rtc = rtc.getTimeStr();
   datahora_leitura = data_rtc + "-" + hora_rtc;
   /*Serial.print("DATAHORA|");
   Serial.println(datahora_leitura);*/

   //ATUALIZA O CONTADOR DO MINUTO
   contMin = contMin + 1; 
   //Serial.println(contMin);
   min_old = min_rtc;

   //Serial.println("");
}

/********* write SD value *******/
void writeSD(void)
{    
SdFat sdCard;

   if (!sdCard.begin(chipSelect))
  {
    cartaosd = " ERRO";
    return;
  }

  //--------------------------------
  File meuArquivo =  sdCard.open("estacaobc.txt", FILE_WRITE);
  if (meuArquivo)
  {
    meuArquivo.print(mediaTemp);//Temperatura
    meuArquivo.print(";");
    meuArquivo.print(mediaUmid);//Umidade
    meuArquivo.print(";");   
    meuArquivo.print(pontoOrvalho);//Ponto de Orvalho
    meuArquivo.print(";");  
    meuArquivo.print(mediaVentokm);//Vento km/h
    meuArquivo.print(";");
    meuArquivo.print(mediaVentoms);//Vento m/s
    meuArquivo.print(";");
    meuArquivo.print(mediaVentodir);//Vento direção
    meuArquivo.print(";");
    meuArquivo.print(mediaUVtensao);//uv Tensão
    meuArquivo.print(";");
    meuArquivo.print(UV_index);//uv Index
    meuArquivo.print(";");
    meuArquivo.print(chuva);//Chuva
    meuArquivo.print(";");
    meuArquivo.print(mediaPressao);//Pressão Atmosférica
    meuArquivo.print(";");
    meuArquivo.print(mediaAltitude);//Altitude
    meuArquivo.print(";");
    meuArquivo.print(data_rtc);//Data leitura
    meuArquivo.print(";");
    meuArquivo.print(hora_rtc);//hora leitura
    meuArquivo.println(";");
  
    meuArquivo.close();
    cartaosd = " OK";
  }
  else
  {
    Serial.println("erro ao gravar / Cartao não inserido");
  }
}  
