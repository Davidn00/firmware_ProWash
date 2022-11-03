#include <18f4550.h>
#device ADC = 10
#fuses HS,NOWDT,NOPROTECT,NOPUT,NOLVP,BROWNOUT
//#use delay(clock=4M)
#use delay(internal=4MHZ)                           
#use i2c(Master,Fast=100000, sda=PIN_C7, scl=PIN_C6,force_sw)  

#include "i2c_LCD.c"                                  // Libreria para el manejo del LCD
#include "Teclado.c"                                  // Libreria para el manejo del teclado matricial 4x4
#include "Control_ls"
#include "tonos_buzzer.c"                             // Libreria para el manejo de los Tonos del buzzer
#include <math.h>

#define USB_HID_DEVICE  TRUE
#define USB_EP1_TX_ENABLE  USB_ENABLE_INTERRUPT     // Activar EP1 para transferencias masivas IN / interrupcion
#define USB_EP1_TX_SIZE    8
#define USB_EP1_RX_ENABLE  USB_ENABLE_INTERRUPT     // Activar EP1 para transferencias masivas OUT / interrupcion
#define USB_EP1_RX_SIZE    8
#include <pic18_usb.h>                              // Funciones de bajo nivel(hardware) para la serie PIC 18Fxx5x
#include <usb_desc_hid.h>                           // Libreria donde van las descripciones de este dispositivo
#include <usb.c> 

#byte PORTD=0XF83
#byte TRISE=0XF96
#byte PORTE=0xF84
#use standard_io(a) 
#define buzzer PIN_C2

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

char K;
int segundos=0,minutos=0;
int contador=1, menu=0;
int8 dataPC[8];
//enums
//wash
typedef enum EWashFormat { UMELISA, MiCROELISA };
typedef enum EWashMode {PLATE, STRIP};
typedef enum EWashAspiration {NORMAL, CRUSADE};
typedef enum EPrimeMode {CONTINUOUS, INTERMITTENT};
typedef enum EShakeIntensity { LOW, MEDIUM, HIGH};
typedef enum EOptionKey {OK, CANCEL, LEFT, RIGHT};

int currentProgram = 0;

typedef void (*_fptr)(void);

/*************************************************
***************** STRUCTURES *********************
**************************************************/

typedef struct TMenu {
  char *label;
  struct TMenu *children[6];
  struct TMenu *parent;
  int childrenCount;
  _fptr action;
  int currentMenu;
} TMenu;

typedef TMenu* pMenu;

/*************************************************
****************** DECLARATIONS ************************
**************************************************/
char read_key ();
void setup();
int min(int, int);
int getMSB(long value);
int getLSB(long value);
long joinBytes(int msb, int lsb);


//menus
pMenu listenToKey(pMenu menu);
void printMenu(pMenu menu);
void scaffoldMenu(pMenu menu);

//operations
void washOp(void);
void saveFormat(void);
void readFormat(void);
void washFormat(void);
void washMode(void);
void washTime(void);
void washAspiration(void);
void washVolume(void);
void washAmount(void);
void primeTime(void);
void primeMode(void);
void shakeTime(void);
void shakeIntensity(void);
void editProgram(void);
void editProgram1(void);
void editProgram2(void);
void editProgram3(void);

void resetConfig(void);
void resetConfig1(void);
void resetConfig2(void);
void resetConfig3(void);
void resetConfigAll(void);

EOptionKey getOptionKey ();

/*************************************************
****************** LABELS ************************
**************************************************/

const char *MAIN_MENU_LABEL = "MENU PRINCIPAL";
const char *WASH_MENU_LABEL = "LAVAR";
const char *SHAKE_MENU_LABEL = "AGITAR";
const char *PRIME_MENU_LABEL = "CEBAR";
const char *PROGRAM_MENU_LABEL = "PROGRAMAR";

const char *WASH_WASH_MENU_LABEL = "Lavar";
const char *WASH_PARAMS_MENU_LABEL = "Selec Parametro";

const char *WASH_WASHING_MENU_LABEL = "Lavando...";
const char *WASH_PARAMS_FORMAT_MENU_LABEL = "Format/Lavado:";
const char *WASH_PARAMS_MODE_MENU_LABEL = "Modo:";
const char *WASH_PARAMS_TIME_MENU_LABEL = "T/Enjuague:";
const char *WASH_PARAMS_ASPIRATION_MENU_LABEL = "Tipo/Aspiracion:";
const char *WASH_PARAMS_VOLUME_MENU_LABEL = "Vol/Dispensar:";
const char *WASH_PARAMS_AMOUNT_MENU_LABEL = "Cant/Ciclos:";

const char *SHAKE_SHAKE_MENU_LABEL = "Agitar";
const char *SHAKE_INTENSITY_MENU_LABEL = "Intensidad:";
const char *SHAKE_TIME_MENU_LABEL = "T/Agitacion:";

const char *PRIME_PRIME_MENU_LABEL = "Cebar";
const char *PRIME_MODE_MENU_LABEL = "M/Cebado:";
const char *PRIME_TIME_MENU_LABEL = "T/Cebado:";


const char *PROGRAM_RESET_MENU_LABEL = "Restablecer";
const char *PRPGRAM_EDIT_MENU_LABEL = "Editar";
const char *PROGRAM_DELET_MENU_LABEL = "Borrar";

const char *PROGRAM_NAME_PROGRAM1_MENU_LABEL = "Programa#1";
const char *PROGRAM_NAME_PROGRAM2_MENU_LABEL = "Programa#2";
const char *PROGRAM_NAME_PROGRAM3_MENU_LABEL = "Programa#3";

//configurations

//wash
byte WASH_FORMAT_ADDR[4] = {0x01, 0x08, 0x0F, 0x17};
byte WASH_MODE_ADDR[4] = {0x02, 0x09, 0x11, 0x18};
byte RINSE_TIME_MSB_ADDR[4] = {0x03, 0x0A, 0x12, 0x19};
byte RINSE_TIME_LSB_ADDR[4] = {0x04, 0x0B, 0x13, 0x1A};
byte WASH_ASPIRATION_ADDR[4] = {0x05, 0x0C, 0x14, 0x1B};
byte WATER_VOLUME_ADDR[4] = {0x06, 0x0D, 0x15, 0x1C};
byte CYCLES_COUNT_ADDR[4] = {0x07, 0x0E, 0x16, 0x1D};
 
//prime
byte PRIME_MODE_ADDR[4] = {0x1E, 0x20, 0x22, 0x24};
byte PRIME_TIME_ADDR[4] = {0x1F, 0x21, 0x23, 0x25};

//shake
byte SHAKE_INTENSITY_ADDR[4] = {0x26, 0x29, 0x3C, 0x3F};
byte SHAKE_TIME_ADDR[4] = {0x27, 0x3A, 0x3D, 0x40};


/*************************************************
*************** CONFIG METHODS *******************
**************************************************/
//wash
void changeWashFormat(EWashFormat newFormat) {
   write_eeprom(WASH_FORMAT_ADDR[currentProgram], newFormat);
}

EWashFormat readWashFormat() {
   return read_eeprom(WASH_FORMAT_ADDR[currentProgram]);
}

void changeWashMode(EWashMode newMode) {
   write_eeprom(WASH_MODE_ADDR[currentProgram], newMode);
}

EWashFormat readWashMode() {
   return read_eeprom(WASH_MODE_ADDR[currentProgram]);
}

void changeRinseTime(long newTime) {
   write_eeprom(RINSE_TIME_MSB_ADDR[currentProgram], getMSB(newTime));
   write_eeprom(RINSE_TIME_LSB_ADDR[currentProgram], getLSB(newTime));
}

long getRinseTime() {
   long msb = read_eeprom(RINSE_TIME_MSB_ADDR[currentProgram]);
   long lsb = read_eeprom(RINSE_TIME_LSB_ADDR[currentProgram]);
   return joinBytes(msb, lsb);
}

void changeWashAspiration(EWashAspiration newAspiration) {
   write_eeprom(WASH_ASPIRATION_ADDR[currentProgram], newAspiration);
}

EWashAspiration readWashAspiration() {
   return read_eeprom(WASH_ASPIRATION_ADDR[currentProgram]);
}

void changeWaterVolume(int newVolume) {
   write_eeprom(WATER_VOLUME_ADDR[currentProgram], newVolume);
}

int readWaterVolume() {
   return read_eeprom(WATER_VOLUME_ADDR[currentProgram]);
}

void changeCyclesCount(int count) {
   write_eeprom(CYCLES_COUNT_ADDR[currentProgram], count);
}

int readCyclesCount() {
   return read_eeprom(CYCLES_COUNT_ADDR[currentProgram]);
}

//prime
void changePrimeMode(EPrimeMode newMode) {
   write_eeprom(PRIME_MODE_ADDR[currentProgram], newMode);
}

EPrimeMode readPrimeMode() {
   return read_eeprom(PRIME_MODE_ADDR[currentProgram]);
}

void changePrimeTime(int newTime) {
   write_eeprom(PRIME_TIME_ADDR[currentProgram], newTime);
}

int readPrimeTime() {
   return read_eeprom(PRIME_TIME_ADDR[currentProgram]);
}

//shake
void changeShakeIntensity(EShakeIntensity intensity) {
   write_eeprom(SHAKE_INTENSITY_ADDR[currentProgram], intensity);
}

EShakeIntensity readShakeIntensity() {
   return read_eeprom(SHAKE_INTENSITY_ADDR[currentProgram]);
}

void changeShakeTime(int newTime) {
   write_eeprom(SHAKE_TIME_ADDR[currentProgram], newTime);
}

int readShakeTime() {
   return read_eeprom(SHAKE_TIME_ADDR[currentProgram]);
}

void switchProgram(int program) {
   if ( program > 3) {
      return;
   }
   currentProgram = program;
}

void initConfigProgram(int program) {
   switchProgram(program);

   // changeWashFormat(EWashFormat.UMELISA);
   changeWaterVolume(30);
   changeCyclesCount(4);
   changeRinseTime(30);

   // changePrimeMode(EPrimeMode.CONTINUOUS);
   changePrimeTime(2);

   changeShakeTime(30);
   // changeShakeIntensity(EShakeIntensity.MEDIUM);
}

void resetToDefault() {
   for (int i = 0; i < 4; i++) {
      initConfigProgram(i);
   }
}

void initMenu(pMenu item, char *label) {
   item->childrenCount = 0;
   item->parent = NULL;
   item->currentMenu = 0;
   item->action = NULL;
   item->label = label;
}

void addItem(pMenu parent, pMenu child) {
   if (parent->childrenCount >= 100) return;
   parent->children[parent->childrenCount] = child;
   parent->childrenCount++;
   child->parent = parent;
}

void labelMenu(pMenu item, char* label) {
  strcpy(item->label, label);
}

//methods to exec
void testMenu() {
  printf(lcd_putc,"menu activated\n");
}


///**************************************************************************///
///******************** FUNCION MENU PRINCIPAL ******************************///
///*************************************************************************///

int min(int a, int b) {
   return a < b ? a : b;
}


///************************* FUNCION TECLA *********************************///
///************************************************************************///
char read_key ()
   {
   Char c;
   do{
   c=kbd_getc();
   }
   while(c=='\0');
   return(c);
}

void tiempo (void)
{
    while(true)
      {
      lcd_gotoxy(12,3);
      printf(lcd_putc,"%02u",segundos);   // Muestro la variable sec en el lcd
      lcd_gotoxy(14,3);
      lcd_putc("seg");
      //lcd_gotoxy(5,2);
      //printf(lcd_putc,"%02u",minutos);  // Muestro la variable min en el lcd
      //lcd_gotoxy(7,2);
      //lcd_putc("min"); 
      setup_timer_0( RTCC_INTERNAL| RTCC_DIV_2);
      set_timer0(64786);
      enable_interrupts ( GLOBAL ); 
      enable_interrupts(INT_TIMER0); 
      }
}

#INT_TIMER0
void timer0_interrupcion()
{
  contador++;
   if(contador == 495)          // Contador para 1 segundo
    {
      segundos++;
      contador = 0;
    }
   if(segundos==60) 
    {
      minutos++;
      segundos= 0;
    }
   
  set_timer0(65036);
}

#INT_RB 
void TECLADO_INTERRUPT(void)
{
   delay_ms(15);                                  //Antirebotes
   
   if(!RB4 || !RB5 || !RB6 ||   !RB7)            //Para no leer el teclado al "soltar" la tecla
   {   
      k= kbd_getc();   
      RB0=RB1=RB2=RB3=0;                      //Hay que anularlas tanto aqu� como al final de Teclado3.c de lo contrario en la col�mna 1,6,7,* se activan los n�meros al soltar la tecla �?                                      
   }
} 

void contador_s ()                                 //contador de 1s
{
      do
      {
        for(int contador=0; contador<6; contador++)
        { 
        lcd_gotoxy(12,3);
        printf (lcd_putc,"%dSeg ",contador);
        delay_ms(255);
        }
      }while (contador==6);
}
///**************************************************************************///
///******************** FUNCION MENU PRINCIPAL ******************************///
///*************************************************************************///
void main(){
   setup_adc_ports(AN0);               // Configuracion del ADC
   setup_adc(adc_clock_internal);
   usb_init();
   usb_task();
   
   lcd_init(0x4E,20,4);              //Inicializa la pantalla
   lcd_backlight_led(ON);            //Enciende la luz de Fondo
   set_tris_b(0xF0);                 //RB0..RB3 salidas a teclado, RB4..RB7 entradas de teclado
   RB0=RB1=RB2=RB3=0;                //Esta l�nea y la siguiente son para detectar el cambio de estado RB0=RB1=RB2=RB3=0 a RB4=RB5=RB6=RB7=1;
   port_b_pullups(true);             //Activar las resistencias internas pull up para el puerto B donde esta conectado el teclado
   enable_interrupts ( INT_RB );            
   enable_interrupts ( GLOBAL ); 
         
   if (usb_enumerated())                                                         // Verifica si ha sido enumerado por el  PC 
      {
         delay_ms(1000);
         disable_interrupts(INT_TIMER0);
         generate_tone(PIN_C2,200,5);
         if (usb_kbhit(1)){                                                      // Verifica si ha recibido dato por USB
         usb_get_packet(1, dataPC, 1);                                           // Lee el valor del dato recibido por USB
         }
      }
   
   lcd_gotoxy(3,2);
   printf (lcd_putc,"CARGANDO SISTEMA");
   int i;
   for (i=3;i<=18;i++)
   {
   lcd_gotoxy(i,3);
   lcd_putc(".");
   //delay_ms(10);                    // Es 70ms
   }
   lcd_putc("\f");
   printf(lcd_putc,"BIENVENIDO...");
   lcd_gotoxy(1,2);
   printf(lcd_putc,"MW-2001");
   lcd_gotoxy(1,3);
   printf(lcd_putc,"Version Beta"); 
   //delay_ms(500);                            // Es 1000ms para una espera de 4s
   lcd_putc("\f");

   resetToDefault();
   
   TMenu main, wash, prime, shake, program, washWash, washParams, 
   wash_format, wash_mode, wash_time, wash_aspiration, wash_volume, 
   wash_amount, shake_shake, shake_intensity, shake_time, prime_prime, prime_mode, 
   prime_time, program_reset, program_edit, program_delet, program_name,
   name_program1, name_program2, name_program3, nameProgramReset1,
   nameProgramReset2, nameProgramReset3;
   
   
   initMenu(&main, MAIN_MENU_LABEL);
   initMenu(&wash, WASH_MENU_LABEL);
   initMenu(&prime, PRIME_MENU_LABEL);
   initMenu(&shake, SHAKE_MENU_LABEL);
   initMenu(&program, PROGRAM_MENU_LABEL);
   
   initMenu(&washWash, WASH_WASH_MENU_LABEL);
   initMenu(&washParams, WASH_PARAMS_MENU_LABEL);
   initMenu(&wash_format, WASH_PARAMS_FORMAT_MENU_LABEL);
   initMenu(&wash_mode, WASH_PARAMS_MODE_MENU_LABEL);
   initMenu(&wash_time, WASH_PARAMS_TIME_MENU_LABEL);
   initMenu(&wash_aspiration, WASH_PARAMS_ASPIRATION_MENU_LABEL);
   initMenu(&wash_volume, WASH_PARAMS_VOLUME_MENU_LABEL);
   initMenu(&wash_amount, WASH_PARAMS_AMOUNT_MENU_LABEL);
   
   initMenu(&shake_shake, SHAKE_SHAKE_MENU_LABEL);
   initMenu(&shake_intensity, SHAKE_INTENSITY_MENU_LABEL);
   initMenu(&shake_time, SHAKE_TIME_MENU_LABEL);
   
   initMenu(&prime_prime, PRIME_PRIME_MENU_LABEL);
   initMenu(&prime_mode, PRIME_MODE_MENU_LABEL);
   initMenu(&prime_time, PRIME_TIME_MENU_LABEL);
   
   
   initMenu(&program_edit, PRPGRAM_EDIT_MENU_LABEL);
   initMenu(&program_reset, PROGRAM_RESET_MENU_LABEL);
   
   initMenu(&name_program1, PROGRAM_NAME_PROGRAM1_MENU_LABEL);
   initMenu(&name_program2, PROGRAM_NAME_PROGRAM2_MENU_LABEL);
   initMenu(&name_program3, PROGRAM_NAME_PROGRAM3_MENU_LABEL);
   
   initMenu(&nameProgramReset1, PROGRAM_NAME_PROGRAM1_MENU_LABEL);
   initMenu(&nameProgramReset2, PROGRAM_NAME_PROGRAM2_MENU_LABEL);
   initMenu(&nameProgramReset3, PROGRAM_NAME_PROGRAM3_MENU_LABEL);


   
   addItem(&main, &wash);
   addItem(&main, &prime);
   addItem(&main, &shake);
   addItem(&main, &program);
   addItem(&wash, &washWash);
   addItem(&wash, &washParams);
   addItem(&washParams, &wash_format);
   addItem(&washParams, &wash_mode);
   addItem(&washParams, &wash_time);
   addItem(&washParams, &wash_aspiration);
   addItem(&washParams, &wash_volume);
   addItem(&washParams, &wash_amount);
   
   addItem(&shake, &shake_shake);
   addItem(&shake, &shake_intensity);
   addItem(&shake, &shake_time);
   addItem(&prime, &prime_prime);
   addItem(&prime, &prime_mode);
   addItem(&prime, &prime_time); 
   addItem(&program, &program_edit);
   addItem(&program, &program_reset);
   
   addItem(&program_edit, &name_program1);
   addItem(&program_edit, &name_program2);
   addItem(&program_edit, &name_program3);
   
   addItem(&program_reset, &nameProgramReset1);
   addItem(&program_reset, &nameProgramReset2);
   addItem(&program_reset, &nameProgramReset3);
   

   washWash.action = washOp;
   wash_format.action = washFormat;
   wash_mode.action = washMode;
   wash_time.action = washTime;
   wash_aspiration.action = washAspiration;
   wash_volume.action = washVolume;
   wash_amount.action = washAmount;
   prime_mode.action = primeMode;
   prime_time.action = primeTime;
   shake_intensity.action = shakeIntensity;
   shake_time.action = shakeTime;
   
   name_program1.action = editProgram1;
   name_program2.action = editProgram2;
   name_program3.action = editProgram3;
   
   nameProgramReset1.action = resetConfig1;
   nameProgramReset2.action = resetConfig2;
   nameProgramReset3.action = resetConfig3;

   setup();
   scaffoldMenu(&main);
}

/*************************************************
****************** DEFINITIONS ************************
**************************************************/
void setup() {

}

int getMSB(long value) {
   return (value & 0xFF00) >> 8;
}

int getLSB(long value) {
   return (value & 0x00FF);
}

long joinBytes(int msb, int lsb) {
   return (int)(msb << 8 | lsb);
}

EOptionKey getOptionKey (){
   while(true){
   char v = kbd_getc();
      switch(v) {
         case 'C':
         return RIGHT;
         break;
         case 'D': 
         return LEFT;
         break;
         case '*': 
         return CANCEL;
         break;
         case '#':
         return OK;
         break;
      }
   }
}

/**************** Operations with the program's parameter ********************/
//wash's parameter
void washOp(void) {
   lcd_putc("\f");
   lcd_gotoxy(2, 3);
   printf(lcd_putc, WASH_WASHING_MENU_LABEL);
   delay_ms(500);
   for( int i=0; i<5; i++)
   {
   generate_tone(PIN_C2, 500, 25);
   delay_ms(100);
   }
}

void saveFormat(void) { 
   EWashFormat z;
   if(rand()%2==0){
      z = UMELISA;
   }
   else{
      z = MICROELISA;
   }
   changeWashFormat(z);
}

void readFormat(void) {
  EWashFormat z = readWashFormat();
  printf(lcd_putc,"%d", z);
  delay_ms(250);
}

void washFormat(void){
   EWashFormat x = readWashFormat();
   while(true){
   lcd_putc("\f");
   lcd_gotoxy(1, 1);
   printf(lcd_putc, WASH_PARAMS_FORMAT_MENU_LABEL);
      if (x == UMELISA){
       lcd_gotoxy(1, 2);
       printf(lcd_putc, "< UMELISA >");
      }
      else{
       lcd_gotoxy(1, 2);
       printf(lcd_putc, "< MicroELISA >");
      }
    EOptionKey b = getOptionKey();
    switch(b) {
         case RIGHT:
         case LEFT:
         x = !x;
         break;
         case OK:
         changeWashFormat(x);
         case CANCEL: return;
      }
   }
}

void washMode(void){
   EWashMode x = readWashMode();
   while(true){
   lcd_putc("\f");
   lcd_gotoxy(1, 1);
   printf(lcd_putc, WASH_PARAMS_MODE_MENU_LABEL);
      if (x == PLATE){
       lcd_gotoxy(1, 2);
       printf(lcd_putc, "< Placa >");
      }
      else{
       lcd_gotoxy(1, 2);
       printf(lcd_putc, "< Tira >");
      }
    EOptionKey b = getOptionKey();
    switch(b) {
         case RIGHT:
         case LEFT:
         x = !x;
         break;
         case OK:
         changeWashMode(x);
         case CANCEL: return;
      }
   }
}

void washTime(void){
   long x = getRinseTime(); 
   while (true){
      delay_ms(150);
      lcd_putc("\f");
      lcd_gotoxy(1, 1);
      printf(lcd_putc, WASH_PARAMS_TIME_MENU_LABEL);
      lcd_gotoxy(1, 3);
      printf(lcd_putc,"%ld seg ",x);
      k = read_key ();
      char ks[2];
      ks[0] = (char)k;
      ks[1] = '\0';
      int val = atoi(ks);
         if(k=='A') {
         }
         else if(K=='B'){
         }
         else if(K=='C'){
         }
         else if(K=='D')
         {
         }
         else if(K=='*'){
         return;
         }
         else if(K=='#'){
         if(x > 0){
         changeRinseTime(x);
         return;
         }
         }
          else{
            x = x*10 + val;
               if (x >300){
               x=0;
               }
            }
      }
 }

void washAspiration(void){
   EWashAspiration s = readWashAspiration();
   while(true){
   lcd_putc("\f");
   lcd_gotoxy(1, 1);
   printf(lcd_putc, WASH_PARAMS_ASPIRATION_MENU_LABEL);
      if (s == NORMAL){
       lcd_gotoxy(1, 2);
       printf(lcd_putc, "< Normal >");
      }
      else{
       lcd_gotoxy(1, 2);
       printf(lcd_putc, "< Cruzada >");
      }
    EOptionKey b = getOptionKey();
    switch(b) {
         case RIGHT:
         case LEFT:
         s = !s;
         break;
         case OK:
         changeWashAspiration(s);
         case CANCEL: return;
      }
   }
}

void washVolume(void){
   long x = readWaterVolume(); 
   while (true){
      delay_ms(150);
      lcd_putc("\f");
      lcd_gotoxy(1, 1);
      printf(lcd_putc, WASH_PARAMS_VOLUME_MENU_LABEL);
      lcd_gotoxy(1, 3);
      printf(lcd_putc,"%ld ml",x);
      k = read_key ();
      char ks[2];
      ks[0] = (char)k;
      ks[1] = '\0';
      int val = atoi(ks);
         if(k=='A') {
         }
         else if(K=='B'){
         }
         else if(K=='C'){
         }
         else if(K=='D')
         {
         }
         else if(K=='*'){
         return;
         }
         else if(K=='#'){
         if(x > 0){
         changeWaterVolume(x);
         return;
         }
         }
          else{
            x = x*10 + val;
               if (x >450){
               x=0;
               }
            }
      }
 }

void washAmount(void){
   int x = readCyclesCount();
   while (true){
      delay_ms(150);
      lcd_putc("\f");
      lcd_gotoxy(1, 1);
      printf(lcd_putc, WASH_PARAMS_AMOUNT_MENU_LABEL);
      lcd_gotoxy(1, 3);
      printf(lcd_putc,"%d",x);
      k = read_key ();
      char ks[2];
      ks[0] = (char)k;
      ks[1] = '\0';
      int val = atoi(ks);
         if(k=='A') {
         }
         else if(K=='B'){
         }
         else if(K=='C'){
         }
         else if(K=='D')
         {
         }
         else if(K=='*'){
         return;
         }
         else if(K=='#'){
         if(x > 0){
         changeCyclesCount(x);
         return;
         }
         }
          else{
            x = x*10 + val;
               if (x >99){
               x=0;
               }
            }
      }
}
 
//prime's parameter
void primeMode(void){
   EPrimeMode x = readPrimeMode();
   while(true){
   lcd_putc("\f");
   lcd_gotoxy(1, 1);
   printf(lcd_putc, PRIME_MODE_MENU_LABEL);
      if (x == CONTINUOUS){
       lcd_gotoxy(1, 2);
       printf(lcd_putc, "< Continuo >");
      }
      else{
       lcd_gotoxy(1, 2);
       printf(lcd_putc, "< Intermitente >");
      }
    EOptionKey b = getOptionKey();
    switch(b) {
         case RIGHT:
         case LEFT:
         x = !x;
         break;
         case OK:
         changePrimeMode(x);
         case CANCEL: return;
      }
   }
}
void primeTime(void){
   int x = readPrimeTime();
   while (true){
      delay_ms(150);
      lcd_putc("\f");
      lcd_gotoxy(1, 1);
      printf(lcd_putc, PRIME_TIME_MENU_LABEL);
      lcd_gotoxy(1, 3);
      printf(lcd_putc,"%d min",x);
      k = read_key ();
      char ks[2];
      ks[0] = (char)k;
      ks[1] = '\0';
      int val = atoi(ks);
         if(k=='A') {
         }
         else if(K=='B'){
         }
         else if(K=='C'){
         }
         else if(K=='D')
         {
         }
         else if(K=='*'){
         return;
         }
         else if(K=='#'){
         if(x > 0){
         changePrimeTime(x);
         return;
         }
         }
          else{
            x = x*1 + val;
               if (x >10){
               x=0;
               }
            }
      }
}

//shake's parameter
void shakeIntensity(void){
   EShakeIntensity x = readShakeIntensity();
   while(true){
   delay_ms(50);
   lcd_putc("\f");
   lcd_gotoxy(1, 1);
   printf(lcd_putc, SHAKE_INTENSITY_MENU_LABEL);
      if (x == LOW){
       lcd_gotoxy(1, 2);
       printf(lcd_putc, "< Baja >");
      }
      if (x == MEDIUM) {
       lcd_gotoxy(1, 2);
       printf(lcd_putc, "< Media >");
      }
       if (x== HIGH) {
       lcd_gotoxy(1, 2);
       printf(lcd_putc, "< Alta >");
      }    
    EOptionKey b = getOptionKey();
    switch(b) {
         case RIGHT:  x = (x+1) %3;        
         break;
         case LEFT: x = (x - 1 + 3) % 3;
         break;
         case OK:
         changeShakeIntensity(x);
         case CANCEL: return;
      }
   }
}
void shakeTime(void){
   int x = readShakeTime();
   while (true){
      delay_ms(150);
      lcd_putc("\f");
      lcd_gotoxy(1, 1);
      printf(lcd_putc, SHAKE_TIME_MENU_LABEL);
      lcd_gotoxy(1, 3);
      printf(lcd_putc,"%d",x);
      k = read_key ();
      char ks[2];
      ks[0] = (char)k;
      ks[1] = '\0';
      int val = atoi(ks);
         if(k=='A') {
         }
         else if(K=='B'){
         }
         else if(K=='C'){
         }
         else if(K=='D')
         {
         }
         else if(K=='*'){
         return;
         }
         else if(K=='#'){
         if(x > 0){
         changeShakeTime(x);
         return;
         }
         }
          else{
            x = x*10 + val;
               if (x >60){
               x=0;
               }
            }
      }
}
//menu handlers
pMenu listenToKey(pMenu menu) {
   k = read_key ();
   char ks[2];
   ks[0] = (char)k;
   ks[1] = '\0';
   int val = atoi(ks);
   if (val && menu->childrenCount >= val) {
      menu->currentMenu = val;
   } else if (k == '*' && menu->parent != NULL) {
      return menu->parent;
   } else if (k == 'A' && menu->currentMenu > 1) {
      menu->currentMenu--;
   } else if (k == 'B' && menu->currentMenu < menu->childrenCount) {
      menu->currentMenu++;
   } else if (k == '#' && menu->currentMenu > 0) {
      pMenu child = menu->children[menu->currentMenu -1];
      if (child->action != NULL) {
         _fptr toAct = child->action;
         (*toAct)();
         return menu;
      }
      return child;
   }
   return menu;
}

void scaffoldMenu(pMenu menu) {
   pMenu nextMenu = menu;
   while(true) {
      printMenu(nextMenu);
      delay_ms(10);
      nextMenu = listenToKey(nextMenu);
   }
}

void printMenu(pMenu menu) {
   LCD_PUTC("\f");
   lcd_gotoxy(3, 1);
   printf(lcd_putc, menu->label);
   int start = 0;
   int cMenu = menu->currentMenu;
   int offsets[3] = {2, 0, 1};
   int offset = offsets[cMenu % 3];
   if (cMenu > 3) {
      start = cMenu - offset - 1;
   }
   
   int menusToShow = min(menu->childrenCount - start, 3);
   
   for (int i = 0; i < menusToShow; i++) {
      pMenu child = menu->children[i+start];
      lcd_gotoxy(2, i+2);
      printf(lcd_putc, "%d", start + i +1);
      lcd_gotoxy(3, i+2);
      printf(lcd_putc, "-");
      lcd_gotoxy(4, i+2);
      printf(lcd_putc, child->label);
   }
   if (menu->currentMenu > 0) {
      lcd_gotoxy(1, offset+2);
      printf(lcd_putc, "*");
   }
}

void editProgram(){
   washFormat();
   washMode();
   washTime();
   washAspiration();
   washVolume();
   washAmount();
}
void editProgram1(){
   currentProgram = 1;
   editProgram();
}
 
void editProgram2(){
   currentProgram = 2;
   editProgram();
}
void editProgram3(){
   currentProgram = 3;
   editProgram();
}
void resetConfig(){
   changeWashFormat(UMELISA);
   changeWashMode(PLATE);
   changeRinseTime(30);
   changeWashAspiration(NORMAL);
   changeWaterVolume(30);
   changeCyclesCount(4);
   changePrimeMode(CONTINUOUS);
   changePrimeTime(2);
   changeShakeIntensity(MEDIUM);
   changeShakeTime(30);  
}
void resetConfig1(){
   currentProgram = 1;
   resetConfig();
}
void resetConfig2(){
   currentProgram = 2;
   resetConfig();
}
void resetConfig3(){
   currentProgram = 3;
   resetConfig();
}
void resetConfigAll(){
   for(currentProgram=0; currentProgram<4; currentProgram++){
   resetConfig();
   }
}
