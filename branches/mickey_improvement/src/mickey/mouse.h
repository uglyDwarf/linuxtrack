#ifndef MOUSE__H
#define MOUSE__H

typedef enum {LEFT_BUTTON = 1, RIGHT_BUTTON = 2, MIDDLE_BUTTON = 4} buttons_t;

#ifdef __cplusplus

struct mouseLocalData;

class mouseClass{
 public:
  mouseClass();
  ~mouseClass();
  bool init();
  bool move(int dx, int dy);
  bool click(buttons_t button, struct timeval ts);
 private:
  mouseLocalData *data;
};
#endif

#endif
