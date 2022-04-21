#ifndef __CONSOLE_H__
#define __CONSOLE_H__
#include "types.h"
#include "spinlock.h"
#define BACKSPACE 0x100
struct console {
  struct spinlock lock;
  
  // input
#define INPUT_BUF 128
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
};
void consputc(int c);
void consoleinit(void);
#endif
