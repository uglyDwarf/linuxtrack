#ifndef POSE__H
#define POSE__H

#include <stdbool.h>
#include "cal.h"

struct transform{
  float rot[3][3];
  float tr[3];
};

void init_model();
bool process_blobs(struct bloblist_type blobs, struct transform *trans);
void center();
void print_transform(struct transform trans);

#endif
