#ifndef ConsoleH
#define ConsoleH

#include "Session.h"
#include "Config.h"
#include "Log.h"

int  ConsoleRead(char *Out, int Cap);
int  ConsoleExec(const char *Line, SessionPool *P, const Config *C, LogStore *L);
void ConsolePrintHelp(void);

#endif
