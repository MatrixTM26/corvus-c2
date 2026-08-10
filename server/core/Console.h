#ifndef CONSOLE_H
#define CONSOLE_H

#include "Session.h"

int  ConsoleRead(char *Out, int Cap);
int  ConsoleExec(const char *Line, Session *S);
void ConsolePrintHelp(void);

#endif
