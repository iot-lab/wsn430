#include <stdio.h>
#include "uart0.h"

int
putchar(int c)
{
  uart0_putchar((char)c);
  return c;
}
