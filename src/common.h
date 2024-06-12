/*
 * Copyright (C)2005-2024 AQUAXIS TECHNOLOGY.
 *  Don't remove this header.
 * When you use this source, there is a need to inherit this header.
 *
 * This software is released under the MIT License.
 * https://opensource.org/license/mit
 */
#ifndef _H_COMMON_
#define _H_COMMON_

extern int get_width(char *buf);
extern char *charalloc(char *in);
extern char *convname(char *buf);
extern char *convtype(char *buf);

extern int dbgprintf(const char *format, ...);

#endif
