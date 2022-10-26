#include <18f4550.h>
#device ADC = 10
#fuses HS,NOWDT,NOPROTECT,NOPUT,NOLVP,BROWNOUT
#use delay(clock=4M)
#use i2c(Master,Fast=100000, sda=PIN_C7, scl=PIN_C6,force_sw)  



#include "i2c_LCD.c"                                  // Libreria para el manejo del LCD
#include "Teclado.c"                                  // Libreria para el manejo del teclado matricial 4x4
#include "tonos_buzzer.c"                             // Libreria para el manejo de los Tonos del buzzer
#include <math.h>


//#include <ncurses.h>

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

char K;
int segundos=0,minutos=0;
int contador=1, cont_submenu1=1,cont_submenu2=1;
int menu=0,opcion=0,hinicio=0,hfinal=0,garaje=0;



///**************************************************************************///
///******************** FUNCION MENU PRINCIPAL ******************************///
///*************************************************************************///
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
void main ()
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
   delay_ms(10);                    // Es 70ms
   }
   lcd_putc("\f");
   printf(lcd_putc,"BIENVENIDO...");
   lcd_gotoxy(1,2);
   printf(lcd_putc,"MW-2001");
   lcd_gotoxy(1,3);
   printf(lcd_putc,"Version Beta"); 
   delay_ms(500);                            // Es 1000ms para una espera de 4s
   lcd_putc("\f");
   menu_principal();

   WHILE(TRUE)
   {
      k=tecla();
      switch (k)
      {
      case '1':
               menu_principal();
               lcd_gotoxy(1,2);
               lcd_putc("*");
               menu=1;
               contador=0;
               contador=1;
      break;
      case '2':
               menu_principal();
               lcd_gotoxy(1,3);
               lcd_putc("*");
               menu=2;
               contador=0;
               contador=2;
      break;
      case '3':
               menu_principal();
               lcd_gotoxy(1,4);
               lcd_putc("*");
               menu=3;
               contador=0;
               contador=3;
      break;      
      case '4':
               lcd_putc("\f");
               lcd_gotoxy(1,1);
               lcd_putc("*4-PROGRAMAR");
               menu=4;
               contador=0;
               contador=4;
      break;
      case 'B':
               contador=contador+1;
                  if(contador==1)
                  {
                   menu_principal();
                   lcd_gotoxy(1,2);
                   lcd_putc("*");
                   menu=1;
                   }
                   if(contador==2)
                   {
                   menu_principal();
                   lcd_gotoxy(1,3);
                   lcd_putc("*");
                   menu=2;
                   }
                   if(contador==3)
                   {
                   menu_principal();
                   lcd_gotoxy(1,4);
                   lcd_putc("*");
                   menu=3;
                   }
                   if(contador==4)
                   {
                   lcd_putc("\f");
                   lcd_gotoxy(1,1);
                   lcd_putc("*4-PROGRAMAR");
                   menu=4;
                   contador=4;
                   }
       break;
       case 'A':
                 contador=contador-1;
                   if(contador==4)
                   {
                   menu_principal();
                   lcd_gotoxy(1,4);
                   lcd_putc("*");
                   menu=3;
                   contador==1;
                   }
                   if(contador==3)
                   {
                   menu_principal();
                   lcd_gotoxy(1,3);
                   lcd_putc("*");
                   menu=2;
                   }
                   if(contador==2)
                   {
                   menu_principal();
                   lcd_gotoxy(1,2);
                   lcd_putc("*");
                   menu=1;
                   }
       break;       
          }
///******************* Codigo para entrar a cada MENU ***********************///          
///**************************************************************************///          
        if ((K=='#')&&(menu==1))         // Si se selecciona el MENU LAVAR y pulsa Enter
            {
            menu_lavar();
            int esc=0;                    // Variable escape
            while(esc==0)                  // Si presiona esc sale del ciclo
            {
            k=kbd_getc();
               if(k=='*')
               {
               esc=1;
               menu_principal();
               contador=0;
               contador=1;
               }
               if(k=='1')
                  {
                  submenu_lavar1();
                  cont_submenu1=1;
                  char k1 = kbd_getc();
                        if(k=='A')
                        {
                        lcd_gotoxy(11,1);
                        lcd_putc("*UMMELISA*");
                        }
                        if(k=='B')
                        {
                        lcd_gotoxy(11,1);
                        lcd_putc("*MicMELISA*");
                        }

                     }
                if(k=='2')
                  {
                  submenu_lavar2();
                  cont_submenu1=2;
                  contador_s();
                  generate_tone(buzzer,5000, 100);
                  }
        }
       }       
///**************************************************************************///
            if ((K=='#')&&(menu==2))                                               // Si se selecciona el MENU CEBAR y pulsa Enter
               {
               menu_cebar();
               vselechora();
               int esc=0;
               while(esc==0)
                  {
                  k=kbd_getc();
                     if(k=='*')
                     {
                     esc=1;
                     menu_principal();
                     contador=0;
                     contador=2;
                     }
                  }
               } 
///**************************************************************************///
               if ((K=='#')&&(menu==3))                     // Si se selecciona el MENU AGITAR
                  {
                  menu_agitar();
                  int esc=0;
                    while(esc==0)
                    {
                    k=kbd_getc();
                    if (k=='*')
                    {
                    esc=1;
                    menu_principal();
                    contador=0;
                    contador=3;
                    }
                    }
                    }
 ///************************************************************************///                    
              if ((K=='#')&&(menu==4))                       // Si se selecciona el MENU PROGRAMAR
               {
               menu_programar();
               int esc=0;                                    // Variable escape
               while(esc==0)                                 // Si presiona esc sale del ciclo
               {
               k=kbd_getc();
                  if(k=='*')
                  {
                  esc=1;
                  menu_principal();
                  contador=0;
                  contador=4;
                   }
                   if(k=='1')
                  {
                  submenu_crear1();
                      if(k=='B')
                        {
                        submenu_crear2();
                        if(k=='*')
                              {
                              lcd_putc('\f');
                              menu_programar();
                              }
                        }
                     
                        }
                   }
              }
          
      }
   }
   
