#ifndef OBJREADER__H
#define OBJREADER__H

#include <vector> 

typedef struct {
  float x, y, z;
  float nx, ny, nz;
  float s, t;
} vtx_t;

typedef struct {
  int offset;
  int count;
  bool glass;
} tri_t;

typedef struct {
  std::vector<vtx_t> vtx_table;
  std::vector<int> vtx_indices;
  std::vector<tri_t> tris_table;
  QString texture;
} object_t;

extern std::vector<object_t> object_table;

void read_obj();

#endif
