#include "webcam_info.h"
#include "webcam_driver.h"
#include "list.h"
#include "dyn_load.h"

#include <assert.h>
#include <iostream>

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

/*
 * Bastardized version of singleton pattern to take care of library 
 * loading/unloading...
 */
class WebcamLibProxy{
 private:
  void *libhandle;
  WebcamLibProxy();
  ~WebcamLibProxy();
  WebcamLibProxy(const WebcamLibProxy&){};
  WebcamLibProxy & operator=(const WebcamLibProxy&);
  static WebcamLibProxy wcl;
};

WebcamLibProxy WebcamLibProxy::wcl;

WebcamLibProxy::WebcamLibProxy(){
  if((libhandle = lt_load_library((char *)"libwc.so", functions)) == NULL){
    throw(0);
  }
}

WebcamLibProxy::~WebcamLibProxy(){
  lt_unload_library(libhandle, functions);
  libhandle = NULL;
}

WebcamInfo::WebcamInfo(const QString &id)
{
  webcam_id = id;
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

static QString U32_2_String(__u32 fourcc)
{
  char *fcc = (char *)&(fourcc);
  char fcc1[5];
  qstrncpy(fcc1, fcc, 5);
  fcc1[4] = '\0';
  return QString(fcc1);
}

QString WebcamInfo::getFourcc(int index)
{
  return U32_2_String(fmt_descs[index][0]->fourcc);
}

int WebcamInfo::findFourcc(const QString &fcc)
{
  for(int i = 0; i <= fmt_index; ++i){
    if(getFourcc(i) == fcc){
      return i;
    }
  }
  return 0;
}

typedef struct{
  int w, h;
  int fps_num, fps_den;
} res_struct;

static bool decodeRes(const QString &res, const QString &fps, res_struct &decoded)
{
  const QRegExp &res_rexp = QRegExp("^\\s*(\\d+)\\s*[xX]\\s*(\\d+)\\s*$");
  const QRegExp &fps_rexp = QRegExp("^(\\d+)\\s*/\\s*(\\d+)\\s*$");

  if(res_rexp.indexIn(res) == -1){
    return false;
  }
  if(fps_rexp.indexIn(fps) == -1){
    return false;
  }
  decoded.w = res_rexp.cap(1).toInt();
  decoded.h = res_rexp.cap(2).toInt();
  decoded.fps_num = fps_rexp.cap(1).toInt();
  decoded.fps_den = fps_rexp.cap(2).toInt();
  return true;
}

int WebcamInfo::findRes(const QString &res, const QString &fps, 
			const QString &fourcc)
{
  res_struct fmt;
  if(!decodeRes(res, fps, fmt)){
    return 0;
  }
  int index = findFourcc(fourcc);
  QList<webcam_format*>::const_iterator i;
  int counter = 0;
  webcam_format* tested;
  for(i = (fmt_descs[index]).begin(); i != (fmt_descs[index]).end(); ++i, ++counter){
    tested = *i;
    if((tested->w == fmt.w) && (tested->h == fmt.h) && 
       (tested->fps_num == fmt.fps_den) && (tested->fps_den == fmt.fps_num)){
      if(U32_2_String(tested->fourcc) == fourcc){
	return counter;
      }
    }
  }
  return 0;
}

bool WebcamInfo::findFmtSpecs(int i_fmt, int i_res, QString &res, 
			      QString &fps, QString &fmt)
{
  webcam_format* format = fmt_descs[i_fmt][i_res];
  res = QString("%1 x %2").arg(QString::number(format->w))
                          .arg(QString::number(format->h));
  fps = QString("%1/%2").arg(QString::number(format->fps_den))
                          .arg(QString::number(format->fps_num));
  fmt = getFourcc(i_fmt);
  return true;
}

WebcamInfo::~WebcamInfo()
{
  enum_webcam_formats_cleanup_fun(&fmts);
}

QStringList& WebcamInfo::EnumerateWebcams()
{
  QStringList *res = new QStringList();
  char **ids = NULL;
  
  if(enum_webcams_fun(&ids) > 0){
    int id_num = 0;
    
    while((ids[id_num]) != NULL){
      res->append(ids[id_num]);
      ++id_num;
    }
    array_cleanup(&ids);
  }
  return *res;
}
