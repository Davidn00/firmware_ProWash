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
int contador=1, cont_submenu1=1,cont_submenu2=1;
int menu=0,opcion=0,hinicio=0,hfinal=0,garaje=0;

const char *MAIN_MENU_LABEL = "Main Menu";
const char *WASH_MENU_LABEL = "Wash";
const char *SHAKE_MENU_LABEL = "Shake";
const char *PRIME_MENU_LABEL = "Prime";
const char *PROGRAM_MENU_LABEL = "Program";
const char *PROGRAM2_MENU_LABEL = "Program2";
const char *WASH_WASH_MENU_LABEL = "Inner Wash";
const char *WASH_PARAMS_MENU_LABEL = "Select Params";

typedef struct TMenu {
  char *label;
  struct TMenu *children[5];
  struct TMenu *parent;
  int childrenCount;
  void (*action) ();
  int currentMenu;
} TMenu;

typedef TMenu* pMenu;

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

//declarations
char tecla ();
pMenu listenToKey(pMenu menu);
void printMenu(pMenu menu);
int min(int, int);

///**************************************************************************///
///******************** FUNCION MENU PRINCIPAL ******************************///
///*************************************************************************///

int min(int a, int b) {
   return a < b ? a : b;
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

pMenu listenToKey(pMenu menu) {
   k = tecla();
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
      return menu->children[menu->currentMenu-1];
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


void menu_principal(void)
{
         LCD_PUTC("\f");
         lcd_gotoxy(3,1);
         printf (lcd_putc,"MENU PRINCIPAL");
         lcd_gotoxy(2,2);
         printf (lcd_putc,"1-LAVAR");               
         lcd_gotoxy(2,3);                   
         printf (lcd_putc,"2-CEBAR");               
         lcd_gotoxy(2,4);
         printf (lcd_putc,"3-AGITAR");
}
//**************************** MENU LAVAR **********************************///
///************************************************************************///
void menu_lavar (void)
{
         lcd_putc("\f");
         lcd_gotoxy(4,1);
         printf (lcd_putc,"MENU LAVAR");                
         lcd_gotoxy(1,2);
         printf (lcd_putc,"1-Selec Parametros");
         lcd_gotoxy(1,3);
         printf (lcd_putc,"2-Lavar");                
}
///**************************** MENU CEBAR **********************************///
///************************************************************************///
void menu_cebar (void)
{
         lcd_putc("\f");
         lcd_gotoxy(1,1);
         printf (lcd_putc,"MENU CEBAR");                  
         lcd_gotoxy(1,2);
         printf (lcd_putc,"1-M/cebado:");
         lcd_gotoxy(1,3);
         printf (lcd_putc,"2-T/cebado:");
         lcd_gotoxy(1,4);
         printf (lcd_putc,"3-Cebar:");
}
//**************************** MENU AGITAR **********************************///
///**************************************************************************///
void menu_agitar (void)
{
         lcd_putc("\f");
         lcd_gotoxy(1,1);
         printf (lcd_putc,"MENU AGITAR");
         lcd_gotoxy(1,2);
         printf (lcd_putc,"1-Intensidad:");
         lcd_gotoxy(1,3);
         printf (lcd_putc,"2-T/agitacion:");
         lcd_gotoxy(1,4);
         printf (lcd_putc,"3-Agitar");
}
//**************************** MENU PROGRAMAR *******************************///
///**************************************************************************///
void menu_programar (void)
{
            lcd_putc("\f");
            lcd_gotoxy(1,1);
            lcd_putc("MENU PROGRAMAR");
            lcd_gotoxy(1,2);
            lcd_putc("1-Crear");
            lcd_gotoxy(1,3);
            lcd_putc("2-Editar");
            lcd_gotoxy(1,4);
            lcd_putc("3-Borrar");
}
//******************************* SUBMENUS **********************************///
///**************************************************************************///
void submenu_lavar1 (void)
{
           lcd_putc("\f");
           lcd_gotoxy(1,1);
           lcd_putc("F/lavado:");
           lcd_gotoxy(1,2);
           lcd_putc("T/enjuague:");
           lcd_gotoxy(1,3);
           lcd_putc("Cant/ciclos:");
}
void submenu_lavar2()
{
           lcd_putc('\f');
           lcd_gotoxy(1,1);
           lcd_putc("Lavando...");
           lcd_gotoxy(1,3);
           lcd_putc("T/Lavado:");
}
void submenu_crear1 (void)
{
            lcd_putc("\f");
            lcd_gotoxy(1,1);
            lcd_putc("1-Nombre/Programa");
            lcd_gotoxy(1,2);
            lcd_putc("2-Formato/Lavado");
            lcd_gotoxy(1,3);
            lcd_putc("3-Volumen/Dispensar");
            lcd_gotoxy(1,4);
            lcd_putc("4-Tiemp/Enjuague");
}
void submenu_crear2(void)
{
            lcd_gotoxy(1,1);
            lcd_putc("5-Cant/Ciclos");
            lcd_gotoxy(1,2);
            lcd_putc("6-Tipo/Aspiración:");
}
///************************* FUNCION TECLA *********************************///
///************************************************************************///
char tecla ()
   {
   Char c;
   do{
   c=kbd_getc();
   }
   while(c=='\0');
   return(c);
}
///************************* FUNCION FLECHA ********************************///
///*************************************************************************///

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
      RB0=RB1=RB2=RB3=0;                      //Hay que anularlas tanto aquí como al final de Teclado3.c de lo contrario en la colúmna 1,6,7,* se activan los números al soltar la tecla ¿?                                      
   }
} 

void vselechora (void)
{
lcd_gotoxy(13,2);
printf (lcd_putc,"%dH ",hinicio);
lcd_gotoxy(13,3);
printf (lcd_putc,"%dH ",hfinal);
}

void config_T_enjuague (void)
{
      if((cont_submenu1==1)&&(k=='#'))
      {
      hinicio++;
      lcd_gotoxy(14,3);
      printf (lcd_putc,"%dH ",hinicio);
         if(hinicio==24)
         {
         hinicio=0;
         if(hfinal==0)
         {
         hinicio=0;
         if(hinicio==0)
         {
         lcd_gotoxy(14,3);
         printf (lcd_putc,"%dH ",hinicio);
         }
         }
         }
      }
         }

void config_F_lavado (void)
{
            if((cont_submenu1==1)&&(k=='#'))
            {
            int garaje=0;
            garaje=garaje+1;
               if(garaje==1)
              {
              lcd_gotoxy(11,1);
              lcd_putc("<UMELISA>");
              }
               if(garaje==2)
              {
              garaje=0;
              lcd_gotoxy(11,1);
              lcd_putc("<MicELISA>");
              }
            }
}

void config_F_lavado1(void)
{
         if (garaje==0)
         {
         lcd_gotoxy(11,1);
         lcd_putc("<UMELISA>");
         }
         if (garaje==1)
         {
         lcd_gotoxy(11,1);
         lcd_putc("<MicELISA>");
         }
        
}
void config_F_lavado2(void)
{
        if (garaje==1)
         {
         lcd_gotoxy(11,1);
         lcd_putc("<UMELISA>");
         }
         if (garaje==0)
         {
         lcd_gotoxy(11,1);
         lcd_putc("<MicELISA>");
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
   RB0=RB1=RB2=RB3=0;                //Esta línea y la siguiente son para detectar el cambio de estado RB0=RB1=RB2=RB3=0 a RB4=RB5=RB6=RB7=1;
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
   
   TMenu main, wash, prime, shake, program, program2, washWash, washParams;
   initMenu(&main, MAIN_MENU_LABEL);
   initMenu(&wash, WASH_MENU_LABEL);
   initMenu(&prime, PRIME_MENU_LABEL);
   initMenu(&shake, SHAKE_MENU_LABEL);
   initMenu(&washWash, WASH_WASH_MENU_LABEL);
   initMenu(&washParams, WASH_PARAMS_MENU_LABEL);
   initMenu(&program, PROGRAM_MENU_LABEL);
   initMenu(&program2, PROGRAM2_MENU_LABEL);

   
   addItem(&main, &wash);
   addItem(&main, &prime);
   addItem(&main, &shake);
   addItem(&main, &program);
   addItem(&main, &program2);
   addItem(&wash, &washWash);
   addItem(&wash, &washParams);
   
   scaffoldMenu(&main);
}

