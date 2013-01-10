#ifndef EXTRACTOR__H
#define EXTRACTOR__H

#include "ui_extractor.h"
#include "ui_progress.h"

#include <QWidget>
#include <QThread>
#include <QProcess>
#include <QDialog>

#include <stdint.h>
#include <map>
#include "hashing.h"
#include "downloading.h"

typedef std::multimap<uint16_t, BlockId> targets_t;
typedef std::multimap<uint16_t, BlockId>::iterator targets_iterator_t;

class Progress: public QDialog
{
 Q_OBJECT
 public:
  Progress(){ui.setupUi(this); ui.InfoLabel->setText("");};
  void show(){ui.ProgressBar->setValue(0);};
 private:
  Ui::DLProgress ui;
 public slots:
  void message(qint64 read, qint64 all){ui.ProgressBar->setValue((float)read / all * 100.0);};
};

class ExtractThread: public QThread
{
 Q_OBJECT
 public:
  ExtractThread() : targets(NULL), gameDataFound(false){};
  virtual void start(targets_t &t, const QString &p, const QString &d);
  void run();
  void stop(){};
  bool haveEverything(){return everything;};
 signals:
  void progress(const QString &msg);
 private:
  void analyzeFile(const QString fname);
  bool findCandidates(QString name);
  bool allFound();
  targets_t *targets;
  QString path;
  QString destPath;
  bool gameDataFound;
  bool quit;
  bool everything;
};

class Extractor: public QDialog
{
 Q_OBJECT
 public:
  Extractor(QWidget *parent = 0);
  ~Extractor();
 
 private:
  Ui::Form ui;
  targets_t targets;
  ExtractThread *et;
  QProcess *wine;
  QString winePrefix;
  Downloading *dl;
  Progress *progressDlg;
  bool readSpec();
  bool readSources();
  QString findSrc(const QString &name);
  void extractFirmware(QString file);
 signals:
  void finished(bool result);
 private slots:
  void on_BrowseButton_pressed();
  void on_AnalyzeSourceButton_pressed();
  void on_DownloadButton_pressed();
  void on_QuitButton_pressed();
  void progress(const QString &msg);
  void threadFinished();
  void wineFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void downloadDone(bool ok, QString fileName);
};


#endif