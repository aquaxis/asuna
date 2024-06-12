/*
 * Copyright (C)2005-2024 AQUAXIS TECHNOLOGY.
 *  Don't remove this header.
 * When you use this source, there is a need to inherit this header.
 *
 * This software is released under the MIT License.
 * https://opensource.org/license/mit
 */

#ifndef _H_PARSR_IR_SIGNAL_
#define _H_PARSR_IR_SIGNAL_

extern int clean_signal_tree();
extern int create_verilog_signal();
extern int insert_signal_tree();
extern int output_signal_tree(FILE *fp);
extern int print_signal_tree(FILE *fp);
extern int register_signal_tree(char *label, char *type, int flag);

#endif
