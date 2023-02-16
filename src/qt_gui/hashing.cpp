#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include "hashing.h"

bool hashFile(const QString fname, qint64 &size, uint16_t &fastVal, QByteArray &md5, 
              QByteArray &sha1)
{
  FastHash fh;
  QFile f(fname);
  if(!f.open(QIODevice::ReadOnly)){
    return false;
  }
  size = f.size();
  char val;
  uint16_t res;
  for(int i = 0; i < fh.getLength(); ++i){
    f.read(&val, 1);
    res = fh.hash(val);
  }
  fastVal = res;
  f.seek(0);
  QCryptographicHash ch_sha(QCryptographicHash::Sha1);
  ch_sha.addData(f.readAll());
  sha1 = ch_sha.result().toHex();

  f.seek(0);
  QCryptographicHash ch_md5(QCryptographicHash::Md5);
  ch_md5.addData(f.readAll());
  md5 = ch_md5.result().toHex();
  
  return true;
}

void BlockId::isBlock(QFile &f, const QString &destPath, QStringList &msgs)
{
  QString fname = QFileInfo(name).fileName();
  qint64 pos = f.pos();
  f.seek(pos - FastHash::length);
  QByteArray buf = f.read(size);
  if(buf.size() != size){
    goto no_match;
  }
  if(QCryptographicHash::hash(buf, QCryptographicHash::Md5).toHex() != md5){
    goto no_match;
  }
  if(QCryptographicHash::hash(buf, QCryptographicHash::Sha1).toHex() != sha1){
    goto no_match;
  }
  if(!found){
    QString outfile = destPath + fname;
    QFile out(outfile);
    found = out.open(QIODevice::WriteOnly);
    if(found){
      found = (out.write(buf) == size);
      if(found){
        msgs.append(QString::fromUtf8("Extracted %1...").arg(fname));
      }
    }
    out.close();
    if(fname.endsWith(QString::fromUtf8(".fw"), Qt::CaseInsensitive)){
      QStringList args;
      args << QStringLiteral("-9") << QStringLiteral("%1").arg(outfile);
      QProcess::execute(QStringLiteral("gzip"), args);
    }
  }
 no_match:
  f.seek(pos);
}

