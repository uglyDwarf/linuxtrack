#include "webcam_info.h"
#include "webcam_driver.h"
#include "utils.h"
#include "dyn_load.h"

typedef int (*enum_webcams_fun_t)(char **ids[]);
typedef int (*enum_webcam_formats_fun_t)(char *id, webcam_formats *all_formats);
typedef int (*enum_webcam_formats_cleanup_fun_t)(webcam_formats *all_formats);

static enum_webcams_fun_t enum_webcams_fun = NULL;
static enum_webcam_formats_fun_t enum_webcam_formats_fun = NULL;
static enum_webcam_formats_cleanup_fun_t enum_webcam_formats_cleanup_fun = NULL;
static lib_fun_def_t functions[] = {
  {(char *)"enum_webcams", (void*) &enum_webcams_fun},
  {(char *)"enum_webcam_formats", (void*) &enum_webcam_formats_fun},
  {(char *)"enum_webcam_formats_cleanup", (void*) &enum_webcam_formats_cleanup_fun},
  {NULL, NULL}
};
static void *libhandle = NULL;


static bool load_webcam_lib()
{
  if(libhandle == NULL){
    if((libhandle = lt_load_library((char *)"libwc.so", functions)) == NULL){
      return false;
    }
  }
  return true;
}



WebcamInfo::WebcamInfo(const QString &id)
{
  webcam_id = id;
  load_webcam_lib();
  enum_webcam_formats_fun(webcam_id.toAscii().data(), &fmts);
  
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

const QStringList& WebcamInfo::getFormats()
{
  return format_strings;
}

const QStringList& WebcamInfo::getResolutions(int index)
{
  if((index >=0) && (index <= fmt_index)){
    return res_list[index];
  }else{
    return res_list[0];
  }
}

QString WebcamInfo::getFourcc(int index)
{
  char *fcc = (char *)&(fmt_descs[index][0]->fourcc);
  char fcc1[5];
  qstrncpy(fcc1, fcc, 5);
  fcc1[4] = '\0';
  return QString(fcc1);
}

WebcamInfo::~WebcamInfo()
{
  enum_webcam_formats_cleanup_fun(&fmts);
  //lt_unload_library(libhandle, functions);
}

QStringList& WebcamInfo::EnumerateWebcams()
{
  QStringList *res = new QStringList();
  if(!load_webcam_lib()){
    return *res;
  }
  char **ids = NULL;
  
  if(enum_webcams_fun(&ids) > 0){
    int id_num = 0;
    
    while((ids[id_num]) != NULL){
      res->append(ids[id_num]);
      ++id_num;
    }
    array_cleanup(&ids);
  }
  //lt_unload_library(libhandle, functions);
  
  return *res;
}
