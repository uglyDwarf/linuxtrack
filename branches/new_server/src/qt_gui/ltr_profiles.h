#ifndef LTR_PROFILES__H
#define LTR_PROFILES__H

#include <QWidget>
#include <QString>
#include <QStringList>

#define PROFILE Profile::getProfiles()

class AppProfile : public QWidget{
  Q_OBJECT
 public:
  AppProfile(const QString &n, QWidget *parent = 0);
  ~AppProfile();
  bool changeProfile(const QString &newName);
  const QString &getProfileName() const;
  float getFilterFactor();
  void setFilterFactor(float f);
 signals:
  void filterFactorChanged(float f);
 private:
  QString name;
  float filterFactor;
  void filterFactorReload();
  bool initializing;
};

class Profile{
 private:
  Profile();
  Profile(const Profile&);
  static Profile *prof;
  QStringList names;
  AppProfile *currentProfile;
  QString currentName;
 public:
  static Profile& getProfiles();
  void addProfile(const QString &name);
  const QStringList &getProfileNames();
  bool setCurrent(const QString &name);
  AppProfile *getCurrentProfile();
  const QString &getCurrentProfileName();
  int isProfile(const QString &name);
};

#endif
