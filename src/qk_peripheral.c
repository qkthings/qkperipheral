/*
 * QkThings LICENSE
 * The open source framework and modular platform for smart devices.
 * Copyright (C) 2014 <http://qkthings.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qk_peripheral.h"
#include "qk_peripheral_p.h"

void qk_peripheral_setup(void)
{
	_qk_mcu_startup();

	_qk_gpio_startup();
	_qk_gpio_init();

	_qk_uart_startup();
	_qk_uart_init();

	_qk_vtimer_startup();

	_qk_timer_startup();
	_qk_timer_init();

#ifdef QK_ADC
	_qk_adc_startup();
	_qk_adc_init();
#endif

#ifdef QK_PWM
	_qk_pwm_startup();
#endif

#ifdef QK_I2C
	_qk_i2c_startup();
#endif

#ifdef QK_SPI
	_qk_spi_startup();
#endif

	_qk_mcu_finalize();
}

// ------------------------------------------------------------------
// Lightweight STDIO functions
// Source: http://www.menie.org/georges/embedded/

static void printchar(char **str, int c)
{
  if (str) {
    **str = c;
    ++(*str);
  }
  else {
    qk_uart_write(_QK_PROGRAM_UART, (uint8_t*) &c, 1);
    if(c == '\n')
    {
      c = '\r';
      qk_uart_write(_QK_PROGRAM_UART, (uint8_t*) &c, 1);
    }
  }
}

#define PAD_RIGHT 1
#define PAD_ZERO 2

static int prints(char **out, const char *string, int width, int pad)
{
  register int pc = 0, padchar = ' ';

  if (width > 0) {
    register int len = 0;
    register const char *ptr;
    for (ptr = string; *ptr; ++ptr) ++len;
    if (len >= width) width = 0;
    else width -= len;
    if (pad & PAD_ZERO) padchar = '0';
  }
  if (!(pad & PAD_RIGHT)) {
    for ( ; width > 0; --width) {
      printchar (out, padchar);
      ++pc;
    }
  }
  for ( ; *string ; ++string) {
    printchar (out, *string);
    ++pc;
  }
  for ( ; width > 0; --width) {
    printchar (out, padchar);
    ++pc;
  }

  return pc;
}

// the following should be enough for 32 bit int
#define PRINT_BUF_LEN 12

static int printi(char **out, int i, int b, int sg, int width, int pad, int letbase)
{
  char print_buf[PRINT_BUF_LEN];
  register char *s;
  register int t, neg = 0, pc = 0;
  register unsigned int u = i;

  if (i == 0) {
    print_buf[0] = '0';
    print_buf[1] = '\0';
    return prints (out, print_buf, width, pad);
  }

  if (sg && b == 10 && i < 0) {
    neg = 1;
    u = -i;
  }

  s = print_buf + PRINT_BUF_LEN-1;
  *s = '\0';

  while (u) {
    t = u % b;
    if( t >= 10 )
      t += letbase - '0' - 10;
    *--s = t + '0';
    u /= b;
  }

  if (neg) {
    if( width && (pad & PAD_ZERO) ) {
      printchar (out, '-');
      ++pc;
      --width;
    }
    else {
      *--s = '-';
    }
  }

  return pc + prints (out, s, width, pad);
}

static int print(char **out, int *varg)
{
  register int width, pad;
  register int pc = 0;
  register char *format = (char *)(*varg++);
  char scr[2];

  for (; *format != 0; ++format) {
    if (*format == '%') {
      ++format;
      width = pad = 0;
      if (*format == '\0') break;
      if (*format == '%') goto out;
      if (*format == '-') {
        ++format;
        pad = PAD_RIGHT;
      }
      while (*format == '0') {
        ++format;
        pad |= PAD_ZERO;
      }
      for ( ; *format >= '0' && *format <= '9'; ++format) {
        width *= 10;
        width += *format - '0';
      }
      if( *format == 's' ) {
        register char *s = *((char **)varg++);
        pc += prints (out, s?s:"(null)", width, pad);
        continue;
      }
      if( *format == 'd' ) {
        pc += printi (out, *varg++, 10, 1, width, pad, 'a');
        continue;
      }
      if( *format == 'x' ) {
        pc += printi (out, *varg++, 16, 0, width, pad, 'a');
        continue;
      }
      if( *format == 'X' ) {
        pc += printi (out, *varg++, 16, 0, width, pad, 'A');
        continue;
      }
      if( *format == 'u' ) {
        pc += printi (out, *varg++, 10, 0, width, pad, 'a');
        continue;
      }
      if( *format == 'c' ) {
        // char are converted to int then pushed on the stack
        scr[0] = *varg++;
        scr[1] = '\0';
        pc += prints (out, scr, width, pad);
        continue;
      }
    }
    else {
    out:
      printchar (out, *format);
      ++pc;
    }
  }
  if (out) **out = '\0';
  return pc;
}

// assuming sizeof(void *) == sizeof(int)

int printf(const char *format, ...)
{
  register int *varg = (int *)(&format);
  return print(0, varg);
}

int sprintf(char *str, const char *format, ...)
{
  register int *varg = (int *)(&format);
  return print(&str, varg);
}


