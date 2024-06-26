/*
 * Copyright (C)2005-2024 AQUAXIS TECHNOLOGY.
 *  Don't remove this header.
 * When you use this source, there is a need to inherit this header.
 *
 * This software is released under the MIT License.
 * https://opensource.org/license/mit
 */

/*!
 * @file  parser_ir_top.c
 * @brief  最上位モジュールの生成
 * @author  Hidemi Ishiahra
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "main.h"
#include "token.h"
#include "parser.h"
#include "parser_ir.h"
#include "parser_ir_call.h"
#include "parser_ir_memmap.h"
#include "parser_ir_memory.h"
#include "parser_ir_proc.h"
#include "parser_ir_signal.h"
#include "parser_ir_struct.h"
#include "parser_ir_top.h"

/*!
 * 最上位階層の信号
 */
typedef struct top_signal_tree{
  struct top_signal_tree  *prev_ptr;
  struct top_signal_tree  *next_ptr;

  char *label;          // ラベル
  int   size;
  int   inout;          // 信号の方向、0:In、1:OUT
  int   used;           // 信号が使用されたかどうか、0:未使用、1:使用済
  char *module_name;    // モジュール名
  int   flag;           // フラグ
  char *name;           // 信号名
  char *call_name;
  int   args_num;
  char *verilog_signal;
  char *verilog_wire;
} TOP_SIGNAL_TREE;

TOP_SIGNAL_TREE *top_signal_tree_top        = NULL;

#define FLAG_NONE   0
#define FLAG_CTRL   1
#define FLAG_GMEM   2
#define FLAG_ARGS   3
#define FLAG_BASE   4
#define FLAG_CALL   5
#define FLAG_RETURN 6

#define SIG_IN   0
#define SIG_OUT  1

int use_llvm_memcpy = 0;
int use_gm_if = 0;

/*!
 * @brief  モジュール宣言の登録
 */
char *top_module_decl = NULL;
int register_top_module_decl(char *line)
{
  char *temp = NULL;

  if(top_module_decl == NULL){
    top_module_decl = calloc(strlen(line)+1, 1);
    sprintf(top_module_decl, "%s", line);
  }else{
    temp = calloc(strlen(top_module_decl)+1, 1);
    strcpy(temp, top_module_decl);
    free(top_module_decl);
    top_module_decl = calloc(strlen(temp)+strlen(line)+1, 1);
    sprintf(top_module_decl, "%s%s", temp, line);
  }

  return 0;
}

/*!
 * @brief  モジュール宣言の登録
 *
 */
char *top_module_assign = NULL;
int register_top_module_assign(char *line)
{
  char *temp = NULL;

  if(top_module_assign == NULL){
    top_module_assign = calloc(strlen(line)+1, 1);
    sprintf(top_module_assign, "%s", line);
  }else{
    temp = calloc(strlen(top_module_assign)+1, 1);
    strcpy(temp, top_module_assign);
    free(top_module_assign);
    top_module_assign = calloc(strlen(temp)+strlen(line)+1, 1);
    sprintf(top_module_assign, "%s%s", temp, line);
  }

  return 0;
}

/*!
 * @brief  最後の信号ツリーを取得する
 */
int get_top_signal_tree_last(TOP_SIGNAL_TREE **last_top_signal_tree)
{
  TOP_SIGNAL_TREE *now_top_signal_tree;
  TOP_SIGNAL_TREE *old_top_signal_tree;

  now_top_signal_tree = top_signal_tree_top;
    while(now_top_signal_tree != NULL){
    old_top_signal_tree = now_top_signal_tree;
    now_top_signal_tree = old_top_signal_tree->next_ptr;
  }

  *last_top_signal_tree = old_top_signal_tree;

  return 0;
}

/*!
 * @brief  信号の登録
 */
int register_top_signal_tree(char *label,char *module_name, char *name, int flag, int inout, int size, char *call_name, int args_num, char *verilog_signal, char *verilog_wire)
{
  TOP_SIGNAL_TREE *now_top_signal_tree;
  TOP_SIGNAL_TREE *new_top_signal_tree;

  new_top_signal_tree  = (TOP_SIGNAL_TREE *)calloc(sizeof(TOP_SIGNAL_TREE), 1);

  if(top_signal_tree_top == NULL){
    top_signal_tree_top  = new_top_signal_tree;
  }else{
    get_top_signal_tree_last(&now_top_signal_tree);
    now_top_signal_tree->next_ptr  = new_top_signal_tree;
    new_top_signal_tree->prev_ptr  = now_top_signal_tree;
  }

  new_top_signal_tree->label      = calloc(strlen(label)+1,1);
  strcpy(new_top_signal_tree->label, label);
  new_top_signal_tree->module_name      = calloc(strlen(module_name)+1,1);
  strcpy(new_top_signal_tree->module_name, module_name);
  new_top_signal_tree->name      = calloc(strlen(name)+1,1);
  strcpy(new_top_signal_tree->name, name);
  if(flag == FLAG_CALL){
    new_top_signal_tree->call_name      = calloc(strlen(call_name)+1,1);
    strcpy(new_top_signal_tree->call_name, call_name);
  }
  new_top_signal_tree->verilog_signal  = calloc(strlen(verilog_signal)+1,1);
  strcpy(new_top_signal_tree->verilog_signal, verilog_signal);
  new_top_signal_tree->verilog_wire  = calloc(strlen(verilog_wire)+1,1);
  strcpy(new_top_signal_tree->verilog_wire, verilog_wire);

  new_top_signal_tree->inout      = inout;
  new_top_signal_tree->flag      = flag;
  new_top_signal_tree->args_num    = args_num;
  new_top_signal_tree->size      = size;
  new_top_signal_tree->used      = 0;

  return 0;
}

/*!
 * @brief  信号名を検索する
 */
char *search_top_signal_call_args(char *call_name, int args_num)
{
  TOP_SIGNAL_TREE *now_top_signal_tree = NULL;
  char *result = NULL;
  char *label = NULL;
  char *signal_name = NULL;
  char *verilog = NULL;

  result = calloc(STR_MAX, 1);
//printf("call_name: %s_%s(%d)\n", call_name, name, args_num);

  signal_name = calloc(STR_MAX, 1);
  label = calloc(STR_MAX, 1);

  sprintf(label, "__call_%s_%s_%d", call_name, "args", args_num);
  sprintf(signal_name, "%s", label);
//printf("call_name: %s\n", signal_name);

  now_top_signal_tree = top_signal_tree_top;
  while(now_top_signal_tree != NULL){
    if(now_top_signal_tree->flag == FLAG_CALL){
      if(!strcmp(now_top_signal_tree->label, label)){
        sprintf(result, "%s %s |", result, now_top_signal_tree->verilog_signal);
        now_top_signal_tree->used = 1;
//printf("    call_name: %s <> %s\n", now_top_signal_tree->label, label);
      }
    }
    now_top_signal_tree = now_top_signal_tree->next_ptr;
  }

  if(strlen(result) > 0){
    result[strlen(result)-1] = ';';
    verilog = calloc(strlen(result)+1, 1);
    strcpy(verilog, result);
//printf("    result: %s,%s\n", result, verilog);
  }

  free(result);

  return verilog;
}

/*!
 * @brief  信号名を検索する
 */
char *search_top_signal_func(char *func, char *call_name, char *args)
{
  TOP_SIGNAL_TREE *now_top_signal_tree = NULL;
  char *result = NULL;
  char *label = NULL;
  char *signal_name = NULL;
  char *verilog = NULL;

  result = calloc(STR_MAX, 1);
//printf("call_name: %s_%s(%d)\n", call_name, name, args_num);

  signal_name = calloc(STR_MAX, 1);
  label = calloc(STR_MAX, 1);

  sprintf(label, "__call_%s_%s", call_name, args);
  if(!strcmp(func, "call")){
    sprintf(label, "__%s_%s_%s", func, call_name, args);
  }else{
    sprintf(label, "__%s_%s", func, args);
  }
  sprintf(signal_name, "%s", label);
//printf("call_name: %s\n", signal_name);

  now_top_signal_tree = top_signal_tree_top;
  while(now_top_signal_tree != NULL){
//printf("    call_name: %s <> %s\n", now_top_signal_tree->label, label);
    if(now_top_signal_tree->flag == FLAG_CALL){
      if(!strcmp(now_top_signal_tree->label, label)){
        sprintf(result, "%s %s |", result, now_top_signal_tree->verilog_signal);
        now_top_signal_tree->used = 1;
//printf("    call_name: %s <> %s\n", now_top_signal_tree->label, label);
      }
    }else if(now_top_signal_tree->flag == FLAG_CTRL){
      if(!strcmp(now_top_signal_tree->label, label) && !strcmp(now_top_signal_tree->module_name, call_name)){
        sprintf(result, "%s %s |", result, now_top_signal_tree->verilog_signal);
        now_top_signal_tree->used = 1;
//printf("    call_name: %s <> %s\n", now_top_signal_tree->label, label);
      }
    }else if(now_top_signal_tree->flag == FLAG_GMEM){
      if(!strcmp(now_top_signal_tree->label, label)){
        sprintf(result, "%s %s |", result, now_top_signal_tree->verilog_signal);
        now_top_signal_tree->used = 1;
//printf("    call_name: %s <> %s\n", now_top_signal_tree->label, label);
      }
    }else if(now_top_signal_tree->flag == FLAG_RETURN){
      if(!strcmp(now_top_signal_tree->label, label) && !strcmp(now_top_signal_tree->module_name, call_name)){
        sprintf(result, "%s %s |", result, now_top_signal_tree->verilog_signal);
        now_top_signal_tree->used = 1;
//printf("    call_name: %s <> %s\n", now_top_signal_tree->label, label);
      }
    }


    now_top_signal_tree = now_top_signal_tree->next_ptr;
  }

  if(strlen(result) > 0){
    result[strlen(result)-1] = ';';
    verilog = calloc(strlen(result)+1, 1);
    strcpy(verilog, result);
//printf("    result: %s,%s\n", result, verilog);
  }

  free(result);

  return verilog;
}

/*!
 * @brief  信号接続の生成
 *
 * @note
 * ここでトップモジュールのassign接続を生成します。
 */
int create_top_assign()
{
  TOP_SIGNAL_TREE *now_top_signal_tree;
  char *verilog = NULL;
  char *wire = NULL;
  int i;

  wire = calloc(STR_MAX, 1);

  now_top_signal_tree = top_signal_tree_top;
  while(now_top_signal_tree != NULL){
    if(now_top_signal_tree->inout == SIG_IN){
//      sprintf(wire, "assign %s = ", now_top_signal_tree->label);
      sprintf(wire, "assign %s = ", now_top_signal_tree->verilog_signal);

      switch(now_top_signal_tree->flag){
        case FLAG_ARGS:
          verilog = search_top_signal_call_args(now_top_signal_tree->module_name, now_top_signal_tree->args_num);
          if(verilog != NULL){
            now_top_signal_tree->used = 1;
            sprintf(wire, "%s%s\n", wire, verilog);
            register_top_module_assign(wire);
          }
          break;
        case FLAG_CTRL:
          if(!strcmp(now_top_signal_tree->name, "__func_start")){
            verilog = search_top_signal_func("call", now_top_signal_tree->module_name, "req");
            if(verilog != NULL){
              now_top_signal_tree->used = 1;
              sprintf(wire, "%s%s\n", wire, verilog);
              register_top_module_assign(wire);
            }
          }
          break;
        case FLAG_CALL:
          if(!strcmp(now_top_signal_tree->name, "done")){
            verilog = search_top_signal_func("func", now_top_signal_tree->call_name, "done");
            if(verilog != NULL){
              now_top_signal_tree->used = 1;
              sprintf(wire, "%s%s\n", wire, verilog);
              register_top_module_assign(wire);
            }
          }else if(!strcmp(now_top_signal_tree->name, "ready")){
            verilog = search_top_signal_func("func", now_top_signal_tree->call_name, "ready");
            if(verilog != NULL){
              now_top_signal_tree->used = 1;
              sprintf(wire, "%s%s\n", wire, verilog);
              register_top_module_assign(wire);
            }
          }else if(!strcmp(now_top_signal_tree->name, "result")){
            verilog = search_top_signal_func("func", now_top_signal_tree->call_name, "result");
            if(verilog != NULL){
              now_top_signal_tree->used = 1;
              sprintf(wire, "%s%s\n", wire, verilog);
              register_top_module_assign(wire);
            }
          }
          break;
        case FLAG_GMEM:
          if(!strcmp(now_top_signal_tree->name, "__gm_done")){
            now_top_signal_tree->used = 1;
            sprintf(wire, "%s%s\n", wire, "__gm_done;");
            register_top_module_assign(wire);
          }else if(!strcmp(now_top_signal_tree->name, "__gm_di")){
            now_top_signal_tree->used = 1;
            sprintf(wire, "%s%s\n", wire, "__gm_di;");
            register_top_module_assign(wire);
          }else if(!strcmp(now_top_signal_tree->name, "__gm_base")){
            now_top_signal_tree->used = 1;
            sprintf(wire, "%s%s\n", wire, "__gm_base;");
            register_top_module_assign(wire);
          }
          break;
        default:
          break;
      }
    }
        now_top_signal_tree = now_top_signal_tree->next_ptr;
  }

  sprintf(wire, "\n");
  register_top_module_assign(wire);

  if(use_gm_if){
    sprintf(wire, "// Global Memory\n");
    register_top_module_assign(wire);

    // Global Memory
    verilog = search_top_signal_func("gm", NULL, "req");
    sprintf(wire, "assign __gm_req = %s\n", verilog);
    register_top_module_assign(wire);

    verilog = search_top_signal_func("gm", NULL, "adrs");
    sprintf(wire, "assign __gm_adrs = %s\n", verilog);
    register_top_module_assign(wire);

    verilog = search_top_signal_func("gm", NULL, "rnw");
    sprintf(wire, "assign __gm_rnw = %s\n", verilog);
    register_top_module_assign(wire);

    verilog = search_top_signal_func("gm", NULL, "do");
    sprintf(wire, "assign __gm_do = %s\n", verilog);
    register_top_module_assign(wire);

    verilog = search_top_signal_func("gm", NULL, "leng");
    sprintf(wire, "assign __gm_leng = %s\n", verilog);
    register_top_module_assign(wire);

    sprintf(wire, "\n");
    register_top_module_assign(wire);
  }

  // LLVM memcpy
  verilog = search_top_signal_func("call", "llvm_memcpy", "req");
  if(verilog != NULL){
    sprintf(wire, "// LLVM memcpy\n");
    register_top_module_assign(wire);

    sprintf(wire, "assign __llvm_memcpy_req = %s\n", verilog);
    register_top_module_assign(wire);

    use_llvm_memcpy = 1;
  }

  now_top_signal_tree = top_signal_tree_top;
  while(now_top_signal_tree != NULL){
    if(now_top_signal_tree->inout == SIG_IN){
      sprintf(wire, "assign %s = ", now_top_signal_tree->verilog_signal);

      if(now_top_signal_tree->flag == FLAG_CALL && !strcmp(now_top_signal_tree->call_name, "llvm_memcpy")){
        now_top_signal_tree->used = 1;
        sprintf(wire, "%s__llvm_memcpy_%s;\n", wire, now_top_signal_tree->name);
        register_top_module_assign(wire);
      }
    }
        now_top_signal_tree = now_top_signal_tree->next_ptr;
  }

  for(i = 0; i < 5; i++){
    sprintf(wire, "assign __llvm_memcpy_args_%d = ", i);
    verilog = search_top_signal_call_args("llvm_memcpy", i);
    if(verilog != NULL){
//      now_top_signal_tree->used = 1;
      sprintf(wire, "%s%s\n", wire, verilog);
      register_top_module_assign(wire);
    }
  }

  return 0;
}

/*!
 * @brief  信号宣言の生成
 */
int create_top_wire(FILE *fp)
{
  TOP_SIGNAL_TREE *now_top_signal_tree;

  now_top_signal_tree = top_signal_tree_top;
    while(now_top_signal_tree != NULL){
    fprintf(fp, "%s\n", now_top_signal_tree->verilog_wire);
        now_top_signal_tree = now_top_signal_tree->next_ptr;
  }

  return 0;
}

/*!
 * @brief  未使用の信号宣言の生成
 */
int create_top_nowire(FILE *fp, char *topname)
{
  TOP_SIGNAL_TREE *now_top_signal_tree;

  char *topmodule;
  topmodule = malloc(strlen(topname)+1);
  strncpy(topmodule, topname, strlen(topname)-4);

  now_top_signal_tree = top_signal_tree_top;
  while(now_top_signal_tree != NULL){
    if((now_top_signal_tree->used == 0) && (!strcmp(now_top_signal_tree->module_name, topmodule))){
      if(now_top_signal_tree->flag == FLAG_ARGS){
        if(now_top_signal_tree->inout == SIG_IN){
          fprintf(fp, "\tinput ");
        }else{
          fprintf(fp, "\toutput ");
        }
        if(now_top_signal_tree->size > 0){
          fprintf(fp, "[%d:0] ", now_top_signal_tree->size*8-1);
        }
        fprintf(fp, "%s,\n", now_top_signal_tree->label);
      }else if(now_top_signal_tree->flag == FLAG_RETURN){
        fprintf(fp, "\toutput ");
        if(now_top_signal_tree->size > 0){
          fprintf(fp, "[%d:0] ", now_top_signal_tree->size*8-1);
        }
        fprintf(fp, "%s\n", now_top_signal_tree->label);
      }
    }
    now_top_signal_tree = now_top_signal_tree->next_ptr;
  }

  free(topmodule);

  return 0;
}

/*!
 * @brief  システム信号の生成
 */
int create_top_nofunc(FILE *fp)
{
  TOP_SIGNAL_TREE *now_top_signal_tree;

  now_top_signal_tree = top_signal_tree_top;
  while(now_top_signal_tree != NULL){
    if((now_top_signal_tree->used == 0) && (
      (now_top_signal_tree->flag == FLAG_CTRL) ||
      (now_top_signal_tree->flag == FLAG_ARGS) ||
      (now_top_signal_tree->flag == FLAG_RETURN)
      )
    ){
      if(now_top_signal_tree->inout == SIG_IN){
        fprintf(fp, "assign %s = %s;\n", now_top_signal_tree->verilog_signal, now_top_signal_tree->label);
      }else{
        fprintf(fp, "assign %s = %s;\n", now_top_signal_tree->label, now_top_signal_tree->verilog_signal);
      }
    }
    now_top_signal_tree = now_top_signal_tree->next_ptr;
  }

  return 0;
}

/*!
 * @brief  システム信号の生成
 */
int create_top_global(FILE *fp)
{
  MODULE_TREE *now_module_tree = NULL;
  MEMORY_TREE *now_memory_tree = NULL;

  char *label;
  label        = calloc(STR_MAX, 1);

  now_module_tree = module_tree_top;
  while(now_module_tree != NULL){
    now_memory_tree = now_module_tree->memory_tree_ptr;
    while(now_memory_tree != NULL){
      if(now_memory_tree->flag == MEMORY_FLAG_GLOBAL){
        sprintf(label, "%s", convname(sep_p(now_memory_tree->label)));
        fprintf(fp, "\tinput [31:0] __base_%s,\n", label);
      }
      now_memory_tree = now_memory_tree->next_ptr;
    }
    now_module_tree = now_module_tree->next_ptr;
  }

  return 0;
}

/*!
 * @brief  最上位階層の生成
 */
int create_top_module()
{
  MODULE_TREE *now_module_tree = NULL;
  MEMORY_TREE *now_memory_tree = NULL;
  CALL_TREE *now_call_tree = NULL;
  CALL_SIGNAL_TREE *now_call_signal_tree = NULL;
  char *module_name;
  char *call_signal_name;
  int args_num;
  char *label;
  char *verilog_signal;
  char *verilog_wire;
  char *verilog_module;
  int inout;

  printf("[Create top Module]\n");

  call_signal_name = calloc(STR_MAX, 1);
  label            = calloc(STR_MAX, 1);
  verilog_signal   = calloc(STR_MAX, 1);
  verilog_wire     = calloc(STR_MAX, 1);
  verilog_module   = calloc(STR_MAX, 1);

  now_module_tree  = module_tree_top;
  while(now_module_tree != NULL){
    // 初期化
    args_num    = 0;
    module_name = calloc(strlen(now_module_tree->module_name)+1,1);
    strcpy(module_name, convname(sep_p(now_module_tree->module_name)));

    // モジュール宣言
    sprintf(verilog_module, "%s u_%s(\n", module_name, module_name);
    register_top_module_decl(verilog_module);

    // 制御信号の生成
    sprintf(verilog_module, "\t// system signals\n");
    register_top_module_decl(verilog_module);
    {
      sprintf(verilog_module, "\t.__func_clock(system_clock),\n");
      register_top_module_decl(verilog_module);
      sprintf(verilog_module, "\t.__func_reset(system_reset),\n");
      register_top_module_decl(verilog_module);
      // __func_start(in)
      sprintf(label, "__func_start");
      sprintf(verilog_signal, "%s%s", module_name, label);
      sprintf(verilog_module, "\t.%s(%s),\n", label, verilog_signal);
      register_top_module_decl(verilog_module);
      sprintf(verilog_wire, "wire %s;", verilog_signal);
      register_top_signal_tree(label, module_name, label, FLAG_CTRL, SIG_IN, 0, NULL, 0, verilog_signal, verilog_wire);
      // ToDo: 接続されるべき信号を検索してWired OR宣言を生成する。
      // __func_done(out)
      sprintf(label, "__func_done");
      sprintf(verilog_signal, "%s%s", module_name, label);
      sprintf(verilog_module, "\t.%s(%s),\n", label, verilog_signal);
      register_top_module_decl(verilog_module);
      sprintf(verilog_wire, "wire %s;", verilog_signal);
      register_top_signal_tree(label, module_name, label, FLAG_CTRL, SIG_OUT, 0, NULL, 0, verilog_signal, verilog_wire);
      // __func_ready(out)
      sprintf(label, "__func_ready");
      sprintf(verilog_signal, "%s%s", module_name, label);
      sprintf(verilog_module, "\t.%s(%s),\n", label, verilog_signal);
      register_top_module_decl(verilog_module);
      sprintf(verilog_wire, "wire %s;", verilog_signal);
      register_top_signal_tree(label, module_name, label, FLAG_CTRL, SIG_OUT, 0, NULL, 0, verilog_signal, verilog_wire);
    }

    sprintf(verilog_module, "\n");
    register_top_module_decl(verilog_module);

    // メモリバスの生成
    sprintf(verilog_module, "\t// memory bus\n");
    register_top_module_decl(verilog_module);
    if(now_module_tree->is_module_gm_if){
      use_gm_if = 1;

      // __gm_base(in)
      sprintf(label, "__gm_base");
      sprintf(verilog_signal, "%s%s", module_name, label);
      sprintf(verilog_module, "\t.%s(%s),\n", label, verilog_signal);
      register_top_module_decl(verilog_module);
      sprintf(verilog_wire, "wire [31:0] %s;", verilog_signal);
      register_top_signal_tree(label, module_name, label, FLAG_GMEM, SIG_IN, 0, NULL, 0, verilog_signal, verilog_wire);
      // __gm_req(out)
      sprintf(label, "__gm_req");
      sprintf(verilog_signal, "%s%s", module_name, label);
      sprintf(verilog_module, "\t.%s(%s),\n", label, verilog_signal);
      register_top_module_decl(verilog_module);
      sprintf(verilog_wire, "wire %s;", verilog_signal);
      register_top_signal_tree(label, module_name, label, FLAG_GMEM, SIG_OUT, 0, NULL, 0, verilog_signal, verilog_wire);
      // __gm_rnw(out)
      sprintf(label, "__gm_rnw");
      sprintf(verilog_signal, "%s%s", module_name, label);
      sprintf(verilog_module, "\t.%s(%s),\n", label, verilog_signal);
      register_top_module_decl(verilog_module);
      sprintf(verilog_wire, "wire %s;", verilog_signal);
      register_top_signal_tree(label, module_name, label, FLAG_GMEM, SIG_OUT, 0, NULL, 0, verilog_signal, verilog_wire);
      // __gm_done(in)
      sprintf(label, "__gm_done");
      sprintf(verilog_signal, "%s%s", module_name, label);
      sprintf(verilog_module, "\t.%s(%s),\n", label, verilog_signal);
      register_top_module_decl(verilog_module);
      sprintf(verilog_wire, "wire %s;", verilog_signal);
      register_top_signal_tree(label, module_name, label, FLAG_GMEM, SIG_IN, 0, NULL, 0, verilog_signal, verilog_wire);
      // ToDo: 接続されるべき信号を検索してWired OR宣言を生成する。
      // __gm_adrs(out)
      sprintf(label, "__gm_adrs");
      sprintf(verilog_signal, "%s%s", module_name, label);
      sprintf(verilog_module, "\t.%s(%s),\n", label, verilog_signal);
      register_top_module_decl(verilog_module);
      sprintf(verilog_wire, "wire [31:0] %s;", verilog_signal);
      register_top_signal_tree(label, module_name, label, FLAG_GMEM, SIG_OUT, 0, NULL, 0, verilog_signal, verilog_wire);
      // __gm_leng(out)
      sprintf(label, "__gm_leng");
      sprintf(verilog_signal, "%s%s", module_name, label);
      sprintf(verilog_module, "\t.%s(%s),\n", label, verilog_signal);
      register_top_module_decl(verilog_module);
      sprintf(verilog_wire, "wire [1:0] %s;", verilog_signal);
      register_top_signal_tree(label, module_name, label, FLAG_GMEM, SIG_OUT, 0, NULL, 0, verilog_signal, verilog_wire);
      // __gm_di(in)
      sprintf(label, "__gm_di");
      sprintf(verilog_signal, "%s%s", module_name, label);
      sprintf(verilog_module, "\t.%s(%s),\n", label, verilog_signal);
      register_top_module_decl(verilog_module);
      sprintf(verilog_wire, "wire [31:0] %s;", verilog_signal);
      register_top_signal_tree(label, module_name, label, FLAG_GMEM, SIG_IN, 0, NULL, 0, verilog_signal, verilog_wire);
      // ToDo: 接続されるべき信号を検索してWired OR宣言を生成する。
      // __gm_do(out)
      sprintf(label, "__gm_do");
      sprintf(verilog_signal, "%s%s", module_name, label);
      sprintf(verilog_module, "\t.%s(%s),\n", label, verilog_signal);
      register_top_module_decl(verilog_module);
      sprintf(verilog_wire, "wire [31:0] %s;", verilog_signal);
      register_top_signal_tree(label, module_name, label, FLAG_GMEM, SIG_OUT, 0, NULL, 0, verilog_signal, verilog_wire);
    }

    sprintf(verilog_module, "\n");
    register_top_module_decl(verilog_module);

    // メモリアドレスの生成
    sprintf(verilog_module, "\t// base address\n");
    register_top_module_decl(verilog_module);
    {
      now_memory_tree = now_module_tree->memory_tree_ptr;
      while(now_memory_tree != NULL){
        if(now_memory_tree->flag == MEMORY_FLAG_GLOBAL){
          sprintf(label, "%s", convname(sep_p(now_memory_tree->label)));
          sprintf(verilog_module, "\t.__base_%s(__base_%s),\n", label, label);
          register_top_module_decl(verilog_module);
        }
        now_memory_tree = now_memory_tree->next_ptr;
      }
    }
    sprintf(verilog_module, "\n");
    register_top_module_decl(verilog_module);

    // 引数の生成
    sprintf(verilog_module, "\t// arguments\n");
    register_top_module_decl(verilog_module);
    args_num = 0;
    {
      now_memory_tree = now_module_tree->memory_tree_ptr;
      while(now_memory_tree != NULL){
        if(now_memory_tree->flag == MEMORY_FLAG_REGISTER){
          sprintf(label, "__args_%s", convname(sep_p(now_memory_tree->label)));
          sprintf(verilog_signal, "%s%s", module_name, label);
          sprintf(verilog_module, "\t.%s(%s),\n", label, verilog_signal);
          register_top_module_decl(verilog_module);

          if(now_memory_tree->size > 0){
            sprintf(verilog_wire, "wire [%d:0] %s;", now_memory_tree->size*8-1, verilog_signal);
          }else{
            sprintf(verilog_wire, "wire %s;", verilog_signal);
          }
          register_top_signal_tree(label, module_name, convname(sep_p(now_memory_tree->label)), FLAG_ARGS, SIG_IN, now_memory_tree->size, NULL, args_num, verilog_signal, verilog_wire);
          // ToDo: 接続されるべき信号を検索してWired OR宣言を生成する。
          args_num ++;
        }
        now_memory_tree = now_memory_tree->next_ptr;
      }
    }
    sprintf(verilog_module, "\n");
    register_top_module_decl(verilog_module);

    // CALL命令の生成
    sprintf(verilog_module, "\t// call instruction\n");
    register_top_module_decl(verilog_module);
    {
      now_call_tree = now_module_tree->call_tree_ptr;
      while(now_call_tree != NULL){
        now_call_signal_tree = now_call_tree->signal_top_ptr;
        while(now_call_signal_tree != NULL){
          if(
            !strcmp(now_call_signal_tree->signal_name, "req") ||
            !strcmp(now_call_signal_tree->signal_name, "ready") ||
            !strcmp(now_call_signal_tree->signal_name, "done") ||
            !strcmp(now_call_signal_tree->signal_name, "result")
          ){
            sprintf(label, "__call_%s_%s", convname(sep_p(now_call_tree->call_name)), now_call_signal_tree->signal_name);
          }else{
            sprintf(label, "__call_%s_%s_%d", convname(sep_p(now_call_tree->call_name)), now_call_signal_tree->signal_name, now_call_signal_tree->num);
          }
          sprintf(verilog_signal, "%s%s", module_name, label);
          sprintf(verilog_module, "\t.%s(%s),\n", label, verilog_signal);
          register_top_module_decl(verilog_module);
          if(now_call_signal_tree->size > 0){
            sprintf(verilog_wire, "wire [%d:0] %s;", now_call_signal_tree->size-1, verilog_signal);
          }else{
            sprintf(verilog_wire, "wire %s;", verilog_signal);
          }
          if(
            !strcmp(now_call_signal_tree->signal_name, "done") ||
            !strcmp(now_call_signal_tree->signal_name, "ready") ||
            !strcmp(now_call_signal_tree->signal_name, "result")
          ){
            inout = SIG_IN;
          }else{
            inout = SIG_OUT;
          }
          register_top_signal_tree(label, module_name, now_call_signal_tree->signal_name, FLAG_CALL, inout, 0, convname(sep_p(now_call_tree->call_name)), now_call_signal_tree->num, verilog_signal, verilog_wire);
          now_call_signal_tree = now_call_signal_tree->next_ptr;
        }
        sprintf(verilog_module, "\n");
        now_call_tree = now_call_tree->next_ptr;
      }
    }

    sprintf(verilog_module, "\n");
    register_top_module_decl(verilog_module);

    // 戻り値の生成
    sprintf(verilog_module, "\t// return value\n");
    register_top_module_decl(verilog_module);

    {
      now_memory_tree = now_module_tree->memory_tree_ptr;
      while(now_memory_tree != NULL){
        if(now_memory_tree->flag == MEMORY_FLAG_RETURN){
          if(strcmp(now_memory_tree->type, "void")){
            sprintf(verilog_signal, "%s__func_result", module_name);
            sprintf(verilog_module, "\t.%s(%s)\n", "__func_result", verilog_signal);
            register_top_module_decl(verilog_module);

            if(now_memory_tree->size > 0){
              sprintf(verilog_wire, "wire [%d:0] %s;", now_memory_tree->size*8-1,verilog_signal);
            }else{
              sprintf(verilog_wire, "wire %s;", verilog_signal);
            }

            register_top_signal_tree("__func_result", module_name, verilog_signal, FLAG_RETURN, SIG_OUT, now_memory_tree->size, verilog_signal, 0, verilog_signal, verilog_wire);
          }
        }
        now_memory_tree = now_memory_tree->next_ptr;
      }
    }

    sprintf(verilog_module, ");\n");
    register_top_module_decl(verilog_module);

    now_module_tree = now_module_tree->next_ptr;

    free(module_name);
  }

  free(call_signal_name);
  free(verilog_signal);

  return 0;
}

/*!
 * @brief トップモジュールの出力
 */
int output_top_module(FILE *fp, char *topname)
{
  fprintf(fp,"/*\n");
  fprintf(fp," * Copyright (C)2005-2024 AQUAXIS TECHNOLOGY.\n");
  fprintf(fp," *  Don't remove this header.\n");
  fprintf(fp," * When you use this source, there is a need to inherit this header.\n");
  fprintf(fp," *\n");
  fprintf(fp," * This software is released under the MIT License.\n");
  fprintf(fp," * https://opensource.org/license/mit\n");
  fprintf(fp," */\n");

  fprintf(fp,"module %s(\n", topname);

  fprintf(fp,"\tinput  wire       system_clock,\n");
  fprintf(fp,"\tinput  wire       system_reset,\n");
  fprintf(fp,"\tinput  wire       __func_start,\n");
  fprintf(fp,"\toutput wire       __func_done,\n");
// fprintf(fp,"\toutput wire         __func_idle,\n");
  fprintf(fp,"\toutput wire       __func_ready,\n\n");

  if(use_gm_if){
    fprintf(fp,"\tinput  wire [31:0] __gm_base,\n");
    fprintf(fp,"\toutput wire        __gm_req,\n");
    fprintf(fp,"\toutput wire        __gm_rnw,\n");
    fprintf(fp,"\tinput  wire        __gm_done,\n");
    fprintf(fp,"\toutput wire [31:0] __gm_adrs,\n");
    fprintf(fp,"\toutput wire [ 1:0] __gm_leng,\n");
    fprintf(fp,"\tinput  wire [31:0] __gm_di,\n");
    fprintf(fp,"\toutput wire [31:0] __gm_do,\n\n");
  }

  if(use_llvm_memcpy){
    fprintf(fp,"\toutput wire        __llvm_memcpy_req,\n");
    fprintf(fp,"\tinput  wire        __llvm_memcpy_ready,\n");
    fprintf(fp,"\tinput  wire        __llvm_memcpy_done,\n");
    fprintf(fp,"\toutput wire [ 7:0] __llvm_memcpy_args_0,\n");
    fprintf(fp,"\toutput wire [ 7:0] __llvm_memcpy_args_1,\n");
    fprintf(fp,"\toutput wire [31:0] __llvm_memcpy_args_2,\n");
    fprintf(fp,"\toutput wire [31:0] __llvm_memcpy_args_3,\n");
    fprintf(fp,"\toutput wire [31:0] __llvm_memcpy_args_4,\n\n");
  }

  // ToDo:
  // 外部信号を入れる
  fprintf(fp,"\t// global signal\n");
  create_top_global(fp);
  create_top_nowire(fp, topname);
  fprintf(fp,");\n");
  fprintf(fp,"\n");

  // Wire宣言を入れる
  fprintf(fp,"// wire\n");
  create_top_wire(fp);
  fprintf(fp,"\n");

  fprintf(fp,"// connection\n");
  fprintf(fp,"%s", top_module_assign);

  fprintf(fp,"// system signal\n");
  create_top_nofunc(fp);
  fprintf(fp,"\n");

  fprintf(fp,"// modules\n");
  fprintf(fp,"%s", top_module_decl);

  fprintf(fp,"endmodule\n");

  return 0;
}

/*!
 * @brief  ツリーの表示
 *
 */
int print_top_signal_tree(FILE *fp)
{
  TOP_SIGNAL_TREE *now_top_signal_tree;

  fprintf(fp,"==============================\n");
  fprintf(fp,"Print - Top Signal Tree\n");
  fprintf(fp,"==============================\n");
  now_top_signal_tree = top_signal_tree_top;
  while(1){
    if(now_top_signal_tree == NULL) break;

    fprintf(fp, "Label:          %s\n", now_top_signal_tree->label);
    fprintf(fp, "Size:           %d\n", now_top_signal_tree->size);
    fprintf(fp, "InOut:          %d\n", now_top_signal_tree->inout);
    fprintf(fp, "Used:           %d\n", now_top_signal_tree->used);
    fprintf(fp, "Module_Name:    %s\n", now_top_signal_tree->module_name);
    fprintf(fp, "Flag:           %d\n", now_top_signal_tree->flag);
    fprintf(fp, "Name:           %s\n", now_top_signal_tree->name);
    fprintf(fp, "Call_Name:      %s\n", now_top_signal_tree->call_name);
    fprintf(fp, "Args_Num:       %d\n", now_top_signal_tree->args_num);
    fprintf(fp, "Verilog_Signal: %s\n", now_top_signal_tree->verilog_signal);
    fprintf(fp, "Verilog_Wire:   %s\n", now_top_signal_tree->verilog_wire);

    now_top_signal_tree = now_top_signal_tree->next_ptr;
    fprintf(fp,"==============================\n");

  }

  fprintf(fp,"==============================\n");

  return 0;
}
