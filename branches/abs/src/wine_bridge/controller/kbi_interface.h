#ifndef KBI_INTERFACE__H
#define KBI_INTERFACE__H

#include <windows.h>
#include <string>

typedef enum {RECEIVING, DEF_PAUSE, DEF_RECENTER} state_t; 

//Implemented by client
bool kbi_init(HWND hwnd);
void kbi_close(void);
const std::string &kbi_check(int num);
void kbi_msg_loop();
void kbi_set_state(state_t s);

//Implemented by server
void set_names(const std::string pause_keys, const std::string recenter_keys);
#endif
