#ifndef LTR_SERVER__H
#define LTR_SERVER__H

#ifdef __cplusplus
extern "C" {
#endif

int prep_main_loop(char *section);
#ifdef LTR_GUI
extern void new_frame(struct frame_type *frame, void *param);
#endif


#ifdef __cplusplus
}
#endif



#endif
