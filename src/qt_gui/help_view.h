#ifndef HELP_VIEW__H
#define HELP_VIEW__H

#include <QWidget>
#include "ui_logview.h"
#include "help_viewer.h"

class QSettings;
class QHelpEngine;
class QHelpContentWidget;
class QSplitter;
class HelpViewWidget;

class HelpViewer : public QWidget{
  Q_OBJECT
  HelpViewer(QWidget *parent = 0);
  ~HelpViewer();
  static HelpViewer *hlp;
  static HelpViewer &getHlp();
  void ChangeHelpPage(QString name);
 public:
  static void ShowWindow();
  static void ChangePage(QString name);
  static void CloseWindow();
  static void LoadPrefs(QSettings &settings);
  static void StorePrefs(QSettings &settings);
  static void RaiseWindow();
 private slots:
  void on_CloseButton_pressed();
  void followLink(const QUrl &url);
  //void currentTextChanged(const QString &currentText);
  void helpInitialized();
  void itemClicked(const QModelIndex &index);
 private:
  Ui::LogViewerForm ui;
  QHBoxLayout *layout;
  QSplitter *splitter;
  QHelpEngine *helpEngine;
  QHelpContentWidget *contents;
  HelpViewWidget *viewer;
};

#endif

