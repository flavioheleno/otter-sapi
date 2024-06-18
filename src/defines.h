#ifndef __OTTER_DEFINES_H__
#define __OTTER_DEFINES_H__

#define DEBUG(msg) fprintf(stderr, "[DEBUG]\t%s\n", msg)

#define DEBUGF(fmt, ...) do { \
    fprintf(stderr, "[DEBUG]\t"); \
    fprintf(stderr, fmt, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
  } while (0);

#endif
