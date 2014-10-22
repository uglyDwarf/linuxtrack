#ifndef DOWNLOADING__H
#define DOWNLOADING__H

#include <QObject>
#include <QString>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class Downloading : public QObject
{
  Q_OBJECT
 public:
  Downloading();
  ~Downloading();
  int download(QString urlStr, QString dest);
 signals:
  void msg(qint64 read, qint64 all);
  void msg(const QString &m);
  void done(bool ok, QString fileName);
 private:
  QFile* file;
  QString fname;
  QString destination;
  QString origFname;
  QNetworkAccessManager* mgr;
  QNetworkReply* reply;
  bool busy;
 private slots:
  void finished(QNetworkReply*);
  void progress(qint64, qint64);
  void writeData();
  void error(QNetworkReply::NetworkError);
};


#endif
