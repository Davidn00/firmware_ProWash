/******************************************************************************
Libreria para el manejo del Teclado Matricial 4x4                             *
Creado por David Nuñez Paredes                                                *
******************************************************************************/
// Conexiones del teclado.
/* 

           |---|---|---|---|
  RB4 ---> | 1 | 2 | 3 | A |
           |---|---|---|---|
  RB5 ---> | 4 | 5 | 6 | B |
           |---|---|---|---|
  RB6 ---> | 7 | 8 | 9 | C |
           |---|---|---|---|
  RB7 ---> | * | 0 | # | D |
           |---|---|---|---|
             ^   ^   ^   ^
             |   |   |   |
             RB0 RB1 RB2 RB3
*/

#byte puerto_tcl= 0xF81

#bit RB0=0xF81.0
#bit RB1=0xF81.1
#bit RB2=0xF81.2
#bit RB3=0xF81.3
#bit RB4=0xF81.4
#bit RB5=0xF81.5
#bit RB6=0xF81.6
#bit RB7=0xF81.7

char kbd_getc()
{
   char tecla;
   
     RB0=0;
     RB1=1;
     RB2=1;
     RB3=1; 
     if(puerto_tcl==0B11101110)tecla='1';
     if(puerto_tcl==0B11011110)tecla='4';
     if(puerto_tcl==0B10111110)tecla='7';                
     if(puerto_tcl==0B01111110)tecla='*';
     
     RB0=1;
     RB1=0;
     RB2=1;
     RB3=1; 
     if(puerto_tcl==0B11101101)tecla='2';
     if(puerto_tcl==0B11011101)tecla='5';
     if(puerto_tcl==0B10111101)tecla='8';                
     if(puerto_tcl==0B01111101)tecla='0';
     
     RB0=1;
     RB1=1;
     RB2=0;
     RB3=1; 
     if(puerto_tcl==0B11101011)tecla='3';
     if(puerto_tcl==0B11011011)tecla='6';
     if(puerto_tcl==0B10111011)tecla='9';                
     if(puerto_tcl==0B01111011)tecla='#';
     
     RB0=1;
     RB1=1;
     RB2=1;
     RB3=0; 
     if(puerto_tcl==0B11100111)tecla='A';
     if(puerto_tcl==0B11010111)tecla='B';
     if(puerto_tcl==0B10110111)tecla='C';                
     if(puerto_tcl==0B01110111)tecla='D';
     
   RB0=RB1=RB2=RB3=0;                                      
              
     return tecla;
}

