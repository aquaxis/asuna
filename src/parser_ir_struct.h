/*
 * Copyright (C)2005-2024 AQUAXIS TECHNOLOGY.
 *  Don't remove this header.
 * When you use this source, there is a need to inherit this header.
 *
 * This software is released under the MIT License.
 * https://opensource.org/license/mit
 */

#ifndef _H_PARSR_IR_STRUCT_
#define _H_PARSR_IR_STRUCT_

extern int clean_struct_tree();
extern int get_struct_argument(char *label, char *argument);
extern int insert_struct_tree();
extern int get_struct_argument(char *label, char *argument);
extern int parser_struct_tree();
extern int register_struct_tree(char *label, char *argument);
extern int is_struct_name(char *label);

#endif
