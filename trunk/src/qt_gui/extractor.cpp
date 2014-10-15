#include <QFileDialog>
#include <QtDebug>
#include <QProcessEnvironment>
#include <QMessageBox>
#include <QDateTime>
#include <QTextStream>
#include <unistd.h>
#include <cstdlib>

#include "extractor.h"
#include "hashing.h"
#include "game_data.h"
#include "ltr_gui_prefs.h"
#include "help_view.h"

#ifdef HAVE_CONFIG_H
  #include "../../config.h"
#endif

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
  emit progress(QString::fromUtf8("Commencing analysis of directory '%1'...").arg(path));
  gameDataFound = false;
  tirviewsFound = false;
  for(targets_iterator_t it = targets->begin(); it != targets->end(); ++it){
    it->second.clearFoundFlag();
  }

  findCandidates(path);
  emit progress(QString::fromUtf8("==============================="));
  if(allFound()){
    everything = true;
    emit progress(QString::fromUtf8("Extraction done!"));
  }else{
    everything = false;
    for(targets_iterator_t it = targets->begin(); it != targets->end(); ++it){
      if(!it->second.foundAlready())
        emit progress(QString::fromUtf8("Couldn't extract %1!").arg(it->second.getFname()));
    }
    if(!gameDataFound){
      emit progress(QString::fromUtf8("Couldn't extract game data!"));
    }
    if(!tirviewsFound){
      emit progress(QString::fromUtf8("Couldn't extract TIRViews.dll!"));
    }
  }
}

void ExtractThread::analyzeFile(const QString fname)
{
  QFile file(fname);
  if(!file.open(QIODevice::ReadOnly)){
    return;
  }
  qDebug()<<QString::fromUtf8("Analyzing ")<<fname;
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
      //qDebug()<<cntr<<qPrintable("Checking against ")<<file.pos() <<res <<(it->second.getFname());
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
  return gameDataFound && tirviewsFound;
}

bool ExtractThread::findCandidates(QString name)
{
  if(quit) return false;
  int i;
  QDir dir(name);
  QStringList patt;
  patt<<QString::fromUtf8("*.dll")<<QString::fromUtf8("*.exe")<<QString::fromUtf8("*.dat");
  QFileInfoList files = dir.entryInfoList(patt, QDir::Files | QDir::Readable);
  for(i = 0; i < files.size(); ++i){
    if(quit) return false;
    if(files[i].fileName().compare(QString::fromUtf8("TIRViews.dll")) == 0){
      QString outfile = QString::fromUtf8("%1/TIRViews.dll").arg(destPath);
      if((tirviewsFound = QFile::copy(files[i].canonicalFilePath(), outfile))){
        emit progress(QString::fromUtf8("Extracted TIRViews.dll..."));
      }
    }else if(files[i].fileName().compare(QString::fromUtf8("sgl.dat"))){
      analyzeFile(files[i].canonicalFilePath());
    }else{
      QString outfile = QString::fromUtf8("%1/gamedata.txt").arg(destPath);
      gameDataFound = get_game_data(files[i].canonicalFilePath().toUtf8().constData(),
                                    outfile.toUtf8().constData(), false);
      emit progress(QString::fromUtf8("Extracted game data..."));
    }
    if(allFound()){
      return true;
    }
  }

  QFileInfoList subdirs =
    dir.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
  QString dirname;
  for(i = 0; i < subdirs.size(); ++i){
    dirname = subdirs[i].canonicalFilePath();
    if((!dirname.endsWith(QString::fromUtf8("windows"))) && findCandidates(dirname)){
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

bool Extractor::readSources(const QString &sources)
{
  progress(QString::fromUtf8("Looking for existing ") + sources + QString::fromUtf8("..."));
  QFile f(findSrc(sources));
  if(!f.open(QIODevice::ReadOnly)){
    progress(sources + QString::fromUtf8(" not found."));
    return false;
  }
  progress(QString::fromUtf8("Found '%1'.").arg(f.fileName()));

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
  progress(sources + QString::fromUtf8(" found and read."));
  return (ui.FWCombo->count() != 0);
}


bool Extractor::readSpec()
{
  progress(QString::fromUtf8("Looking for existing spec.txt..."));
  QFile f(findSrc(QString::fromUtf8("spec.txt")));
  if(!f.open(QIODevice::ReadOnly)){
    progress(QString::fromUtf8("spec.txt not found."));
    return false;
  }
  progress(QString::fromUtf8("Found '%1'.").arg(f.fileName()));

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
  progress(QString::fromUtf8("spec.txt found and read."));
  return (targets.size() != 0);
}

Extractor::Extractor(QWidget *parent) : QDialog(parent), dl(NULL), progressDlg(NULL)
{
  ui.setupUi(this);
  wine = new WineLauncher();
  dl = new Downloading();
  progressDlg = new Progress();
  enableButtons(true);
}

TirFwExtractor::TirFwExtractor(QWidget *parent) : Extractor(parent), et(NULL)
{
  et = new ExtractThread();
  QObject::connect(et, SIGNAL(progress(const QString &)), this, SLOT(progress(const QString &)));
  QObject::connect(et, SIGNAL(finished()), this, SLOT(threadFinished()));
  QObject::connect(wine, SIGNAL(finished(bool)), this, SLOT(wineFinished(bool)));
  QObject::connect(dl, SIGNAL(done(bool, QString)), this, SLOT(downloadDone(bool, QString)));
  QObject::connect(dl, SIGNAL(msg(const QString &)), this, SLOT(progress(const QString &)));
  QObject::connect(dl, SIGNAL(msg(qint64, qint64)), progressDlg, SLOT(message(qint64, qint64)));
  haveSpec = readSpec();
#ifndef DARWIN
  QString sources = QString::fromUtf8("sources.txt");
#else
  QString sources = QString::fromUtf8("sources_mac.txt");
#endif
  readSources(sources);
  QString dbg = QProcessEnvironment::systemEnvironment().value(QString::fromUtf8("LINUXTRACK_DBG"));
  if(!dbg.contains(QChar::fromLatin1('d'))){
    ui.AnalyzeSourceButton->setVisible(false);
  }
}

Extractor::~Extractor()
{
  delete wine;
  delete dl;
  delete progressDlg;
}

TirFwExtractor::~TirFwExtractor()
{
  if(et->isRunning()){
    et->stop();
    et->wait(5000);
    if(et->isRunning()){
      et->terminate();
      et->wait(5000);
    }
  }
  delete et;
  et = NULL;
}

static QString makeDestPath(const QString &base)
{
  QDateTime current = QDateTime::currentDateTime();
  QString result = QString::fromUtf8("%2").arg(current.toString(QString::fromUtf8("yyMMdd_hhmmss")));
  QString final = result;
  QDir dir = QDir(base);
  int counter = 0;
  while(dir.exists(final)){
    final = QString(QString::fromUtf8("%1_%2")).arg(result).arg(counter++);
  }
  dir.mkpath(final);
  return base + QString::fromUtf8("/") + final + QString::fromUtf8("/");
}


void TirFwExtractor::wineFinished(bool result)
{
  if(!result){
    QMessageBox::warning(this, QString::fromUtf8("Error running Wine"),
      QString::fromUtf8("There was an error when extracting\n"
      "the firmware, will try the analysis\n"
      "just in case..."
      "Please see the log for more details.\n\n")
    );
  }
  destPath = makeDestPath(PrefProxy::getRsrcDirPath());
  et->start(targets, winePrefix, destPath);
}

void TirFwExtractor::commenceExtraction(QString file)
{
#ifndef DARWIN
  QMessageBox::information(this, QString::fromUtf8("Instructions"),
  QString::fromUtf8("NP's TrackIR installer will pop up now.\n\n"
  "Install it with all components to the default location, so the firmware and other necessary "
  "elements can be extracted.\n\n"
  "The software will be installed to the wine sandbox, that will be deleted afterwards, so "
  "there are no leftovers.")
  );
#endif
  qDebug()<<winePrefix;
  progress(QString::fromUtf8("Initializing wine and running installer %1").arg(file));
  //To avoid adding TrackIR icons/menus to Linux "start menu"...
  wine->setEnv(QString::fromUtf8("WINEDLLOVERRIDES"), QString::fromUtf8("winemenubuilder.exe=d"));
  //To redirect wine's Desktop directory to avoid TrackIR icon being placed on Linux desktop
  QFile xdgFile(winePrefix + QString::fromUtf8("/user-dirs.dirs"));
  if(xdgFile.open(QFile::WriteOnly | QFile::Truncate)){
    QTextStream xdg(&xdgFile);
    xdg<<"XDG_DESKTOP_DIR=\""<<winePrefix<<"\""<<endl;
    xdgFile.close();
    wine->setEnv(QString::fromUtf8("XDG_CONFIG_HOME"), winePrefix);
  }
  wine->setEnv(QString::fromUtf8("WINEPREFIX"), winePrefix);
  wine->run(file);
}


void Extractor::on_BrowseInstaller_pressed()
{
  enableButtons(false);
  ui.BrowseInstaller->setEnabled(false);
  QString file = QFileDialog::getOpenFileName(this, QString::fromUtf8("Open an installer:"));
  if(file.isEmpty()){
    enableButtons(true);
    return;
  }
  winePrefix = QDir::tempPath();
  winePrefix += QString::fromUtf8("/wineXXXXXX");
  QByteArray charData = winePrefix.toUtf8();
  char *prefix = mkdtemp(charData.data());
  if(prefix == NULL){
    enableButtons(true);
    return;
  }
  winePrefix = QString::fromUtf8(prefix);
  commenceExtraction(file);
}


void TirFwExtractor::browseDirPressed()
{
  enableButtons(false);
  QString dirName = QFileDialog::getExistingDirectory(this,
    QString::fromUtf8("Open a directory containing unpacked TrackIR driver:"));
  if(dirName.isEmpty()){
    enableButtons(true);
    return;
  }
  destPath = makeDestPath(PrefProxy::getRsrcDirPath());
  et->start(targets, dirName, destPath);
}

void TirFwExtractor::on_AnalyzeSourceButton_pressed()
{
  enableButtons(false);
  targets.clear();
  QString dirName = QFileDialog::getExistingDirectory(this, QString::fromUtf8("Open a wine directory:"));
  if(dirName.isEmpty()){
    enableButtons(true);
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
    qDebug()<<files[i].fileName()<<QString::fromUtf8(" ")<<size<<fh<<md5<<sha1;
    BlockId blk(files[i].fileName(), size, fh, md5, sha1);
    targets.insert(std::pair<uint16_t, BlockId>(fh, blk));
  }

  QFile of(QString::fromUtf8("spec.txt"));
  if(!of.open(QFile::WriteOnly | QFile::Truncate)){
    enableButtons(true);
    return;
  }
  QTextStream out(&of);

  for(targets_iterator_t it = targets.begin(); it != targets.end(); ++it){
    it->second.save(out);
  }
  of.close();
  enableButtons(true);
}

void Extractor::progress(const QString &msg)
{
  ui.LogView->appendPlainText(msg);
}

void TirFwExtractor::threadFinished()
{
  enableButtons(true);
  bool everything = et->haveEverything();
  if(everything){
    QString l = PrefProxy::getRsrcDirPath() + QString::fromUtf8("/tir_firmware");
      if(QFile::exists(l)){
        QFile::remove(l);
      }
    QFile::link(destPath, l);
    QMessageBox::information(NULL, QString::fromUtf8("Firmware extraction successfull"),
      QString::fromUtf8("Firmware extraction finished successfuly!"
#ifdef DARWIN
      "\nNow you can install linuxtrack-wine.exe to the Wine bottle/prefix of your choice."
#endif
      )
    );
  }else{
    QMessageBox::warning(NULL, QString::fromUtf8("Firmware extraction unsuccessfull"),
      QString::fromUtf8("Some of the files needed to fully utilize TrackIR were not "
      "found! Please see the log for more details.")
    );
  }
  emit finished(everything);
}


void Extractor::enableButtons(bool enable)
{
  ui.BrowseInstaller->setEnabled(enable);
  ui.BrowseDir->setEnabled(enable);
  ui.DownloadButton->setEnabled(enable);
  ui.QuitButton->setEnabled(enable);
}

void TirFwExtractor::enableButtons(bool enable)
{
  ui.BrowseInstaller->setEnabled(haveSpec && enable);
  ui.BrowseDir->setEnabled(haveSpec && enable);
  ui.DownloadButton->setEnabled(haveSpec && enable);
  ui.QuitButton->setEnabled(enable);
}


void Extractor::on_QuitButton_pressed()
{
  hide();
  //emit finished(et->haveEverything());
}


void TirFwExtractor::on_QuitButton_pressed()
{
  if(et->isRunning()){
    et->stop();
    et->wait();
  }
  hide();
  //emit finished(et->haveEverything());
}


void Extractor::on_DownloadButton_pressed()
{
  QString target(ui.FWCombo->currentText());
  qDebug()<<QString::fromUtf8("Going to download ")<<target;
  winePrefix = QDir::tempPath();
  winePrefix += QString::fromUtf8("/wineXXXXXX");
  QByteArray charData = winePrefix.toUtf8();
  char *prefix = mkdtemp(charData.data());
  if(prefix == NULL){
    return;
  }
  enableButtons(false);
  winePrefix = QString::fromUtf8(prefix);
  progressDlg->show();
  progressDlg->raise();
  progressDlg->activateWindow();
  dl->download(target, winePrefix);
}


void Extractor::downloadDone(bool ok, QString fileName)
{
  progressDlg->hide();
  if(ok){
    progress(QString::fromUtf8("Downloading finished!"));
    commenceExtraction(fileName);
  }else{
    QMessageBox::warning(NULL, QString::fromUtf8("Download unsuccessfull"),
      QString::fromUtf8("Download of the file was unsuccessful.\n"
      "Please check your network connection and try again;\n"
      "you can also download the file yourself and\n"
      "use the \"Extract from installer\" button to extract it.")
    );
    enableButtons(true);
  }
}

void Extractor::on_HelpButton_pressed()
{
  HelpViewer::ChangePage(QString::fromUtf8("extractor.htm"));
  HelpViewer::ShowWindow();
}

void Extractor::show()
{
  HelpViewer::ChangePage(QString::fromUtf8("extractor.htm"));
  QDialog::show();
}


/*
WINEDLLOVERRIDES=winemenubuilder.exe=d WINEPREFIX=/home/qbuilder/devel/research/extractor/test/TrackIR_5.2.Final wine TrackIR_5.2.Final.exe /s /v"/qb"
*/
