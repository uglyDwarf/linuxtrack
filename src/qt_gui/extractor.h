#ifndef EXTRACTOR__H
#define EXTRACTOR__H

#include "ui_extractor.h"

#include <QWidget>
#include <QThread>
#include <QProcess>
#include <QDialog>

#include <stdint.h>
#include <map>
#include "hashing.h"
#include "downloading.h"
#include "wine_launcher.h"
#include "progress.h"

typedef std::multimap<uint16_t, BlockId> targets_t;
typedef std::multimap<uint16_t, BlockId>::iterator targets_iterator_t;

class ExtractThread: public QThread
{
 Q_OBJECT
 public:
  ExtractThread() : targets(NULL), gameDataFound(false){};
  virtual void start(targets_t &t, const QString &p, const QString &d);
  void run();
  void stop(){quit = true;};
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
  bool tirviewsFound;
  bool quit;
  bool everything;
};





class Extractor: public QDialog
{
 Q_OBJECT
 public:
  Extractor(QWidget *parent = 0);
  ~Extractor();

 protected:
  Ui::Form ui;
  targets_t targets;
  WineLauncher *wine;
  QString winePrefix;
  Downloading *dl;
  Progress *progressDlg;
  QString destPath;
  bool readSpec();
  bool readSources(const QString &sources);
  QString findSrc(const QString &name);
  virtual void commenceExtraction(QString file){(void) file;};
  virtual void enableButtons(bool enable);
  virtual void browseDirPressed(){};
 signals:
  void finished(bool result);
 public slots:
  void show();
 protected slots:
  void on_BrowseInstaller_pressed();
  void on_BrowseDir_pressed(){browseDirPressed();};
  void on_AnalyzeSourceButton_pressed(){};
  void on_DownloadButton_pressed();
  void on_QuitButton_pressed();
  void on_HelpButton_pressed();
  void progress(const QString &msg);
  void threadFinished(){};
  void wineFinished(bool result){(void) result;};
  void downloadDone(bool ok, QString fileName);
};

class TirFwExtractor : public Extractor
{
 Q_OBJECT
 public:
  TirFwExtractor(QWidget *parent = 0);
  ~TirFwExtractor();
 private:
  void commenceExtraction(QString file);
  void enableButtons(bool enable);
  void browseDirPressed();
  ExtractThread *et;
  bool haveSpec;

 private slots:
  void threadFinished();
  void wineFinished(bool result);
  void on_AnalyzeSourceButton_pressed();
  void on_QuitButton_pressed();
};
/*
class Mfc42uExtractor : public Extractor
{
 public:
  TirFwExtractor(QWidget *parent = 0);
  ~TirFwExtractor();
 private:
  void commenceExtraction(QString file);
  void enableButtons(bool enable);
  void browseDirPressed();
  ExtractThread *et;
  bool haveSpec;

 private slots:
  void threadFinished();
  void wineFinished(bool result);
  void on_AnalyzeSourceButton_pressed();
  void on_QuitButton_pressed();
};
*/


#endif
