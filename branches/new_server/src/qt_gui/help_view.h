#include <QWidget>
#include <QFile>
#include <QTextStream>
#include <QWebView>
#include <QListWidget>
#include "ui_logview.h"
#include "QMap"

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
  static void RaiseWindow();
 private slots:
  void on_CloseButton_pressed();
  void followLink(const QUrl &url);
  void currentTextChanged(const QString &currentText);
 private:
  void readFile();
  void addPage(QString name, QString page);
  bool ReadContents();
  Ui::LogViewerForm ui;
  QWebView *viewer;
  QListWidget *contents;
  QHBoxLayout *layout;
  QMap<QString, QString> pages;
};

