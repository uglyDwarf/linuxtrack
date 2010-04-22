#ifndef PREF__H
#define PREF__H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pref_struct *pref_id;
typedef void (*pref_callback)(void *);
void default_cbk(void *flag_ptr);

#ifdef __cplusplus
}
#endif

#endif

