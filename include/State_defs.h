#ifndef LIBBYZEA_STATE_DEFS_H
#define LIBBYZEA_STATE_DEFS_H

// uncomment to use BASE instead of BFT
//#define BASE

#ifdef BASE
#define OBJ_REP
#else
#ifndef NO_STATE_TRANSLATION
#define NO_STATE_TRANSLATION
#endif
#endif

#endif
