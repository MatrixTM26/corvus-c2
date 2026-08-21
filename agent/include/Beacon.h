#ifndef BeaconH
#define BeaconH

#include "Config.h"

int BeaconSend(const AgentConfig *C, const char *Uuid,
               const char *Payload, char *Response);

#endif
