#ifndef WINE_LAUNCHER__H
#define WINE_LAUNCHER__H

#include <QObject>
#include <QProcess>

class WineLauncher:public QObject
{
 Q_OBJECT
 public:
  WineLauncher();
  ~WineLauncher();
  void setEnv(const QString &var, const QString &val);
  void run(const QString &tgt);
  bool wineAvailable();
 private:
  bool check();
  void envSet(const QString var, const QString val);
  QProcess wine;
  QProcessEnvironment env;
  QString winePath;
  bool available;
 private slots:
  void finished(int exitCode, QProcess::ExitStatus exitStatus);
  void error(QProcess::ProcessError error);
 signals:
  void finished(bool result);
};

#endif
