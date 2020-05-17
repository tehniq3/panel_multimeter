/*
 * indicator de panou
 * program scris de Nicu FLORICA (niq_ro) pentru Georgel GHITA (zvonacfirst)
 * http://www.arduinotehniq.com
 * https://nicuflorica.blogspot.com/
 * http://arduinotehniq.blogspot.com/
 * 
 * ver.0 - indicare tensiune, curent, putere, temperatura, decuplare releu la supratempratura si supracurent
 * ver.0a1 - corectat afisare temperaura si facut mai multe masuratori de temperatura ca si la tensiune si curent
 * ver.0a2 - tensiune de referinta interbna, diferita
 * ver,0a3 - adaugare coeficient corectie teniuje si curent
 * ver.0a4 - corectie calcul curent
 * ver.0a5 - eliminare automata offset operational
 * ver.0a6 - aducere la 0,00A a curentului pentru valori negative (-0,01A)
 * ver.0a7 - aducere la 0,00V a tensiunii pentru valori negative (-0,01V)
 * ver.0a8 - corectare masurare (ADC -> ADC+0.5), confotmr http://www.skillbank.co.uk/arduino/measure.htm
 * ver.0a9 - eliminat corectiile ! adaugat apasare lunga pentru cuplare releu
 * ver.0b1 - adaugare pornire ventilator la turatie maxima pentru o scurta perioada de timp
 * ver.0b2 - ?!
 * ver.03b - adaugare corectie curent la pornire ventilator racire
 * ver.0b4 - adaugare animatie ventilator
 * ver.0b5 - modificare corectie, curatat variabile mari
 * ver.0b6 - cuplare releu
 * ver.0b7 - corectii la curent cu ventilator, cuplare/decuplare releu
 * ver.0b8 - tot corectii
 * ver.0b8 - corectie curent la tensiune zweo
 * ver.0c - teste pana se obin rezultate bune
 * ver.0c0f - schimbat partea de amplificare curent si ajusrari mici
 */

#define pinAN0 A0  // pinul de masura tensiune
#define pinAN1 A1  // pinul de masura curent
#define pinAN2 A2  // pinul de masura temperatura

#define pinRST 12  // pinul de armare/dezarmare releu (buton fara retinere)
#define pinREL 11  // pinul de comanda releu
#define pinFAN 10  // pin de comanda ventilator
#define pinBUZ 9  // pin de comanda avertizor acustic

#define atras LOW  // comanda releu
#define repaus HIGH
/*
#define atras HIGH  // comanda releu
#define repaus LOW
*/

#include <LiquidCrystal.h>  // se foloseste libraria pentru controlul afisajului LCD
const int rs = 2, en = 3, d4 = 4, d5 = 5, d6 = 6, d7 = 7;  // definire conectare pini
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);  // creere "obiect" pentru afisare

// http://arduino.cc/en/Reference/LiquidCrystalCreateChar  // definim un simbol pentru grad Celsius
byte grad[8] = {
  B01100,
  B10010,
  B10010,
  B01100,
  B00000,
  B00000,
  B00000,
};

byte cooler1[8] = {  // definire simbol pentru ventilator, creea de Georgel Hhita
 B00000,
 B11001,
 B01011,
 B00100,
 B11010,
 B10011,
 B00000,
 B00000 
};

byte cooler2[8] = {  // definire simbol pentru ventilator, creea de Georgel Hhita
B00000,
  B10011,
  B11010,
  B00100,
  B01011,
  B11001,
  B00000,
  B00000 
};

float R2; // rezistenta conectata de la + la A0
float R1 = 1. ; // rezistenta conectata de la A0 la GND
float rsunt = 0.01 ; // valoare rezistenta masura (sunt)
float Amp = 10. ; // valoare amplificare operational (reglata din semireglabil)

//float vref = 1.100 ;  // tensiune de referinta interna 
float vref ;  // se va selecta ulterior tensiune de referinta interna (1100mV) pentru ATmega328 si ATmega168 sau (2560mV) ATmega32U4 siATmega8
              // se va selecta ulterior tensiune de referinta externa (2496mV) pentru TL431


int trcurent = 0;  // treapta masurare curent (0...1023)
int trtensiune = 0; // treapta masurare tensiune (0...1023)

float sumatensiune = 0.;   // valoare insumare tensiune pentru calcul medie
float sumacurent = 0.; // valoare insumare curent pentru calcul medie
float sumatemp = 0. ; // valoare insumare temperaturi pentru calcul medie

double curent = 0.;   // valoare curent
float tensiune = 0.; // valoare tensiune
float putere = 0.;   // valoare putere
float curentmax = 10100; // curent maxim, in mA

float kv; // coeficient corectie tensiune
float ka; // coeficient corectie curent


int te;  // variabila pentru temperatura
int trtemp = 0; // treapta masurare temperatura (0...1023)
int temin = 40;  // prag temperatura pentru pornire ventilator
byte rpmmin = 155; // turatie minima pentru pornire ventilator
byte temax = 60;  // prag temperatura pentru pornire ventilator la turatie maxima
byte rpmmax = 250; // turatie maxima ventilator
byte telim = 100; //temperatura pentru care decupleaza releul
byte rpm;  // turatie controlata ventilator

//boolean senzorLM35 = false; 
boolean senzorLM35 = true; // senzor LM35 montat

float x0; // tensiune pe iesire operational, in gol, in mV
float x1;  // tensiune pentru un curent masurat, y1, in mV
float y0;  // curent zero (0A)
float y1;  // curent masurat, in mAmperi
double m = 0; // panta
float tensiuneoa = 0;
float tensiuneoa1 = 0;
float tensiuneoa2 = 0;
float tensiuneoa3 = 0;
float tensiuneoa4 = 0;
float tensiuneoa5 = 0;

float di = 20.; // corectie curent, im mA

// partea pentru apasare lunga
boolean apasarebuton = 0; // stare buton (apasat/neapasat)
int duratamare = 1500; // valoare timp apaaare lunga, in ms
int durataapasare = duratamare; // durata apasare buton
unsigned long tpaparare = 0;// memorare timp apasare
unsigned long tpeliberare = 0;// memorare timp eliberare
boolean stare = false; // stare de cronometrare

//float curentcorectie = 0.07;  // valoare corectie pt masurare cu ventilator pornit
float curentcorectie = 0.0;  // valoare corectie pt masurare cu ventilator pornit
boolean ventilator = 0;  // variabila pentru stare ventilator
boolean avarie = 0; // variabila pentru depasir temperatura
boolean releu = 0;  // variabila pentru stare releu

float curentreleu = 0; // inflenta pornire releu
float curentventilator= 0; // inflenta pornire ventilator
float curentventilator2= 0; // inflenta pornire ventilator in PWM
float curent2= 0; // inflenta pornire ventilator si releu
float curent3= 0; // inflenta pornire ventilator in PWM si releu

float cur1;
int cur2, cur3;

boolean model = 1;  // 1 = Nicu, 0 - Georgel

void setup() {  // ce este pus aici ruleaza doar o data

if (model == 1)  // Nicu
{
  analogReference(EXTERNAL); // referinta externa
  R2 = 22.;  // kohmi
  kv = 1.0288; // coeficient corectie tensiune
  ka = 1.014; // coeficient corectie curent
  vref = 2495 ;  // tensiune de referinta externa (2490mV) TL431A
  x0 = 19.8; // tensiune pe iesire operational, in gol, in mV
  x1 = 101.5;  // tensiune pentru un curent masurat, y1, in mV
  y0 = 0.;  // curent zero (0A)
  y1 = 383.;  // curent masurat, in mA
  di = 20.; // corectie curent, in mA
}
else  // Georgel
{
  analogReference(INTERNAL);  // referinta externa
  R2 = 56.;
  kv = 1.073; // coeficient corectie tensiune
  ka = 1.0; // coeficient corectie curent
  vref = 2560 ;  // tensiune de referinta interna (2.56V) ATmega32U4 si ATmega8
  x0 = 3; // tensiune pe iesire operational, in gol, in Volti
  x1 = 254.;  // tensiune in mV pentru un curent masurat, de ex. 1.0A
  y0 = 0;  // curent zero (0A)
  y1 = 1000.;  // curent masurat, in Amperi
  di = 20.; // corectie curent
}

 lcd.begin(16, 2);  // selectie afisaj 1602 (16 colane si 2 randuri)
 lcd.createChar(0, grad);  // crearea simbolului pentru grad Celsius
 lcd.createChar(1, cooler1);  // crearea simbolului pentru ventilator
 lcd.createChar(2, cooler2);  // crearea simbolului pentru ventilator
  
 lcd.clear();   // stergere ecran

pinMode(pinAN0, INPUT);  // pin definit ca intrare
pinMode(pinAN1, INPUT);  // pin definit ca intrare
pinMode(pinAN2, INPUT);  // pin definit ca intrare
pinMode(pinRST, INPUT);  // pin definit ca intrare
pinMode(pinREL, OUTPUT);  // pin definit ca iesire
pinMode(pinFAN, OUTPUT);  // pin definit ca iesire
pinMode(pinBUZ, OUTPUT);  // pin definit ca iesire
digitalWrite(pinRST, HIGH); // activare rezistenta de pull-up 
digitalWrite(pinREL, repaus); // releu necomandat
digitalWrite(pinBUZ, LOW);  // avertizor acustic oprit
analogWrite(pinFAN, 255); // ventilator pornit la maxim

  lcd.print("indicator panou");  
  lcd.setCursor(0, 1);
  lcd.print("tensiune-curent");
  delay (1500);
  lcd.clear();
  
  lcd.setCursor(3, 0);
  lcd.print("Umax = 55V");  
  lcd.setCursor(3, 1);
  lcd.print("Imax = 10A");
  delay (1500);
  lcd.clear();

  lcd.setCursor(1, 0);
  lcd.print("SW ver.0.c.0.f");  
  lcd.setCursor(2, 1);
  lcd.print("15.05.2020");
  delay (1500);
  lcd.clear();

  lcd.setCursor(2, 0);
  lcd.print("Deconectati");  
  lcd.setCursor(1, 1);
  lcd.print("orice sarcina!");
  delay (1500);
  lcd.clear();

  
analogWrite(pinFAN, 0); // ventilator oprit

digitalWrite(pinBUZ, HIGH);  // avertizor acustic pornit
delay(500);
digitalWrite(pinBUZ, LOW);  // avertizor acustic oprit

  m = (y1 - y0) / (x1 - x0); // calcul panta dreapta pentru calcul curent

  lcd.setCursor(0, 0);
  lcd.print("Autoconfigurare "); 
  
// eliminare offset
  sumacurent = 0;   // variabila pentru adunare masuratori pentru a obtone apoi media
  for (int i=1; i <= 20; i++)  // se fac 20 de masuratori
  {
  // citeste treptele de tensiune (0-1023) si adauga la o variabila de cumulare
  trcurent = analogRead(pinAN1);    //  citire valoare pe intrarea analogica 
  sumacurent = sumacurent + trcurent + 0.0;  // cumuleaza valoarea - http://www.skillbank.co.uk/arduino/measure.htm
  delay (20);  // pauza de 20ms intre masuratori
    }
// calculam valorile medii
sumacurent = sumacurent/20.;
// calculam valorile tensiunii si curentului de la sursa
tensiuneoa = (float)(ka*sumacurent * vref / 1024.0) ;
di = (double)(m * (tensiuneoa - x0) + y0);  // tensiune de offset (curent 0) 

 lcd.setCursor(0, 1);
 lcd.print("Offset: "); 
 lcd.print(di); 
 lcd.print("mV   "); 
 
// masuram curentul releului ce influenteaza masuratoarea
digitalWrite(pinREL, atras); // releu comandat
delay(1500);
  sumacurent = 0;   // variabila pentru adunare masuratori pentru a obtone apoi media
  for (int i=1; i <= 20; i++)  // se fac 20 de masuratori
  {
  // citeste treptele de tensiune (0-1023) si adauga la o variabila de cumulare
  trcurent = analogRead(pinAN1);    //  citire valoare pe intrarea analogica 
  sumacurent = sumacurent + trcurent + 0.0;  // cumuleaza valoarea - http://www.skillbank.co.uk/arduino/measure.htm
  delay (20);  // pauza de 20ms intre masuratori
    }
// calculam valorile medii
sumacurent = sumacurent/20.;
// calculam vvaloare curent ce infliuenteaza masura cand e releul pornit
//curentreleu = (float)(ka * sumacurent * vref/ 1024.0 / Amp /rsunt) ;

tensiuneoa1 = (float)(ka*sumacurent * vref / 1024.0) ;
curentreleu = (double)(m * (tensiuneoa1 - x0) + y0 -di);

 lcd.setCursor(0, 1);
 lcd.print("Corectie1: "); 
 lcd.print(curentreleu); 
// lcd.print("mA  ");

digitalWrite(pinREL, repaus); // releu necomandat
delay(1000);

// masuram curentul ventilatorului ce influenteaza masuratoarea
analogWrite(pinFAN, 255); // ventilator comandat la maxim
delay(500);
  sumacurent = 0;   // variabila pentru adunare masuratori pentru a obtone apoi media
  for (int i=1; i <= 20; i++)  // se fac 20 de masuratori
  {
  // citeste treptele de tensiune (0-1023) si adauga la o variabila de cumulare
  trcurent = analogRead(pinAN1);    //  citire valoare pe intrarea analogica 
  sumacurent = sumacurent + trcurent + 0.0;  // cumuleaza valoarea - http://www.skillbank.co.uk/arduino/measure.htm
  delay (20);  // pauza de 20ms intre masuratori
    }
// calculam valorile medii
sumacurent = sumacurent/20.;
// calculam vvaloare curent ce infliuenteaza masura cand e releul pornit
//curentventilator = (float)(ka * sumacurent * vref/ 1024.0 / Amp /rsunt) ;

tensiuneoa2 = (float)(ka*sumacurent * vref / 1024.0) ;
curentventilator = (double)(m * (tensiuneoa2 - x0) + y0 -di);

 lcd.setCursor(0, 1);
 lcd.print("Corectie2: "); 
 lcd.print(curentventilator); 
// lcd.print("mA  ");

analogWrite(pinFAN, 0); // ventilator oprit
delay(1000);

// masuram curentul ventilatorului la turatie medie ce influenteaza masuratoarea
rpm = (rpmmin + rpmmax)/2;
analogWrite(pinFAN, rpm); // ventilator comandat la maxim
delay(500);
  sumacurent = 0;   // variabila pentru adunare masuratori pentru a obtone apoi media
  for (int i=1; i <= 20; i++)  // se fac 20 de masuratori
  {
  // citeste treptele de tensiune (0-1023) si adauga la o variabila de cumulare
  trcurent = analogRead(pinAN1);    //  citire valoare pe intrarea analogica 
  sumacurent = sumacurent + trcurent + 0.0;  // cumuleaza valoarea - http://www.skillbank.co.uk/arduino/measure.htm
  delay (20);  // pauza de 20ms intre masuratori
    }
// calculam valorile medii
sumacurent = sumacurent/20.;
// calculam vvaloare curent ce infliuenteaza masura cand e releul pornit
//curentventilator = (float)(ka * sumacurent * vref/ 1024.0 / Amp /rsunt) ;

tensiuneoa4 = (float)(ka*sumacurent * vref / 1024.0) ;
curentventilator2 = (double)(m * (tensiuneoa4 - x0) + y0 -di);

 lcd.setCursor(0, 1);
 lcd.print("Corectie3: "); 
 lcd.print(curentventilator2); 
// lcd.print("mA  ");

analogWrite(pinFAN, 0); // ventilator oprit
delay(1000);

// masuram curentul releului si ventilatorului ce influenteaza masuratoarea
digitalWrite(pinREL, atras); // releu comandat
delay(500);
analogWrite(pinFAN, 255); // ventilator comandat la maxim
delay(1500);
  sumacurent = 0;   // variabila pentru adunare masuratori pentru a obtone apoi media
  for (int i=1; i <= 20; i++)  // se fac 20 de masuratori
  {
  // citeste treptele de tensiune (0-1023) si adauga la o variabila de cumulare
  trcurent = analogRead(pinAN1);    //  citire valoare pe intrarea analogica 
  sumacurent = sumacurent + trcurent + 0.0;  // cumuleaza valoarea - http://www.skillbank.co.uk/arduino/measure.htm
  delay (20);  // pauza de 20ms intre masuratori
    }
// calculam valorile medii
sumacurent = sumacurent/20.;
// calculam vvaloare curent ce infliuenteaza masura cand e releul pornit
//curentreleu = (float)(ka * sumacurent * vref/ 1024.0 / Amp /rsunt) ;

tensiuneoa3 = (float)(ka*sumacurent * vref / 1024.0) ;
curent2 = (double)(m * (tensiuneoa3 - x0) + y0 -di);

 lcd.setCursor(0, 1);
 lcd.print("Corectie4: "); 
 lcd.print(curent2); 
// lcd.print("mA  ");

analogWrite(pinFAN, 0); // ventilator oprit
delay(500);
digitalWrite(pinREL, repaus); // releu necomandat
delay(1000);

// masuram curentul releului si ventilatorului in PWM ce influenteaza masuratoarea
digitalWrite(pinREL, atras); // releu comandat
delay(500);
analogWrite(pinFAN, rpm); // ventilator comandat la turatie medie
delay(1500);
  sumacurent = 0;   // variabila pentru adunare masuratori pentru a obtone apoi media
  for (int i=1; i <= 20; i++)  // se fac 20 de masuratori
  {
  // citeste treptele de tensiune (0-1023) si adauga la o variabila de cumulare
  trcurent = analogRead(pinAN1);    //  citire valoare pe intrarea analogica 
  sumacurent = sumacurent + trcurent + 0.0;  // cumuleaza valoarea - http://www.skillbank.co.uk/arduino/measure.htm
  delay (20);  // pauza de 20ms intre masuratori
    }
// calculam valorile medii
sumacurent = sumacurent/20.;
// calculam vvaloare curent ce infliuenteaza masura cand e releul pornit
//curentreleu = (float)(ka * sumacurent * vref/ 1024.0 / Amp /rsunt) ;

tensiuneoa5 = (float)(ka*sumacurent * vref / 1024.0) ;
curent3 = (double)(m * (tensiuneoa5 - x0) + y0 -di);

 lcd.setCursor(0, 1);
 lcd.print("Corectie5: "); 
 lcd.print(curent3); 
// lcd.print("mA  ");

analogWrite(pinFAN, 0); // ventilator oprit
delay(500);
digitalWrite(pinREL, repaus); // releu necomandat
delay(1000);

  lcd.clear();

avarie = 0;
ventilator = 0;
releu = 0;
  
  lcd.setCursor(1, 0);
  lcd.print("Puteti conecta");  
  lcd.setCursor(3, 1);
  lcd.print("o sarcina!");
  delay (1500);
  lcd.clear();
  
  lcd.setCursor(8, 0);  // releu neatras
  lcd.print("!"); 
}

void loop() {
  // pune in zero variabilele de insumare pentru a calcula ulterior tensiunea medie
  sumatensiune = 0;
  sumacurent = 0;
     
  for (int i=1; i <= 20; i++)
  {
  // citeste treptele de tensiune (0-1023) si adauga la o variabila de cumulare
  trcurent = analogRead(pinAN1);    //  citire valoare pe intrarea analogica 
  sumacurent = sumacurent + trcurent + 0.0;  // cumuleaza valoarea - http://www.skillbank.co.uk/arduino/measure.htm

  trtensiune = analogRead(pinAN0); 
  sumatensiune = sumatensiune + trtensiune + 0.0; // http://www.skillbank.co.uk/arduino/measure.htm
  
  delay (20);  // pauza de 20ms intre masuratori
    }

// calculam valorile medii
sumacurent = sumacurent/20.;
sumatensiune = sumatensiune/20.;

// calculam valorile tensiunii si curentului de la sursa
//curent = (float)(ka * sumacurent * vref/ 1024.0 / Amp /rsunt) ;

tensiuneoa = (float)(ka*sumacurent * vref / 1024.0) ;
curent = (double)(m * (tensiuneoa - x0) + y0 -di);

if (curent < 5.) curent = 0;
if (curent < 0) curent = -curent;

if ((ventilator == 1) and (releu == 0)) curent = curent - curentventilator;  // doar ventilator la turatie maxima
if ((ventilator == 2) and (releu == 0)) curent = curent - curentventilator2;  // doar ventilator in PWM
if ((ventilator == 0) and (releu == 1)) curent = curent - curentreleu; // doar releu conectat
if ((ventilator == 1) and (releu == 1)) curent = curent - curent2;  // ventilator la maxim si releu conectat
if ((ventilator == 2) and (releu == 1)) curent = curent - curent3;  // ventilator in PWM si releu conectat

if (curent < 5.) curent = 0;
if (curent < 0) curent = -curent;
/*
//if ((ventilator) and (curent < curentcorectie))
if ((ventilator) and (tensiune > 0.1))
{
  curent = curent + curentcorectie;
}
if (curent < 0) curent = 0.; // daca curentul are valoare negativa se face 0
*/

tensiune = (float)(kv*(R1+R2)/R1 * sumatensiune * vref / 1024.0)/1000. ;
//tensiune = (float)(tensiune - curent * rsunt);  // facem media rezultatelor si scadem caderea de tensiune de pe rezistenta sunt
//if (tensiune < 0) tensiune = 0.; // daca tensiunea are valoare negativa se face 0
putere = (float)(tensiune * curent/1000.);   // putere consumata

// afisare valori
 lcd.setCursor(0, 0);
 if (tensiune < 10.0) lcd.print(" ");  // daca tensiunea e mai mica de 10V 
 lcd.print(tensiune);
 lcd.print("V ");
 
 lcd.setCursor(0, 1);

 if (curent < 20) curent = 0;  // eliminare erori masura
 cur1 = curent/10.;
 if (cur1 <= 1) cur1 = 0.;
/*
if (curent < 1000)
{
   if (cur1 < 1000.0) lcd.print(" ");  // daca curentul e mai mic de 1000mA 
   if (cur1 < 100.0) lcd.print(" ");  // daca curentul e mai mic de 100mA
   if (cur1 < 10.0) lcd.print(" ");  // daca curentul e mai mic de 10mA
   lcd.print(curent,0);
   lcd.print("mA ");
}
 else
 {
 if (cur1 < 10.0) lcd.print(" ");  // daca curentul e mai mic de 10A 
 lcd.print(cur1);
 lcd.print("A ");
 }
*/

cur2 = cur1/100;
cur3 = cur1 - cur2*100;

 if (cur2 < 10.0) lcd.print(" ");  // daca curentul e mai mic de 10A 
   lcd.print(cur2);
   lcd.print(".");
//   if (cur3 < 100) lcd.print("0");
   if (cur3 < 10) lcd.print("0");
   lcd.print(cur3);
   lcd.print("A");

 lcd.setCursor(10, 0);
 if (putere < 100.0) lcd.print(" ");  // daca putrea e mai mica de 100W  
 if (putere < 10.0) lcd.print(" ");  // daca putrea e mai mica de 10W 
 lcd.print(putere,1);  // afisare putere cu o singura cifra dupa virgula
 lcd.print("W"); 

if (senzorLM35)  // daca este senzor LM35
{  

if ((digitalRead(pinRST) == LOW) and (digitalRead(pinREL) == atras))
{
  digitalWrite(pinREL, repaus);  // decuplez bornele
  releu = 0;
  lcd.setCursor(8, 0);
  lcd.print("!"); 
  delay(1000); // o mica pauza
}
  
 sumatemp = 0;    
  for (int i=1; i <= 20; i++)
  {
  // citeste treptele de tensiune (0-1023) si adauga la o variabila de cumulare
  trtemp = analogRead(pinAN2);    //  citire valoare pe intrarea analogica 
  sumatemp = sumatemp + trtemp + 0.0;  // cumuleaza valoarea http://www.skillbank.co.uk/arduino/measure.htm
  delay (20);  // pauza de 20ms intre masuratori
    }
// calculam valorile medii
sumatemp = sumatemp/20.;
   te = (float)(0.1 * vref * sumatemp / 1024.0) ; // conversie in grade Celsius

//   te = 30;  // test
   
   durataapasare = duratamare; 
   durataapasare = aflareduratapasare(); // detectie apasare lunga

if ((durataapasare > duratamare) and (avarie == 0))  // daca butonul s-a apasat lung si temperatura ok
//if ((durataapasare > duratamare) and (ventilator == 0))  // daca butonul s-a apasat lung si ventilator oprit
//if ((durataapasare > duratamare) and (te < telim))  // daca butonul s-a apasat lung
//if ((digitalRead(pinRST) == LOW) and (te < telim))
{
  digitalWrite(pinREL, atras);  // cuplez bornele
  releu = 1;
  lcd.setCursor(8, 0);
  lcd.print("="); 
  delay(250); // o mica pauza
}

if ((digitalRead(pinRST) == LOW) and (digitalRead(pinREL) == atras))
{
  digitalWrite(pinREL, repaus);  // decuplez bornele
  releu = 0;
  lcd.setCursor(8, 0);
  lcd.print("!"); 
  delay(250); // o mica pauza
}

lcd.setCursor(11, 1);
if (te < 100.0) lcd.print(" ");  // daca tempratura e mai mica de 100 grade Celsius 
if (te < 10.0) lcd.print(" ");  // daca tempratura e mai mica de 10 grade Celsius 
lcd.print(te);  // afisam doar valoarea intreaga
//   lcd.write(0b11011111);  // caracter asemanatpor cu gradul Celsius
lcd.write(byte(0));  // simbolul de grad Celsius creat de mine
lcd.print("C"); 

if (te < temin)
{
  analogWrite(pinFAN, 0);  // ventilator oprit
  ventilator = 0;
}
else
if ((te>temin) and (te<=temax))
{
  
  rpm = map (te, temin, temax, rpmmin, rpmmax);  // calculeaza turatia variabila
  analogWrite(pinFAN, rpm);
  
//  analogWrite(pinFAN, rpmmax);  // ventilator laturatie maxima
  ventilator = 2;  // PWM mode
  avarie = 0;
}
else
if ((te>temax) and (te<=telim))
{
  analogWrite(pinFAN, rpmmax);  // ventilator laturatie maxima
  ventilator = 1;
}
else
if (te > telim)
{
  analogWrite(pinFAN, rpmmax);  // ventilator laturatie maxima
  ventilator = 1;
  avarie = 1;
  releu = 0;
  digitalWrite(pinREL, repaus);    // decuplare releu
  lcd.setCursor(8, 0);
  lcd.print("!"); 
  digitalWrite(pinBUZ, HIGH);   // avertizare ca este o problema
  delay(500);   // pauza mica
}  

if (curent > curentmax)  // depasire curent maxim
{
  digitalWrite(pinREL, repaus);    // decuplare releu
  releu = 0;
  lcd.setCursor(8, 0);
  lcd.print("!"); 
  digitalWrite(pinBUZ, HIGH);   // avertizare ca este o problema
  delay(500);   // pauza mica
}  

lcd.setCursor(8, 1);
if (ventilator == 1) 
{
int shou = millis()/1000;
if (shou %2 == 0)
lcd.write(byte(1));  // simbol de ventilator1
else 
lcd.write(byte(2));  // simbol de ventilator2
}
else
{
  lcd.print(" ");  // 
}
} // sfarsit conditie de existenta senzor temperatura

digitalWrite(pinBUZ, LOW);   // oprire avertizare
delay(50);   // pauza mica


}  // sfarsit de program principal


int aflareduratapasare () {  //  original ideea from Andy DOZ's blog - https://andydoz.blogspot.com
apasarebuton = digitalRead(pinRST);    // citiree stare buton
 if(apasarebuton == LOW && stare == false) {   // daca butonul e apasat, dar nu mai fusese apasat
   tpaparare = millis();
   stare = true;
   };
          
 if (apasarebuton == HIGH && stare == true) {  // daca butonul este neapasat si tocmai a fost eliberat
    tpeliberare = millis ();
    durataapasare = tpeliberare - tpaparare;
    stare = false;
    };
   return durataapasare;  // iesire din subrutina si memorare timp
}
