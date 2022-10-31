#include <18f4550.h>
#device ADC = 10
#fuses HS,NOWDT,NOPROTECT,NOPUT,NOLVP,BROWNOUT
#use delay(clock=4M)
#use i2c(Master,Fast=100000, sda=PIN_C7, scl=PIN_C6,force_sw)  

#include "i2c_LCD.c"                                  // Libreria para el manejo del LCD
#include "Teclado.c"                                  // Libreria para el manejo del teclado matricial 4x4
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
int contador=1;
int menu=0,opcion=0;

//enums
//wash
typedef enum EWashFormat { UMELISA, MICROELISA };
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
  struct TMenu *children[5];
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

int min(int, int);
int getMSB(int value);
int getLSB(int value);
int joinBytes(int msb, int lsb);


//menus
pMenu listenToKey(pMenu menu);
void printMenu(pMenu menu);
void scaffoldMenu(pMenu menu);

//operations
void washOp(void);
void formatWash(void);
void saveFormat(void);
void readFormat(void);

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
const char *WASH_PARAMS_FORMAT_MENU_LABEL = "F/Lavado:";
const char *WASH_PARAMS_TIME_MENU_LABEL = "T/Enjuague:";
const char *WASH_PARAMS_AMOUNT_MENU_LABEL = "C/Ciclos:";

const char *SHAKE_SHAKE_MENU_LABEL = "Agitar";
const char *SHAKE_MODE_MENU_LABEL = "Intensidad:";
const char *SHAKE_TIME_MENU_LABEL = "T/Agitacion:";

const char *PRIME_PRIME_MENU_LABEL = "Cebar";
const char *PRIME_MODE_MENU_LABEL = "M/Cebado:";
const char *PRIME_TIME_MENU_LABEL = "T/Cebado:";


const char *PROGRAM_CREATE_MENU_LABEL = "Crear";
const char *PRPGRAM_EDIT_MENU_LABEL = "Editar";
const char *PROGRAM_DELET_MENU_LABEL = "Borrar";

const char *PROGRAM_name_MENU_LABEL = "Nombre";

//configurations

//wash
byte WASH_FORMAT_ADDR[4] = {0x01, 0x0B, 0x10, 0x15};
byte RINSE_TIME_MSB_ADDR[4] = {0x02, 0x0C, 0x11, 0x16};
byte RINSE_TIME_LSB_ADDR[4] = {0x03, 0x0D, 0x12, 0x17};
byte CYCLES_COUNT_ADDR[4] = {0x04, 0x0E, 0x13, 0x18};
byte WATER_VOLUME_ADDR[4] = {0x0A, 0x0F, 0x14, 0x19};

//prime
byte PRIME_MODE_ADDR[4] = {0x05, 0x1A, 0x1C, 0x1E};
byte PRIME_TIME_ADDR[4] = {0x06, 0x1B, 0x1D, 0x1F};


//shake
byte SHAKE_INTENSITY_ADDR[4] = {0x07, 0x20, 0x23, 0x26};
byte SHAKE_TIME_MSB_ADDR[4] = {0x08, 0x21, 0x24, 0x27};
byte SHAKE_TIME_LSB_ADDR[4] = {0x09, 0x22, 0x25, 0x28};

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

void changeRinseTime(int newTime) {
   write_eeprom(RINSE_TIME_MSB_ADDR[currentProgram], getMSB(newTime));
   write_eeprom(RINSE_TIME_LSB_ADDR[currentProgram], getLSB(newTime));
}

int getRinseTime() {
   int msb = read_eeprom(RINSE_TIME_MSB_ADDR[currentProgram]);
   int lsb = read_eeprom(RINSE_TIME_LSB_ADDR[currentProgram]);
   return joinBytes(msb, lsb);
}

void changeCyclesCount(int count) {
   write_eeprom(CYCLES_COUNT_ADDR[currentProgram], count);
}

int readCyclesCount() {
   return read_eeprom(CYCLES_COUNT_ADDR[currentProgram]);
}

void changeWaterVolume(int newVolume) {
   write_eeprom(WATER_VOLUME_ADDR[currentProgram], newVolume);
}

int readWaterVolume() {
   return read_eeprom(WATER_VOLUME_ADDR[currentProgram]);
}

//prime
void changePrimeMode(EPrimeMode newMode) {
   write_eeprom(PRIME_MODE_ADDR[currentProgram], newMode);
}

EPrimeMode redPrimeMode() {
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
   write_eeprom(SHAKE_TIME_MSB_ADDR[currentProgram], getMSB(newTime));
   write_eeprom(SHAKE_TIME_LSB_ADDR[currentProgram], getLSB(newTime));
}

int readShakeTime() {
   int msb = read_eeprom(SHAKE_TIME_MSB_ADDR[currentProgram]);
   int lsb = read_eeprom(SHAKE_TIME_LSB_ADDR[currentProgram]);

   return joinBytes(msb, lsb);
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
void main()
{
   lcd_init(0x4E,20,4);              //Inicializa la pantalla
   lcd_backlight_led(ON);            //Enciende la luz de Fondo
   set_tris_b(0xF0);                 //RB0..RB3 salidas a teclado, RB4..RB7 entradas de teclado
   RB0=RB1=RB2=RB3=0;                //Esta l�nea y la siguiente son para detectar el cambio de estado RB0=RB1=RB2=RB3=0 a RB4=RB5=RB6=RB7=1;
   port_b_pullups(true);             //Activar las resistencias internas pull up para el puerto B donde esta conectado el teclado
   enable_interrupts ( INT_RB );            
   enable_interrupts ( GLOBAL ); 
   
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
   wash_format, wash_time, wash_amount, shake_shake, shake_mode,
   shake_time, prime_prime, prime_mode, prime_time, 
   program_create, program_edit, program_delet, program_name;
   
   
   
   initMenu(&main, MAIN_MENU_LABEL);
   initMenu(&wash, WASH_MENU_LABEL);
   initMenu(&prime, PRIME_MENU_LABEL);
   initMenu(&shake, SHAKE_MENU_LABEL);
   initMenu(&program, PROGRAM_MENU_LABEL);
   
   initMenu(&washWash, WASH_WASH_MENU_LABEL);
   initMenu(&washParams, WASH_PARAMS_MENU_LABEL);
   initMenu(&wash_format, WASH_PARAMS_FORMAT_MENU_LABEL);
   initMenu(&wash_time, WASH_PARAMS_TIME_MENU_LABEL);
   initMenu(&wash_amount, WASH_PARAMS_AMOUNT_MENU_LABEL);
   initMenu(&shake_shake, SHAKE_SHAKE_MENU_LABEL);
   initMenu(&shake_mode, SHAKE_MODE_MENU_LABEL);
   initMenu(&shake_time, SHAKE_TIME_MENU_LABEL);
   initMenu(&prime_prime, PRIME_PRIME_MENU_LABEL);
   initMenu(&prime_mode, PRIME_MODE_MENU_LABEL);
   initMenu(&prime_time, PRIME_TIME_MENU_LABEL);
   initMenu(&program_create, PROGRAM_CREATE_MENU_LABEL);
   initMenu(&program_edit, PRPGRAM_EDIT_MENU_LABEL);
   initMenu(&program_delet, PROGRAM_DELET_MENU_LABEL);
   initMenu(&program_name, PROGRAM_NAME_MENU_LABEL);


   
   addItem(&main, &wash);
   addItem(&main, &prime);
   addItem(&main, &shake);
   addItem(&main, &program);
   addItem(&wash, &washWash);
   addItem(&wash, &washParams);
   addItem(&washParams, &wash_format);
   addItem(&washParams, &wash_time);
   addItem(&washParams, &wash_amount);
   addItem(&shake, &shake_shake);
   addItem(&shake, &shake_mode);
   addItem(&shake, &shake_time);
   addItem(&prime, &prime_prime);
   addItem(&prime, &prime_mode);
   addItem(&prime, &prime_time);
   addItem(&program, &program_create);
   addItem(&program, &program_edit);
   addItem(&program, &program_delet);
   addItem(&program_create, &program_name);

   washWash.action = washOp;
   wash_format.action = formatWash;
   wash_time.action =  saveFormat;
   
   scaffoldMenu(&main);
}

/*************************************************
****************** DEFINITIONS ************************
**************************************************/

int getMSB(int value) {
   return (value & 0xFF00) >> 8;
}

int getLSB(int value) {
   return (value & 0x00FF);
}

int joinBytes(int msb, int lsb) {
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

//operations
void washOp(void) {
   lcd_putc("\f");
   lcd_gotoxy(2, 3);
   printf(lcd_putc, "Lavando ...");
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

void formatWash(void){
   EWashFormat x = readWashFormat();
   while(true){
   lcd_putc("\f");
   lcd_gotoxy(1, 1);
   printf(lcd_putc, WASH_PARAMS_MENU_LABEL);
    if (x == UMELISA){
    lcd_gotoxy(1, 2);
    printf(lcd_putc, "< UMELISA >");
   }
   else{
    lcd_gotoxy(1, 2);
    printf(lcd_putc, "< MICROELISA >");
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
