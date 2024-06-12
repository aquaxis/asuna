/*
 * Copyright (C)2005-2024 AQUAXIS TECHNOLOGY.
 *  Don't remove this header.
 * When you use this source, there is a need to inherit this header.
 *
 * This software is released under the MIT License.
 * https://opensource.org/license/mit
 */
#ifndef _H_MAIN_
#define _H_MAIN_

#define STR_MAX 1024 // 1行の最大長

#define REVISION_MAIN    1
#define REVISION_MANOR   0

extern void help();
extern int main(int argc,char *argv[]);
extern int print_verilog_if();
extern int process(char *csource);
extern int process_function(char *name);
extern int split_c_source(char *filename);

#endif
