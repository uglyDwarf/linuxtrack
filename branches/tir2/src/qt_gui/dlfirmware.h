#ifndef DLFIRMWARE__H
#define DLFIRMWARE__H

#include <QObject>
#include <QTextStream>
#include <QtNetwork/QNetworkReply>
#include <QWidget>
#include <QProcess>
#include "ui_dl.h"

class QFile;
class QNetworkAccessManager;
class QProcess;
class dlfwGui;

class DLFirmware : public QObject
{
  Q_OBJECT
 public:
  DLFirmware();
  ~DLFirmware();
  int download(QString urlStr, QString dest);
 signals:
  void msg(QString message);
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

class dlfwGui : public QWidget{
  Q_OBJECT
 public:
  dlfwGui(QWidget *parent = 0);
  ~dlfwGui();
  void setMsg(QString msg);
 private slots:
  void on_DLButton_pressed();
  void on_FromFileButton_pressed();
  void msg(QString message);
  void done(bool ok, QString fileName);
  void unpack_finished(int exitCode, QProcess::ExitStatus exitStatus);
 signals:
  void finished(bool status);  
 private:
  bool unpackFirmware(QString fname, QString dest);
  QProcess *unpacker;
  QString destPath;
  QString basePath;
  Ui::DLFW ui;
  DLFirmware *dlfw;
};


#endif
