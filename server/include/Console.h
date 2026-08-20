#ifndef ConsoleH
#define ConsoleH

#include "Session.h"
#include "Config.h"

int  ConsoleRead(char *Out, int Cap);
int  ConsoleExec(const char *Line, SessionPool *P, const Config *C);
void ConsolePrintHelp(void);

#endif
