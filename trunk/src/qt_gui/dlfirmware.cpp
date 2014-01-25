#include "dlfirmware.h"
#include "ltr_gui_prefs.h"
#include <QTemporaryFile>
#include <QProcess>
#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>
//#include <QtNetwork>
#include <iostream>

DLFirmware::DLFirmware(): file(NULL), reply(NULL), busy(false) 
{
  mgr = new QNetworkAccessManager(this);
  connect(mgr, SIGNAL(finished(QNetworkReply*)), 
          this, SLOT(finished(QNetworkReply*)));
}

DLFirmware::~DLFirmware()
{
  if(file && file->isOpen()){
    file->close();
  }
  if(reply != NULL){
    delete reply;
  }
  delete mgr;
}

int DLFirmware::download(QString urlStr, QString dest)
{
  if(busy){
    return 2;
  };
  QUrl url(urlStr);
  destination = dest;
  origFname = url.path().split('/').last();
  fname = QString(fromUtf8("%1/%2.XXXXXX")).arg(dest).arg(origFname);
  file = new QTemporaryFile(fname);
  if(!file->open(QIODevice::WriteOnly)){
    emit done(false, QString(fromUtf8("Can't open file \"%1\"!")).arg(fname));
    return 1;
  }
  fname = file->fileName();
  reply = mgr->get(QNetworkRequest(url));
  busy = true;
  connect(reply, SIGNAL(downloadProgress(qint64, qint64)), 
          this, SLOT(progress(qint64, qint64)));
  connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), 
          this, SLOT(error(QNetworkReply::NetworkError)));
  connect(reply, SIGNAL(readyRead()), 
          this, SLOT(writeData()));
  return 0;
}

void DLFirmware::writeData()
{
  file->write(reply->readAll());
}

void DLFirmware::finished(QNetworkReply* reply)
{
  if(file && file->isOpen()){
    file->close();
  }
  if(reply->error() == QNetworkReply::NoError){
    QString result = QString(fromUtf8("%1/%2")).arg(destination).arg(origFname);
    //std::cout<<"Renaming "<<fname.toAscii().data()<<" to "
    //         <<result.toAscii().data()<<std::endl;
    if(QFile::exists(result)){
      QFile::remove(result);
    }
    if(!QFile::rename(fname, result)){
      emit done(false, QString(fromUtf8("Can't rename file \"%1\" to \"%2\"!")).arg(fname).arg(result));
    }
    emit msg(fromUtf8("Download finished."));
    emit done(true, result);
  }else{
    QFile::remove(fname);
    emit msg(reply->errorString());
    emit done(false, reply->errorString());
  }
  busy = false;
}

void DLFirmware::progress(qint64 dl, qint64 all)
{
  if(all != 0){
    QString message = QString(fromUtf8("Downloaded %1 of %2...")).arg(dl).arg(all);
    emit msg(message);
  }
}

void DLFirmware::error(QNetworkReply::NetworkError e)
{
  (void) e;
}


QString baseName(QString fileName)
{
  QFileInfo fi(fileName);
  return fi.baseName();
}

QString dirName(QString fileName)
{
  QFileInfo fi(fileName);
  return fi.absolutePath();
}

dlfwGui::dlfwGui(QWidget *parent):QWidget(parent)
{
  ui.setupUi(this);
  ui.DLLabel->setText(fromUtf8(""));
  ui.FileURL->setText(fromUtf8("http://media.naturalpoint.com/software/external/tir_firmware_110215.tbz"));
  dlfw = new DLFirmware();
  basePath = PrefProxy::getRsrcDirPath();
  connect(dlfw, SIGNAL(msg(QString)), 
          this, SLOT(msg(QString)));
  connect(dlfw, SIGNAL(done(bool, QString)), 
          this, SLOT(done(bool, QString)));
  unpacker = new QProcess();
  connect(unpacker, SIGNAL(finished(int, QProcess::ExitStatus)), 
          this, SLOT(unpack_finished(int, QProcess::ExitStatus)));
}

dlfwGui::~dlfwGui()
{
  delete dlfw;
  delete unpacker;
}

QString makeDestPath(QString base)
{
  QDateTime current = QDateTime::currentDateTime();
  QString result = QString(fromUtf8("%2")).arg(current.toString(fromUtf8("yyMMdd_hhmmss")));
  QString final = result;
  QDir dir = QDir(base);
  int counter = 0;
  while(dir.exists(final)){
    final = QString(fromUtf8("%1_%2")).arg(result).arg(counter++);
  }
  dir.mkpath(final);
  return base + final + fromUtf8("/");
}

void dlfwGui::msg(QString message)
{
  ui.DLLabel->setText(message);
}

bool dlfwGui::unpackFirmware(QString fname, QString dest)
{
  //and verify!!!
  QStringList params;
  params << fromUtf8("xfj") << fname;

  unpacker->setWorkingDirectory(dest);
  unpacker->start(fromUtf8("tar"), params);
  return true;
}

void dlfwGui::unpack_finished(int exitCode, QProcess::ExitStatus exitStatus)
{
  (void)exitStatus;
  if(exitCode == 0){
    QString l = basePath + fromUtf8("tir_firmware");
    if(QFile::exists(l)){
      QFile::remove(l);
    }
    QFile::link(destPath, l);
    emit finished(true);
  }else{
    QMessageBox::warning(this, fromUtf8("Problem unpacking firmware..."), 
                         unpacker->readAllStandardError());
    emit finished(false);
  }
}

void dlfwGui::on_DLButton_pressed()
{
  destPath = makeDestPath(basePath);
  dlfw->download(ui.FileURL->text(), destPath);
}

void dlfwGui::on_FromFileButton_pressed()
{
  QString fileName = QFileDialog::getOpenFileName(this,
     fromUtf8("Open Firmware package..."), fromUtf8("."), fromUtf8("Tar bzipped (*.tbz)"));
  if(!fileName.isEmpty()){
    destPath = makeDestPath(basePath);
    QFile::copy(fileName, destPath + baseName(fileName));
    unpackFirmware(fileName, destPath);
  }
}


void dlfwGui::done(bool ok, QString fileName)
{
  if(ok){
    ui.DLLabel->setText(fromUtf8("Finshed!"));
    QFileInfo fi(fileName);
    unpackFirmware(fi.absoluteFilePath(), fi.absolutePath());
  }else{
    QMessageBox::warning(this, fromUtf8("Problem downloading firmware..."), 
                         fileName);
    emit finished(false);
  }
}

