#include <QWidget>
#include <QFile>
#include <QTextStream>
#include <QWebView>
#include "ui_logview.h"

class QSettings;

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
 private slots:
  void on_CloseButton_pressed();
  void followLink(const QUrl &url);
 private:
  void readFile();
  Ui::LogViewerForm ui;
  QWebView *viewer;
};

