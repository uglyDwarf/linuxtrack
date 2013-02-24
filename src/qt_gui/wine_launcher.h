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
  bool check();
 private:
  QProcess wine;
  QProcessEnvironment env;
 private slots:
  void finished(int exitCode, QProcess::ExitStatus exitStatus);
 signals:
  void finished(bool result);
};

#endif
