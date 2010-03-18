#include <QMessageBox>
#include <iostream>
#include <QByteArray>
#include "ltr_gui.h"

#include "webcam_driver.h"

void LinuxtrackGui::WebcamPrefsInit()
{
  ui.WebcamIDs->clear();
  ui.WebcamIDs->addItem("None");
  char **ids;
  
  if(enum_webcams(&ids) > 0){
    int id_num = 0;
    
    while((ids[id_num]) != NULL){
      ui.WebcamIDs->addItem(ids[id_num]);
      ++id_num;
    }
    enum_webcams_cleanup(&ids);
  }else{
    QMessageBox msg;
    msg.setText("Can't find any webcams!");
    msg.exec();
  }
}

static bool res_n_fps2fields(const QString &str, int &w, int &h, float &fps)
{
  const QRegExp &rexp = QRegExp("^\\s*(\\d+)\\s*[xX]\\s*(\\d+)\\s*@\\s*(\\S+)\\s*$");
  if(rexp.indexIn(str) == -1){
    return false;
  }
  w = rexp.cap(1).toInt();
  h = rexp.cap(2).toInt();
  fps = rexp.cap(3).toFloat();
  return true;
}


class webcam_info{
 public:
  webcam_info(const QString &id);
  const QStringList& getFormats();
  const QStringList& getResolutions(int index);
  QString getFourcc(int index);
  ~webcam_info();
 private:
  QString webcam_id;
  webcam_formats fmts;
  typedef QList<QList<webcam_format*> > format_array_t;
  typedef QList<QStringList> res_list_t;
  QStringList format_strings;
  format_array_t fmt_descs;
  res_list_t res_list;
  int fmt_index;
};

webcam_info::webcam_info(const QString &id)
{
  webcam_id = id;
  enum_webcam_formats(webcam_id.toAscii().data(), &fmts);
  
  int j;
  fmt_index = -1;
  int index;
  QString item, pixfmt, width, height, fps;
  for(j = 0; j < fmts.entries; ++j){
    index = fmts.formats[j].i;
    if(fmt_index != index){
      fmt_index = index;
      format_strings.push_back(QString::fromAscii(fmts.fmt_strings[index]));
      fmt_descs.push_back(QList<webcam_format*>());
      res_list.push_back(QStringList());
    }
    fmt_descs[index].push_back(&(fmts.formats[j]));
    width = QString::number(fmts.formats[j].w);
    height = QString::number(fmts.formats[j].h);
    fps = QString::number((float)fmts.formats[j].fps_den 
                          / fmts.formats[j].fps_num);
    item = QString("%2 x %3 @ %4").arg(width, height, fps);
    res_list[index].push_back(item);
  }
}

const QStringList& webcam_info::getFormats()
{
  return format_strings;
}

const QStringList& webcam_info::getResolutions(int index)
{
  if((index >=0) && (index <= fmt_index)){
    return res_list[index];
  }else{
    return res_list[0];
  }
}

QString webcam_info::getFourcc(int index)
{
  char *fcc = (char *)&(fmt_descs[index][0]->fourcc);
  char fcc1[5];
  qstrncpy(fcc1, fcc, 5);
  fcc1[4] = '\0';
  return QString(fcc1);
}

webcam_info::~webcam_info()
{
  enum_webcam_formats_cleanup(&fmts);
}

webcam_info *wc_info = NULL;

void LinuxtrackGui::on_WebcamIDs_currentIndexChanged(const QString &text)
{
  ui.WebcamFormats->clear();
  ui.WebcamResolutions->clear();
  if(text != "None"){
    if(wc_info != NULL){
      delete(wc_info);
    }
    wc_info = new webcam_info(text);
    
    ui.WebcamFormats->addItems(wc_info->getFormats());
  }
}

void LinuxtrackGui::on_WebcamFormats_currentIndexChanged(const QString &text)
{
  ui.WebcamResolutions->clear();
  QString id = ui.WebcamIDs->currentText();
  if(id == "None"){
    return;
  }
  ui.WebcamResolutions->addItems(wc_info->getResolutions(ui.WebcamFormats->currentIndex()));
}

void LinuxtrackGui::on_WebcamResolutions_currentIndexChanged(const QString &text)
{
  int w = 0;
  int h = 0;
  float fps = 0.0f;
  const QString &fourcc = wc_info->getFourcc(ui.WebcamFormats->currentIndex());
  if(res_n_fps2fields(text, w, h, fps)){
    std::cout<<"Id: '" << ui.WebcamIDs->currentText().toAscii().data() 
             << "'" << std::endl;
    std::cout<<"Chci >" << fourcc.toAscii().data() << w << " x " << h << 
               " @ " << fps <<std::endl;
  }
}
