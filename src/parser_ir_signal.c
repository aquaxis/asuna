/*
 * Copyright (C)2005-2024 AQUAXIS TECHNOLOGY.
 *  Don't remove this header.
 * When you use this source, there is a need to inherit this header.
 *
 * This software is released under the MIT License.
 * https://opensource.org/license/mit
 */

/*!
 * @file  parser_ir_signal.c
 * @brief  信号定義ツリー
 * @author  Hidemi Ishiahra
 *
 * 関数内の信号定義ツリー構築
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

extern PARSER_TREE_IR *parser_tree_ir_top;
extern PARSER_TREE_IR *parser_tree_ir_current;
extern PARSER_TREE_IR *parser_tree_ir_prev;

typedef struct signal_tree{
    struct signal_tree  *prev_ptr;
    struct signal_tree  *next_ptr;
    enum SIGNAL_FLAG  flag;
    char        *label;    // ラベル
    int          width;
    char        *type;
    char        *verilog;
} SIGNAL_TREE;

SIGNAL_TREE *signal_tree_top        = NULL;

/**
 * @brief  信号ツリー削除
 */
int clean_signal_tree()
{
  SIGNAL_TREE *now_signal_tree;
  SIGNAL_TREE *new_signal_tree;

    now_signal_tree = signal_tree_top;
    while(now_signal_tree != NULL){
        new_signal_tree = now_signal_tree->next_ptr;
        if(now_signal_tree->label != NULL) free(now_signal_tree->label);
        if(now_signal_tree->verilog != NULL) free(now_signal_tree->verilog);
        free(now_signal_tree);
        now_signal_tree = new_signal_tree;
    }
    signal_tree_top = NULL;
    return 0;
}

/*!
 * @brief  信号の登録
 *
 * @todo
 * 定義を厳密にしていないので不具合に繋がるかも
 */
int register_signal_tree(char *label, char *type, int flag)
{
  SIGNAL_TREE *now_signal_tree = NULL;
  SIGNAL_TREE *old_signal_tree = NULL;
  int width = 0;

//  insert_signal_tree();


  now_signal_tree = signal_tree_top;
  while(now_signal_tree != NULL){
    old_signal_tree = now_signal_tree;
    now_signal_tree = now_signal_tree->next_ptr;
  }

  now_signal_tree        = (SIGNAL_TREE *)calloc(sizeof(SIGNAL_TREE),1);
  now_signal_tree->prev_ptr  = old_signal_tree;
  now_signal_tree->next_ptr  = NULL;

  if(signal_tree_top == NULL){
    signal_tree_top        = now_signal_tree;
  }else{
    old_signal_tree->next_ptr  = now_signal_tree;
  }


  now_signal_tree->label = charalloc(label);
  now_signal_tree->flag = flag;
  now_signal_tree->type = charalloc(type);
  width = get_width(type);
#if 0
  {
    if(!strcmp(type, "i1")){
      /*
       * i1は基本的にicmp用
       */
      signal_tree_current->width = 1;
    }else if(!strcmp(type, "i8") || !strcmp(type, "i8*")){
      signal_tree_current->width = 8;
    }else if(!strcmp(type, "i16") || !strcmp(type, "i16*")){
      signal_tree_current->width = 16;
    }else if(!strcmp(type, "i32") || !strcmp(type, "i32*") ||
      !strcmp(type, "i8**") || !strcmp(type, "i16**") || !strcmp(type, "i32**")
    ){
      /*
       * グローバルメモリは32bitで取り扱う
       */
      signal_tree_current->width = 32;
    }else{
      /*
       * ToDo:
       * 判別していないタイプがあるので不具合につながる可能性がある。
       */
      signal_tree_current->width = 32;
      printf("[WARNING] register_signal_tree(): unknown: label=%s, type=%s\n", label, type);
    }
  }
#endif
  now_signal_tree->width = width;
  
  return 0;
}

/*!
 *  @brief  信号のVerilogHDL生成
 */
int create_verilog_signal()
{
  SIGNAL_TREE *now_signal_tree;
  char *buf;
  char *str;

  buf = calloc(STR_MAX, 1);

    now_signal_tree = signal_tree_top;
    while(now_signal_tree != NULL){
    strcpy(buf ,"");
    if(
      (now_signal_tree->flag == SIGNAL_FLAG_REG) ||
      (now_signal_tree->flag == SIGNAL_FLAG_WIRE)
    ){
      switch(now_signal_tree->flag){
        case SIGNAL_FLAG_REG:
          sprintf(buf, "reg ");
          break;
        case SIGNAL_FLAG_WIRE:
          sprintf(buf, "wire ");
          break;
        default:
          break;
      }
      if(now_signal_tree->width > 0){
        sprintf(buf, "%s[%d:0] ", buf, now_signal_tree->width-1);
      }
      str = regalloc(now_signal_tree->label);
      sprintf(buf, "%s%s;", buf, str);
      free(str);
      now_signal_tree->verilog = charalloc(buf);
    }

        now_signal_tree = now_signal_tree->next_ptr;
    }

    free(buf);

    return 0;
}

/*!
 * @brief  信号のVerilogHDL出力
 */
int output_signal_tree(FILE *fp)
{
  SIGNAL_TREE *now_signal_tree;

    now_signal_tree = signal_tree_top;
    while(now_signal_tree != NULL){
    switch(now_signal_tree->flag){
      case SIGNAL_FLAG_REG:
      case SIGNAL_FLAG_WIRE:
        fprintf(fp, "%s\n", now_signal_tree->verilog);
        break;
      case SIGNAL_FLAG_RAM:
        /*
         * RAMの直接的なVerilogHDLコードは存在しない。
         */
        break;
      default:
        break;
    }

        now_signal_tree = now_signal_tree->next_ptr;
    }

    return 0;
}

/*!
 * @brief  信号のデバッグ出力
 */
int print_signal_tree(FILE *fp)
{
  SIGNAL_TREE *now_signal_tree;

    fprintf(fp,"==============================\n");
    fprintf(fp,"Print Signal\n");
    fprintf(fp,"==============================\n");
    now_signal_tree = signal_tree_top;
    while(now_signal_tree != NULL){
    fprintf(fp, "%s: ", now_signal_tree->label);
    switch(now_signal_tree->flag){
      case SIGNAL_FLAG_REG:
        fprintf(fp, "Reg, ");
        fprintf(fp, "width=%d,", now_signal_tree->width);
        fprintf(fp, "verilog=%s", now_signal_tree->verilog);
        break;
      case SIGNAL_FLAG_WIRE:
        fprintf(fp, "Wire, ");
        fprintf(fp, "width=%d,", now_signal_tree->width);
        fprintf(fp, "verilog=%s", now_signal_tree->verilog);
        break;
      case SIGNAL_FLAG_RAM:
        fprintf(fp, "RAM, ");
        fprintf(fp, "type=%s,", now_signal_tree->type);
        fprintf(fp, "verilog=%s", now_signal_tree->verilog);
        break;
      default:
        break;
    }
    fprintf(fp, "\n");

        now_signal_tree = now_signal_tree->next_ptr;
    }

    fprintf(fp,"==============================\n");

    return 0;
}

