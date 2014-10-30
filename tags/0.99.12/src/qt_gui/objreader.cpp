#include <iostream>
#include <QFile>
#include <QRegExp>
#include <QTextStream>

#include "objreader.h"
#include "pathconfig.h"
#include "ltr_gui_prefs.h"

int cnt = 0;
int vcnt = 0;
int tcnt = 0;

object_t object;
std::vector<object_t> object_table;

static void add_vertex(float x, float y, float z, 
                float nx, float ny, float nz,
                float s, float t)
{
  ++cnt;
  object.vtx_table.push_back((vtx_t){x, y, z, nx, ny, nz, s, t});
}

static void add_index(int index)
{
  ++vcnt;
  object.vtx_indices.push_back(index);
}

static void add_tris(int offset, int count, bool glass)
{
  ++tcnt;
  object.tris_table.push_back((tri_t){offset, count, glass});
}

bool glass = false;
static QRegExp vt_line(QString::fromUtf8("^\\s*VT\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s*$"));
static QRegExp idx10_line(QString::fromUtf8("^\\s*IDX10\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s*$"));
static QRegExp idx_line(QString::fromUtf8("^\\s*IDX\\s+(\\S+)\\s*$"));
static QRegExp tris_line(QString::fromUtf8("^\\s*TRIS\\s+(\\S+)\\s+(\\S+)\\s*$"));
static QRegExp texture_line(QString::fromUtf8("^\\s*TEXTURE\\s+(.*)\\s*$"));
static QRegExp glass_line(QString::fromUtf8("^\\s*GLASS\\s*$"));


static void process_line(const QString &line)
{
  if(vt_line.indexIn(line) != -1){
    float x, y, z, nx, ny, nz, s, t;
    x = vt_line.cap(1).toFloat(); 
    y = vt_line.cap(2).toFloat(); 
    z = vt_line.cap(3).toFloat(); 
    nx = vt_line.cap(4).toFloat(); 
    ny = vt_line.cap(5).toFloat(); 
    nz = vt_line.cap(6).toFloat(); 
    s = vt_line.cap(7).toFloat(); 
    t = vt_line.cap(8).toFloat(); 
    add_vertex(x, y, z, nx, ny, nz, s, t);
  }else if(idx10_line.indexIn(line) != -1){
    add_index(idx10_line.cap(1).toInt());
    add_index(idx10_line.cap(2).toInt());
    add_index(idx10_line.cap(3).toInt());
    add_index(idx10_line.cap(4).toInt());
    add_index(idx10_line.cap(5).toInt());
    add_index(idx10_line.cap(6).toInt());
    add_index(idx10_line.cap(7).toInt());
    add_index(idx10_line.cap(8).toInt());
    add_index(idx10_line.cap(9).toInt());
    add_index(idx10_line.cap(10).toInt());
  }else if(idx_line.indexIn(line) != -1){
    add_index(idx_line.cap(1).toInt());
  }else if(tris_line.indexIn(line) != -1){
    add_tris(tris_line.cap(1).toInt(), tris_line.cap(2).toInt(), glass);
    glass = false;
  }else if(texture_line.indexIn(line) != -1){
    if(!texture_line.cap(1).isEmpty()){
      object.texture = PrefProxy::getDataPath(texture_line.cap(1));
      //std::cout<<"Texture: "<<qPrintable(object.texture)<<std::endl;
    }
  }else if(glass_line.indexIn(line) != -1){
    glass = true;
  }
}

static void obj_init(object_t &obj)
{
  obj.vtx_table.clear();
  obj.vtx_indices.clear();
  obj.tris_table.clear();
  obj.texture = QString::fromUtf8("");
}

void read_obj()
{
  char *obj_list[] = {(char *)"sphere.obj", (char *)"sparow_opaq.obj", 
                      (char *)"sparow_glass.obj", NULL};
  
  for(int i = 0; obj_list[i] != NULL; ++i){
    QFile f(PrefProxy::getDataPath(QString::fromUtf8(obj_list[i])));
    obj_init(object); 
    cnt = 0;
    vcnt = 0;
    tcnt = 0;

    if(!f.open(QIODevice::ReadOnly | QIODevice::Text))
      continue;

    QTextStream in(&f);
    while(!in.atEnd()){
      QString line = in.readLine();
      process_line(line);
    }
    f.close();
    object_table.push_back(object);
    //std::cout<<(char *)obj_list[i]<<std::endl;
    //std::cout<< cnt <<" vertices processed!"<< std::endl;
    //std::cout<< vcnt <<" indexes processed!"<< std::endl;
    //std::cout<< tcnt <<" tris processed!"<< std::endl;
  }
  
}
