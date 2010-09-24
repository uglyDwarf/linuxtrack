#ifndef IPC_UTILS__H
#define IPC_UTILS__H

bool fork_child(int in, int out, const char *args[]);
bool wait_child_exit(int limit);

#endif