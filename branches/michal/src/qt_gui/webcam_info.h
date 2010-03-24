#ifndef WEBCAM_INFO__H
#define WEBCAM_INFO__H

#include <QList>
#include <QString>
#include <QStringList>
#include "webcam_driver.h"


class WebcamInfo{
 public:
  WebcamInfo(const QString &id);
  const QStringList& getFormats();
  const QStringList& getResolutions(int index);
  QString getFourcc(int index);
  int findFourcc(const QString &fcc);
  ~WebcamInfo();
  static QStringList& EnumerateWebcams();
 private:
  QString webcam_id;
  webcam_formats fmts;
  typedef QList<QList<webcam_format*> > format_array_t;
  typedef QList<QStringList> res_list_t;
  QStringList format_strings;
  format_array_t fmt_descs;
  res_list_t res_list;
  int fmt_index;
};

#endif