/*
 * Copyright (C)2005-2024 AQUAXIS TECHNOLOGY.
 *  Don't remove this header.
 * When you use this source, there is a need to inherit this header.
 *
 * This software is released under the MIT License.
 * https://opensource.org/license/mit
 */

#ifndef _H_PARSR_IR_TOP_
#define _H_PARSR_IR_TOP_

extern int clean_top_module_tree();
extern int create_top_assign();;
extern int create_top_global(FILE *fp);
extern int create_top_module();
extern int create_top_nofunc(FILE *fp);
extern int create_top_nowire(FILE *fp, char *topname);
extern int create_top_wire(FILE *fp);
extern int output_top_module(FILE *fp, char *topname);
extern int insert_module_signal_tree(char *module_name);
extern int insert_top_module_tree();
extern int insert_top_signal_tree();
extern int recall_module_signal_tree_end();
extern int recall_top_module_tree_end();
extern int recall_top_signal_tree_end();
extern int register_top_moduel_signal(char *module_name, char *signal_name, int io, int width);
extern int register_top_module_tree(char *module_name);
extern int search_top_module(char *module_name);
extern int search_top_signal(char *signal_name);
extern int print_top_signal_tree(FILE *fp);

#endif
