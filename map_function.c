/* #include <map_function.c>
*
* Creada por: Ing. Abiezer Hernandez O.
* Fecha de creacion: 12/05/2018
* Electronica y Circuitos
*
*/

double map(float valor, float entradaMin, float entradaMax, float salidaMin, float salidaMax)
{
   return((((valor-entradaMin)*(salidaMax-salidaMin))/(entradaMax-entradaMin))+salidaMin);
}

unsigned long map16(int16 valor, int16 entradaMin, int16 entradaMax, int16 salidaMin, int16 salidaMax)
{
   return((((valor-entradaMin)*(salidaMax-salidaMin))/(entradaMax-entradaMin))+salidaMin);
}