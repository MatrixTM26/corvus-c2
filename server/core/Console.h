#ifndef CONSOLE_H
#define CONSOLE_H

#include "Session.h"

int  ConsoleReadLine(char *Out, int Cap);
int  ConsoleHandleInput(const char *Input, AgentSession *Session);
void ConsolePrintHelp(void);
void ConsoleClear(void);

#endif
