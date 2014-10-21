#ifndef PLUGIN_INSTALL
#define PLUGIN_INSTALL

#include <QObject>
#include "ui_ltr.h"
#include "wine_launcher.h"
#include "extractor.h"

class Extractor;

class PluginInstall : public QObject
{
  Q_OBJECT
 public:
  PluginInstall(const Ui::LinuxtrackMainForm &ui);
  ~PluginInstall();
 private slots:
  void installWinePlugin();
  void tirFirmwareInstall();
  void installLinuxtrackWine();
  void on_TIRFWButton_pressed();
  void on_TIRViewsButton_pressed();
  void finished(bool ok);
 private:
  enum state{TIR_FW, MFC, LTR_W, DONE, TIR_FW_ONLY, MFC_ONLY} state;
  const Ui::LinuxtrackMainForm &gui;
  void Connect();
  WineLauncher *inst;
  TirFwExtractor *dlfw;
  Mfc42uExtractor *dlmfc;
  bool isTirFirmwareInstalled();
  bool isMfc42uInstalled();
  void enableButtons(bool ena);
  void mfc42uInstall();
  const QString poem1;
  const QString poem2;
  const QString gameData;
  const QString mfc42u;
  const QString tirViews;
};


#endif

