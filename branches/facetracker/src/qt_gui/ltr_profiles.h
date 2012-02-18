#ifndef LTR_PROFILES__H
#define LTR_PROFILES__H

#include <QString>
#include <QStringList>
#include "ltr_axis.h"

#define PROFILE Profile::getProfiles()

class AppProfile : public QWidget{
  Q_OBJECT
 public:
  AppProfile(const QString &n, QWidget *parent = 0);
  ~AppProfile();
  bool changeProfile(const QString &newName);
  const QString &getProfileName() const;
  LtrAxis *getPitchAxis();
  LtrAxis *getRollAxis();
  LtrAxis *getYawAxis();
  LtrAxis *getTxAxis();
  LtrAxis *getTyAxis();
  LtrAxis *getTzAxis();
  float getFilterFactor();
  void setFilterFactor(float f);
 signals:
  void pitchChanged(AxisElem_t what);
  void rollChanged(AxisElem_t what);
  void yawChanged(AxisElem_t what);
  void txChanged(AxisElem_t what);
  void tyChanged(AxisElem_t what);
  void tzChanged(AxisElem_t what);
  void filterFactorChanged(float f);
 private slots:
  void on_pitchChange(AxisElem_t what);
  void on_rollChange(AxisElem_t what);
  void on_yawChange(AxisElem_t what);
  void on_txChange(AxisElem_t what);
  void on_tyChange(AxisElem_t what);
  void on_tzChange(AxisElem_t what);
 private:
  QString name;
  LtrAxis *pitch;
  LtrAxis *roll;
  LtrAxis *yaw;
  LtrAxis *tx;
  LtrAxis *ty;
  LtrAxis *tz;
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