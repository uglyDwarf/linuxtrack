#include <QFileDialog>
#include <QtDebug>
#include <QProcessEnvironment>
#include <QMessageBox>
#include <QDateTime>
#include <unistd.h>
#include <cstdlib>

#include "extractor.h"
#include "hashing.h"
#include "game_data.h"
#include "ltr_gui_prefs.h"

void ExtractThread::start(targets_t &t, const QString &p, const QString &d)
{
  if(!isRunning()){
    targets = &t;
    path = p;
    destPath = d;
    quit = false;
    QThread::start();
  }
}

void ExtractThread::run()
{
  emit progress(QString("Commencing analysis of directory '%1'...").arg(path));
  findCandidates(path);
  emit progress("===============================");
  if(allFound()){
    everything = true;
    emit progress(QString("Extraction done!"));
  }else{
    everything = false;
    for(targets_iterator_t it = targets->begin(); it != targets->end(); ++it){
      if(!it->second.foundAlready()) 
        emit progress(QString("Couldn't extract %1!").arg(it->second.getFname()));
    }
    if(!gameDataFound)
      emit progress(QString("Couldn't extract game data!"));
  }
  sleep(3);
}

void ExtractThread::analyzeFile(const QString fname)
{
  QFile file(fname);
  if(!file.open(QIODevice::ReadOnly)){
    return;
  }
  //qDebug()<<QString("Analyzing ")<<fname;
  FastHash hash;
  QStringList msgs;
  char val;
  uint16_t res;
  targets_iterator_t it;
  std::pair<targets_iterator_t,targets_iterator_t> range;
  int cntr = 0;
  while(file.read(&val, 1) > 0){
    ++cntr;
    res = hash.hash(val);
    range = targets->equal_range(res);
    for(it = range.first; it != range.second; ++it){
      //qDebug()<<cntr<<QString("Checking against ")<<file.pos() <<res <<(it->second.getFname());
      msgs.clear();
      it->second.isBlock(file, destPath, msgs);
      if(!msgs.isEmpty()){
        for(int i = 0; i < msgs.size(); ++i){
          emit progress(msgs[i]);
        }
      }
    }
  }
  file.close();
}

bool ExtractThread::allFound()
{
  for(targets_iterator_t it = targets->begin(); it != targets->end(); ++it){
    if(!it->second.foundAlready()) return false;
  }
  return gameDataFound;
}

bool ExtractThread::findCandidates(QString name)
{
  if(quit) return false;
  int i;
  QDir dir(name);
  QStringList patt;
  patt<<"*.dll"<<"*.exe"<<"*.dat";
  QFileInfoList files = dir.entryInfoList(patt, QDir::Files | QDir::Readable);
  for(i = 0; i < files.size(); ++i){
    if(files[i].fileName().compare("sgl.dat")){
      analyzeFile(files[i].canonicalFilePath());
    }else{
      QString outfile = QString("%1/gamedata.txt").arg(destPath);
      gameDataFound = get_game_data(qPrintable(files[i].canonicalFilePath()), qPrintable(outfile));
      emit progress(QString("Extracted game data..."));
    }
    if(allFound()){
      return true;
    }
  }
  
  QFileInfoList subdirs = 
    dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks); 
  for(i = 0; i < subdirs.size(); ++i){
    if(findCandidates(subdirs[i].canonicalFilePath())){
      return true;
    }
  }
  return false;
}

QString Extractor::findSrc(const QString &name)
{
  //First look at ~/.config/linuxtrack, then /usr/share/...
  QString path1 = PrefProxy::getRsrcDirPath();
  QString path2 = PrefProxy::getDataPath(name);
  path1 += name;
  QFileInfo fi(path1);
  if(fi.isReadable()) return path1;
  fi.setFile(path2);
  if(fi.isReadable()) return path2;
  return QString();
}

bool Extractor::readSources()
{
  progress("Looking for existing sources.txt...");
  QFile f(findSrc("sources.txt"));
  if(!f.open(QIODevice::ReadOnly)){
    progress("sources.txt not found.");
    return false;
  }
  progress(QString("Found '%1'.").arg(f.fileName()));
  
  QTextStream fs(&f);
  QString url;
  while(1){
    if(fs.atEnd()){
      break;
    }
    fs>>url;
    if(!url.isNull()){
      ui.FWCombo->addItem(url);
    }
  }
  progress("sources.txt found and read.");
  return (ui.FWCombo->count() != 0);
}


bool Extractor::readSpec()
{
  progress("Looking for existing spec.txt...");
  QFile f(findSrc("spec.txt"));
  if(!f.open(QIODevice::ReadOnly)){
    progress("spec.txt not found.");
    return false;
  }
  progress(QString("Found '%1'.").arg(f.fileName()));
  
  QTextStream fs(&f);
  QString name;
    uint16_t fh;
    qint64 size;
    QByteArray sha1, md5;
  while(1){
    if(fs.atEnd()){
      break;
    }
    fs>>name>>size>>fh>>md5>>sha1;
    if(!name.isNull()){
      BlockId blk(name, size, fh, md5, sha1);
      targets.insert(std::pair<uint16_t, BlockId>(fh, blk));
    }
  }
  progress("spec.txt found and read.");
  return (targets.size() != 0);
}

Extractor::Extractor(QWidget *parent) : QDialog(parent), et(NULL), dl(NULL), progressDlg(NULL)
{
  ui.setupUi(this);
  et = new ExtractThread();
  wine = new WineLauncher();
  dl = new Downloading();
  progressDlg = new Progress();
  QObject::connect(et, SIGNAL(progress(const QString &)), this, SLOT(progress(const QString &)));
  QObject::connect(et, SIGNAL(finished()), this, SLOT(threadFinished()));
  QObject::connect(wine, SIGNAL(finished(bool)), this, SLOT(wineFinished(bool)));
  QObject::connect(dl, SIGNAL(done(bool, QString)), this, SLOT(downloadDone(bool, QString)));
  QObject::connect(dl, SIGNAL(msg(const QString &)), this, SLOT(progress(const QString &)));
  QObject::connect(dl, SIGNAL(msg(qint64, qint64)), progressDlg, SLOT(message(qint64, qint64)));
  ui.BrowseButton->setEnabled(readSpec());
  readSources();
}

Extractor::~Extractor()
{
  delete et;
  et = NULL;
}

QString makeDestPath(const QString &base)
{
  QDateTime current = QDateTime::currentDateTime();
  QString result = QString("%2").arg(current.toString("yyMMdd_hhmmss"));
  QString final = result;
  QDir dir = QDir(base);
  int counter = 0;
  while(dir.exists(final)){
    final = QString("%1_%2").arg(result).arg(counter++);
  }
  dir.mkpath(final);
  return base + "/" + final + "/";
}


void Extractor::wineFinished(bool result)
{
  if(result){
    destPath = makeDestPath(PrefProxy::getRsrcDirPath());
    et->start(targets, winePrefix, destPath);
  }
  ui.BrowseButton->setEnabled(true);
}

void Extractor::extractFirmware(QString file)
{
  QMessageBox::information(this, "Instructions", 
  "NP's TrackIR installer will pop up now.\n\n"
  "Install it with all components to the default location, so the firmware and other necessary "
  "elements can be extracted.\n\n"
  "The software will be installed to the wine sandbox, that will be deleted afterwards, so "
  "there are no leftovers."
  );
  qDebug()<<winePrefix;
  progress(QString("Initializing wine and running installer %1").arg(file));
  wine->setEnv("WINEDLLOVERRIDES", "winemenubuilder.exe=d");
  wine->setEnv("WINEPREFIX", winePrefix);
  wine->run(file);
}


void Extractor::on_BrowseButton_pressed()
{
  ui.BrowseButton->setEnabled(false);
  QString dir = QFileDialog::getOpenFileName(this, "Open an installer:");
  if(dir.isEmpty()){
    return;
  }
  winePrefix = QDir::tempPath();
  winePrefix += "/wineXXXXXX";
  QByteArray charData = winePrefix.toUtf8();
  char *prefix = mkdtemp(charData.data()); 
  if(prefix == NULL){
    return;
  }
  winePrefix = prefix;
  //if(!QDir::current().mkdir("wine")){
  //  return;
  //}
  extractFirmware(dir);
}

void Extractor::on_AnalyzeSourceButton_pressed()
{
  targets.clear();
  QString dirName = QFileDialog::getExistingDirectory(this, "Open a wine directory:");
  if(dirName.isEmpty()){
    return;
  }
  QDir dir(dirName);
  QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Readable);
  FastHash hash;
  for(int i = 0; i < files.size(); ++i){
    uint16_t fh;
    qint64 size;
    QByteArray sha1, md5;
    hashFile(files[i].canonicalFilePath(), size, fh, md5, sha1);
    qDebug()<<files[i].fileName()<<QString(" ")<<size<<fh<<md5<<sha1;
    BlockId blk(files[i].fileName(), size, fh, md5, sha1);
    targets.insert(std::pair<uint16_t, BlockId>(fh, blk));
  }
  
  QFile of("spec.txt");
  if(!of.open(QFile::WriteOnly | QFile::Truncate)){
    return;
  }
  QTextStream out(&of);
 
  for(targets_iterator_t it = targets.begin(); it != targets.end(); ++it){
    it->second.save(out);
  }
  of.close();
  
}

void Extractor::progress(const QString &msg)
{
  ui.LogView->appendPlainText(msg);
}

void Extractor::threadFinished()
{
  ui.BrowseButton->setEnabled(true);
  bool everything = et->haveEverything();
  if(everything){
    QString l = PrefProxy::getRsrcDirPath() + "/tir_firmware";
      if(QFile::exists(l)){
        QFile::remove(l);
      }
    QFile::link(destPath, l);
  }
  emit finished(everything);
}


void Extractor::on_QuitButton_pressed()
{
  if(et->isRunning()){
    et->stop();
    et->wait();
  }
  hide();
  emit finished(et->haveEverything());
}


void Extractor::on_DownloadButton_pressed()
{
  QString target(ui.FWCombo->currentText());
  qDebug()<<QString("Going to download ")<<target;
  winePrefix = QDir::tempPath();
  winePrefix += "/wineXXXXXX";
  QByteArray charData = winePrefix.toUtf8();
  char *prefix = mkdtemp(charData.data()); 
  if(prefix == NULL){
    return;
  }
  winePrefix = prefix;
  progressDlg->show();
  progressDlg->raise();
  progressDlg->activateWindow();
  dl->download(target, winePrefix);
}


void Extractor::downloadDone(bool ok, QString fileName)
{
  progressDlg->hide();
  if(ok){
    progress("Downloading finished!");
    extractFirmware(fileName);
  }
}


/*
WINEDLLOVERRIDES=winemenubuilder.exe=d WINEPREFIX=/home/qbuilder/devel/research/extractor/test/TrackIR_5.2.Final wine TrackIR_5.2.Final.exe /s /v"/qb"
*/
