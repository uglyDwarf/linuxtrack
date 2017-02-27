#ifndef PREFS_LINK__H
#define PREFS_LINK__H

#include <QString>
#include <QMetaType>

typedef enum{
  NONE,
  WEBCAM,
  WEBCAM_FT,
  WIIMOTE,
  TIR,
  MACWEBCAM,
  MACWEBCAM_FT,
  JOYSTICK,
  MACPS3EYE
} deviceType_t;


struct PrefsLink{
  deviceType_t deviceType;
  QString ID;
  PrefsLink() : deviceType(NONE), ID(){};
  PrefsLink(const deviceType_t dt, const QString &i) : deviceType(dt), ID(i){};
};

Q_DECLARE_METATYPE(PrefsLink)


#endif
