#ifndef ROUTE_MGR_H
#define ROUTE_MGR_H

#include <stdint.h>

void Route_Init(void);
const char* Route_GetDirString(void);
uint16_t Route_GetNum(void);
void Route_Next(void);

#endif
