#include <18f4550.h>
#device ADC = 10
#fuses HS,NOWDT,NOPROTECT,NOPUT,NOLVP,BROWNOUT
#use delay(internal=4MHZ)                           
#use i2c(Master,Fast=100000, sda=PIN_C7, scl=PIN_C6,force_sw)  

#include "i2c_LCD.c"                                  // Libreria para el manejo del LCD
#include "Teclado.c"                                  // Libreria para el manejo del teclado matricial 4x4
#include "tonos_buzzer.c"                             // Libreria para el manejo de los Tonos del buzzer
#include <math.h>

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>


void BombDispense(){}
void BombAspiration(){}
void MotorTable(){}
void MotorAspiration(){}
void MeasureDistance(){}
void ControlValve(){}

void WashOperation(){}
void PirmeOperation(){}
void ShakeOperation(){}
