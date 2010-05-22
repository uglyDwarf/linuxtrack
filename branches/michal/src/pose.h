#ifndef POSE__H
#define POSE__H

#include <stdbool.h>
#include "cal.h"

/* all units are  in millimeters.  
 * The common, camera centric coordinate system is used:
 * +x is right (when facing the camera)
 * +y is up  (when facing the camera)
 * +z fires straight out of the camera */
typedef struct reflector_model_type {
  /* p0 is the topmost reflector, and it is always 
   * (0,0,0) because it is the origin for p1, p2,
   * and the head center */
  /* p1 is the leftmost reflector */
  float p1[3]; /* x,y,z */
  /* p1 is the remaining/rightmost reflector */
  float p2[3]; /* x,y,z */
  /* user's head center, again referenced to p0 */
  float hc[3];  /* x,y,z */
  enum {CAP, CLIP, SINGLE} type;
} reflector_model_type;

/* like the reflector model, all units are in millimeters.  
 * The common, camera centric coordinate system is used:
 * +x is right (when facing the camera)
 * +y is up  (when facing the camera)
 * +z fires straight out of the camera
 * one more usage note: translate first, then rotate! */
struct transform{
  /* translation from the centered reference frame
   * Example:
   * 1) centered/ref frame was taken with translation of 
   *    (0,0,5000) <- this is straight out from the 
   *                  camera, 50 cm away. 
   * 2) current frame at (-100,0,5000).
   * Ideally, the translation estimate for this will be
   * (-100,0,0), ie, a 10 cm move to the left
   */
  float tr[3];
  /* rotation matrix that will rotate from the centered 
   * reference frame to the latest frame */
  float rot[3][3];
};

void ltr_int_pose_init(struct reflector_model_type rm);

void ltr_int_pose_sort_blobs(struct bloblist_type bl);

bool ltr_int_pose_process_blobs(struct bloblist_type blobs, 
                        struct transform *trans,
                        bool centering);
bool ltr_int_is_single_point();                        
int ltr_int_pose_compute_camera_update(struct transform trans,
                               float *yaw,
                               float *pitch,
                               float *roll,
                               float *tx,
                               float *ty,
                               float *tz);
void ltr_int_transform_print(struct transform trans);

#endif
