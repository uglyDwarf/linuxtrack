#include "macwebcam_info.h"

#include <assert.h>
#include <iostream>
#include <QProcess>
#include <QCoreApplication>

typedef struct{
  int x, y;
} resolution_t;

QList<resolution_t> resolutions;

resolution_t makeRes(int x, int y)
{
  resolution_t res;
  res.x = x;
  res.y = y;
  return res;
}

MacWebcamInfo::MacWebcamInfo(const QString &id)
{
  webcam_id = id;
  resolutions.clear();
  res_list.clear();
  resolutions.push_back(makeRes(160, 120));
  resolutions.push_back(makeRes(320, 240));
  resolutions.push_back(makeRes(352, 288));
  resolutions.push_back(makeRes(640, 480));
  
  QList<resolution_t>::iterator i;
  QString str;
  for(i = resolutions.begin(); i != resolutions.end(); ++i){
    str = QString("%1 x %2").arg(i->x).arg(i->y);
    res_list.push_back(str);
  }
}

const QStringList& MacWebcamInfo::getResolutions()
{
  return res_list;
}

bool MacWebcamInfo::decodeRes(const QString &res, int &res_x, int &res_y)
{
  const QRegExp &res_rexp = QRegExp("^\\s*(\\d+)\\s*[xX]\\s*(\\d+)\\s*$");
  if(res_rexp.indexIn(res) == -1){
    return false;
  }
  res_x = res_rexp.cap(1).toInt();
  res_y = res_rexp.cap(2).toInt();
  return true;
}

int MacWebcamInfo::findRes(const int &res_x, const int &res_y)
{
  QList<resolution_t>::const_iterator i;
  int counter = 0;
  for(i = resolutions.begin(); i != resolutions.end(); ++i, ++counter){
    if((i->x == res_x) && (i->y == res_y)){
      return counter;
    }
  }
  return 0;
}

MacWebcamInfo::~MacWebcamInfo()
{
}

QStringList& MacWebcamInfo::EnumerateWebcams()
{
  QStringList *res = new QStringList();
  QProcess *enum_proc = new QProcess();
  enum_proc->start(QCoreApplication::applicationDirPath() + "/../helper/qt_cam", QStringList() << "-e");
  enum_proc->waitForStarted(10000);
  enum_proc->waitForFinished(10000);
  QString str(enum_proc->readAllStandardOutput());
  *res = str.split("\n", QString::SkipEmptyParts);
  return *res;
}
