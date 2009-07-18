#include <stdlib.h>
#include <stdio.h>
#include "cal.h"
#include "tir4_driver.h"
#include "pose.h"

int main(int argc, char **argv) {
  struct frame_type frame;
  struct camera_control_block ccb;
  struct transform t;
  struct reflector_model_type rm;

  ccb.device.category = tir4_camera;
/*   ccb.mode = diagnostic; */
/*   ccb.mode = operational_1dot; */
  ccb.mode = operational_3dot;

  cal_init(&ccb);

  rm.p1[0] = -35.0;
  rm.p1[1] = -50.0;
  rm.p1[2] = -92.5;
  rm.p2[0] = +35.0;
  rm.p2[1] = -50.0;
  rm.p2[2] = -92.5;
  rm.hc[0] = +0.0;
  rm.hc[1] = -100.0;
  rm.hc[2] = +90.0;
  pose_init(rm, 0.0);

  /* call below sets led green */
  cal_set_good_indication(&ccb, true);

  /* loop forever reading and printing results */
/*   while (true) { */
/*     cal_get_frame(&ccb, &frame); */
/*     frame_print(frame); */
/*     frame_free(&ccb, &frame); */
/*   } */

/*   /\* NEW PLAN *\/ */
/*   /\* NEW PLAN *\/ */
/*   /\* NEW PLAN *\/ */
/*   /\* going to read in from stdin simulated blobs.  */
/*    * echoing input blob out, along with alter92  */
/*    * estimates *\/ */
/*   struct bloblist_type working_bloblist; */
/*   struct blob_type wba[3]; */
/*   bool force_recenter = false; */
/*   int force_recenter_int; */
/*   int scanf_result; */
/*   working_bloblist.num_blobs = 3; */
/*   working_bloblist.blobs = wba; */
/*   while (!feof(stdin)) { */
/* /\*     printf("Enter blobs: "); *\/ */
/*     scanf_result=scanf("%f,%f,%f,%f,%f,%f,%d", */
/*                        &(wba[0].x), &(wba[0].y), */
/*                        &(wba[1].x), &(wba[1].y), */
/*                        &(wba[2].x), &(wba[2].y), */
/*                        &force_recenter_int); */
/*     if (scanf_result == 7) { */
/* /\*       printf("Received: "); *\/ */
/* /\*       printf("%f,%f,", (wba[0].x), (wba[0].y)); *\/ */
/* /\*       printf("%f,%f,", (wba[1].x), (wba[1].y)); *\/ */
/* /\*       printf("%f,%f,", (wba[2].x), (wba[2].y)); *\/ */
/* /\*       printf("%d\n", force_recenter_int); *\/ */
/*       pose_process_blobs(working_bloblist, &t); */
/*       transform_print(t); */
/*     } */
/*   } */

/*   int i; */
/*   for(i=0; i<2000; i++) { */
  while (true) {
    cal_get_frame(&ccb, &frame);
    frame_print(frame);
    pose_process_blobs(frame.bloblist, &t);
    transform_print(t);
    frame_free(&ccb, &frame);
  }

  /* call disconnects, turns all LEDs off */
  cal_shutdown(&ccb);
  return 0;
}
