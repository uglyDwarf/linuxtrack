/*
 * FreeTrackClient.dll
 *
 * Generated from FreeTrackClient.dll by winedump.
 *
 * DO NOT SEND GENERATED DLLS FOR INCLUSION INTO WINE !
 *
 */
#ifndef __WINE_FREETRACKCLIENT_DLL_H
#define __WINE_FREETRACKCLIENT_DLL_H

#include "windef.h"
#include "wine/debug.h"
#include "winbase.h"
#include "winnt.h"
#include <stdbool.h>


typedef struct
{
	unsigned int dataID;
	int res_x; int res_y;
	float yaw; // positive yaw to the left
	float pitch;// positive pitch up
	float roll;// positive roll to the left
	float x;
	float y;
	float z;
    // raw pose with no smoothing, sensitivity, response curve etc.
	float ryaw;
	float rpitch;
	float rroll;
	float rx;
	float ry;
	float rz;
    // raw points, sorted by Y, origin top left corner
	float x0, y0;
	float x1, y1;
	float x2, y2;
	float x3, y3;
}FreeTrackData;

char * __stdcall FREETRACKCLIENT_FTProvider(void);
char * __stdcall FREETRACKCLIENT_FTGetDllVersion(void);
void __stdcall FREETRACKCLIENT_FTReportName(char * name);
bool __stdcall FREETRACKCLIENT_FTGetData(FreeTrackData * data);



#endif	/* __WINE_FREETRACKCLIENT_DLL_H */
