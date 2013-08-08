/*
 * FreeTrackClient.dll
 *
 * Generated from FreeTrackClient.dll by winedump.
 *
 * DO NOT SUBMIT GENERATED DLLS FOR INCLUSION INTO WINE!
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define __WINESRC__
#include "windef.h"
#include "winbase.h"
#include "FreeTrackClient_dll.h"
#include "wine/debug.h"
#include <linuxtrack.h>

WINE_DEFAULT_DEBUG_CHANNEL(FreeTrackClient);


typedef enum {DBG_CHECK, DBG_ON, DBG_OFF} dbg_flag_type; 
static dbg_flag_type dbg_flag;

static dbg_flag_type get_dbg_flag(const int flag)
{
  char *dbg_flags = getenv("LINUXTRACK_DBG");
  if(dbg_flags == NULL) return DBG_OFF;
  if(strchr(dbg_flags, flag) != NULL){
    return DBG_ON;
  }else{
    return DBG_OFF;
  }
}



static void dbg_report(const char *msg,...)
{
  static FILE *f = NULL;
  if(dbg_flag == DBG_ON){
    if(f == NULL){
      f = fopen("FTClient.log", "w");
    }
    va_list ap;
    va_start(ap,msg);
    vfprintf(f, msg, ap);
    fflush(f);
    va_end(ap);
  }
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return TRUE;
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            dbg_flag = get_dbg_flag('w');
            dbg_report("Attach request\n");
            break;
        case DLL_PROCESS_DETACH:
            linuxtrack_shutdown();
            break;
    }

    return TRUE;
}
/******************************************************************
 *		FTProvider (FREETRACKCLIENT.1)
 *
 *
 */
char * __stdcall FREETRACKCLIENT_FTProvider(void)
{
	return "Freetrack";
}
/******************************************************************
 *		FTGetDllVersion (FREETRACKCLIENT.2)
 *
 *
 */
char * __stdcall FREETRACKCLIENT_FTGetDllVersion(void)
{
	return (char *) "1.0.0.266";
}
/******************************************************************
 *		FTReportName (FREETRACKCLIENT.3)
 *
 *
 */
void __stdcall FREETRACKCLIENT_FTReportName(char * name)
{
    linuxtrack_init(name);
}
/******************************************************************
 *		FTGetData (FREETRACKCLIENT.4)
 *
 *
 */
bool __stdcall FREETRACKCLIENT_FTGetData(FreeTrackData * data)
{
        pose_t pose;
        linuxtrack_get_pose_full(&pose);
        data->yaw = pose.yaw;
        data->pitch = pose.pitch;
        data->roll = pose.roll;
        data->x = pose.tx;
        data->y = pose.ty;
        data->z = pose.tz;
        
        data->ryaw = pose.raw_yaw;
        data->rpitch = pose.raw_pitch;
        data->rroll = pose.raw_roll;
        data->rx = pose.raw_tx;
        data->ry = pose.raw_ty;
        data->rz = pose.raw_tz;
        data->x0 = pose.points_x[0];
        data->y0 = pose.points_y[0];
        //should be sorted by Y
        if(pose.points_y[1] > pose.points_y[2]){
          data->x1 = pose.points_x[1];
          data->y1 = pose.points_y[1];
          data->x2 = pose.points_x[2];
          data->y2 = pose.points_y[2];
        }else{
          data->x1 = pose.points_x[2];
          data->y1 = pose.points_y[2];
          data->x2 = pose.points_x[1];
          data->y2 = pose.points_y[1];
        }
        data->x3 = data->y3 = 0.0;
        data->res_x = pose.resolution_x; data->res_y = pose.resolution_y;
        data->dataID = pose.counter;
	return TRUE;
}

/*
yaw +to the left
pitch + up
roll + to the left
x + to the left
y + up
z + back

Points sort by y;
*/
