#ifndef MACWEBCAM_INFO__H
#define MACWEBCAM_INFO__H

#include <QList>
#include <QString>
#include <QStringList>


class MacWebcamInfo{
 public:
  MacWebcamInfo(const QString &id);
  const QStringList& getResolutions();
  int findRes(const int &res_x, const int &res_y);
  static bool decodeRes(const QString &res, int &res_x, int &res_y);
  ~MacWebcamInfo();
  static QStringList& EnumerateWebcams();
 private:
  QString webcam_id;
  QStringList res_list;
};

#endif
