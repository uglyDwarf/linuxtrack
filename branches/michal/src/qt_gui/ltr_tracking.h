#ifndef LTR_TRACKING__H
#define LTR_TRACKING__H

#include "ltr_gui.h"
#include "ltr_axis.h"


class LtrTracking : public QObject
{
  Q_OBJECT
 public:
  LtrTracking(const Ui::LinuxtrackMainForm &ui);
  ~LtrTracking();
 signals:
  void customSectionChanged();
 private slots:
  void on_FilterSlider_valueChanged(int value);
  void ffChanged(float f);
  void on_Profiles_currentIndexChanged(const QString &text);
  void on_CreateNewProfile_pressed();
  
  void on_PitchEnable_stateChanged(int state);
  void on_RollEnable_stateChanged(int state);
  void on_YawEnable_stateChanged(int state);
  void on_XEnable_stateChanged(int state);
  void on_YEnable_stateChanged(int state);
  void on_ZEnable_stateChanged(int state);
  
  void pitchChanged(AxisElem_t what);
  void rollChanged(AxisElem_t what);
  void yawChanged(AxisElem_t what);
  void txChanged(AxisElem_t what);
  void tyChanged(AxisElem_t what);
  void tzChanged(AxisElem_t what);
 private:
  const Ui::LinuxtrackMainForm &gui;
  void Connect();
  LtrAxis *pitch;
  LtrAxis *roll;
  LtrAxis *yaw;
  LtrAxis *tx;
  LtrAxis *ty;
  LtrAxis *tz;
  
};

#endif
