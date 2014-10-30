#include "downloading.h"
#include <QTemporaryFile>
#include <QStringList>

Downloading::Downloading(): file(NULL), reply(NULL), busy(false) 
{
  mgr = new QNetworkAccessManager(this);
  connect(mgr, SIGNAL(finished(QNetworkReply*)), 
          this, SLOT(finished(QNetworkReply*)));
}

Downloading::~Downloading()
{
  if(file && file->isOpen()){
    file->close();
  }
  if(reply != NULL){
    delete reply;
  }
  delete mgr;
}

int Downloading::download(QString urlStr, QString dest)
{
  if(busy){
    return 2;
  };
  QUrl url(urlStr);
  destination = dest;
  origFname = url.path().split(QChar::fromLatin1('/')).last();
  fname = QString(QString::fromUtf8("%1/%2.XXXXXX")).arg(dest).arg(origFname);
  file = new QTemporaryFile(fname);
  if(!file->open(QIODevice::WriteOnly)){
    emit done(false, QString(QString::fromUtf8("Can't open file \"%1\"!")).arg(fname));
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

void Downloading::writeData()
{
  file->write(reply->readAll());
}

void Downloading::finished(QNetworkReply* reply)
{
  if(file && file->isOpen()){
    file->close();
  }
  if(reply->error() == QNetworkReply::NoError){
    QString result = QString(QString::fromUtf8("%1/%2")).arg(destination).arg(origFname);
    //std::cout<<"Renaming "<<qPrintable(fname)<<" to "
    //         <<qPrintable(result)<<std::endl;
    if(QFile::exists(result)){
      QFile::remove(result);
    }
    if(!QFile::rename(fname, result)){
      emit done(false, QString(QString::fromUtf8("Can't rename file \"%1\" to \"%2\"!")).arg(fname).arg(result));
    }
    emit msg(QString::fromUtf8("Download finished."));
    emit done(true, result);
  }else{
    QFile::remove(fname);
    emit msg(reply->errorString());
    emit done(false, reply->errorString());
  }
  busy = false;
}

void Downloading::progress(qint64 dl, qint64 all)
{
  if(all != 0){
    emit msg(dl, all);
  }
}

void Downloading::error(QNetworkReply::NetworkError e)
{
  (void) e;
}
