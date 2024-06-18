/*
 * Copyright (C)2005-2024 AQUAXIS TECHNOLOGY.
 *  Don't remove this header.
 * When you use this source, there is a need to inherit this header.
 *
 * This software is released under the MIT License.
 * https://opensource.org/license/mit
 */

/*! @file
 * @brief asuna - C to Verilog Converter
 * @author Hidemi Ishihara <hidemi(at)aquaxis.com>
 */
#include <stdio.h>
#include <unistd.h>
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

char *topname;
int debug_mode = 0;

/*!
 * @brief  Cソースコードの分離
 */
int split_c_source(char *filename)
{
  int function_count;
  int i;
  char *llvm_command;

  printf("[C Source Parser Start]\n");

  read_file(filename);                // ファイルの読み込みとトークン取得
  set_token_order_current(get_token_order_top());  // トークンのポインタをトップへ戻す

  parser_c_source();                  // 構文解析
  function_count = create_proc_source();        // Processの処理
  create_header_source();                // Lineの処理

  // トークンオーダーをクリーンナップする
  clean_token_order();

  printf(" -> Function Count: %d\n", function_count);

  /*
   * 分離したCソースコードをLLVMでコンパイルする
   */
  printf("[LLVM Compile Start]\n");
  llvm_command = malloc(256);

  for(i = 0; i < function_count; i++){
    sprintf(llvm_command, "clang -S -m32 -O3 -emit-llvm __%03d.c", i);
    system(llvm_command);
  }

  free(llvm_command);

  return function_count;
}

/*!
 * @brief  関数の解析処理
 */
int process_function(char *name)
{
  FILE *fp;
  char *line, *token;
  char *filename, *oldname;

  printf("[LLVM-IR Function Parser Start]\n");

  line = calloc(STR_MAX,1);
  token = calloc(STR_MAX,1);
  filename = calloc(STR_MAX,1);
  oldname = calloc(STR_MAX,1);

  sprintf(filename, "__%s.ll", name);

  if((fp = fopen(filename,"rb")) == NULL){
    printf("can't open file.\n");
    free(line);
    free(token);
    exit(1);
  }
  /*
   * LLVM-IRはラインごとにparseしていく
   */
  clean_signal_tree();
  clean_struct_tree();
  while(!feof(fp)){
    get_line(line, fp);
    do{
      if((line[strlen(line)-1] == '\n') || (line[strlen(line)-1] == '\r')){
        line[strlen(line)-1] = '\0';
      }
    }while((line[strlen(line)-1] == '\n') || (line[strlen(line)-1] == '\r'));
    parser_ir_source(line);
    strcpy(line, "\0");  // LLVM-IRの場合、1ラインで切り上げるので削除対象とする
  }
  fclose(fp);

  // グローバル構造体の解析
  parser_struct_tree();       // structタイプの解析

  create_stage_parser_tree(); // ステージング

  parser_memory_tree();       // メモリの解析
  deploy_array_type();        // メモリタイプの展開する
  create_array_size();        // メモリサイズの取得(アドレス生成)

  create_verilog_proc_tree(); // 実行プロセスのVerilogHDL化
  create_verilog_signal();    // 信号のVerilogHDL化
  create_verilog_state();     // ステートマシン生成
  create_verilog_label();     // ラベルの生成

#if 0
#ifdef DEBUG
  print_array_type();             // メモリ一覧の表示
  print_call_tree(call_tree_top); // CALL命令の表示
#endif
#endif

#if DEBUG
  sprintf(filename, "%s.log", &module_name[1]);
  if((fp = fopen(filename,"w")) == NULL){
    printf("can't open file.\n");
    free(line);
    free(token);
    exit(1);
  }
  print_signal_tree(fp);
  print_parser_tree_ir(fp);
  print_proc_tree(fp);
  fclose(fp);
#endif

  sprintf(filename, "%s.v", &module_name[1]);
  if((fp = fopen(filename,"w")) == NULL){
    printf("can't open file.\n");
    free(line);
    free(token);
    exit(1);
  }
  output_proc_tree(fp); // VerilogHDLの出力
  fclose(fp);

  // モジュールの登録
  register_module_tree(module_name);
  new_module_tree();

  if(debug_mode){
    sprintf(oldname, "__%s.ll", name);
    sprintf(filename, "__%s.ll", &module_name[1]);
    printf("%s -> %s\n", oldname, filename);
    rename(oldname, filename);

    sprintf(oldname, "__%s.c", name);
    sprintf(filename, "__%s.c", &module_name[1]);
    printf("%s -> %s\n", oldname, filename);
    rename(oldname, filename);
  }

  clean_parser_tree_ir();
  clean_proc_tree();

  free(line);
  free(token);
  free(filename);
  free(oldname);

  return 0;
}

/*!
 * @brief  Process
 */
int process(char *csource)
{
  FILE *fp;
  int function_count;
  char *filename;
  int i;

  /*
   * Cソースコードを分離する
   */
  function_count = split_c_source(csource);

  /*
   * 解析
   * ToDo:
   *   最初にグローバルメモリを解析し、アドレスを確定しておく
   */
  for(i = 0; i < function_count; i++){
    filename = malloc(STR_MAX);
    sprintf(filename, "%03d", i);
    process_function(filename);  // 関数の解析
    free(filename);
  }

#if DEBUG
  // モジュールツリーの表示
  filename = malloc(STR_MAX);
  sprintf(filename, "module_list.txt");

  if((fp = fopen(filename,"w+")) == NULL){
    printf("can't open file.\n");
    free(filename);
    exit(1);
  }
  print_module_tree(fp);
  fclose(fp);
#endif

  // メモリマップ出力
  filename = malloc(STR_MAX);
  sprintf(filename, "memory_map.txt");

  if((fp = fopen(filename,"w+")) == NULL){
    printf("can't open file.\n");
    free(filename);
    exit(1);
  }
  print_memmap_tree(fp);      // メモリマップの出力
  fclose(fp);
  clean_memmap_tree();      // メモリマップの削除

  // 最上位モジュールの生成
  create_top_module();
  create_top_assign();

#if DEBUG
  sprintf(filename, "%s.log", topname);
  if((fp = fopen(filename,"w")) == NULL){
    printf("can't open file.\n");
    exit(1);
  }
  print_top_signal_tree(fp);
  fclose(fp);
#endif

  sprintf(filename, "%s.v", topname);
  if((fp = fopen(filename,"w+")) == NULL){
    printf("can't open file.\n");
    exit(1);
  }
  output_top_module(fp, topname); // 最上位モジュールの出力
  fclose(fp);

  for(i = 0; i < function_count; i++){
    sprintf(filename, "__%03d.c", i);
    remove(filename);
    sprintf(filename, "__%03d.ll", i);
    remove(filename);
  }

  remove("__extern.c");
  remove("__extern.h");

  free(filename);

  return 0;
}

/*!
 * @brief  ヘルプの表示
 */
void help()
{
  printf("Asuna Rev %d.%02d\n", REVISION_MAIN, REVISION_MANOR);
  printf(" Algorithmic Synthesis for Unified Netlist Automation\n");
  printf(" C Source to Verilog-HDL Convertor\n");
  printf(" (c) AQUAXIS TECHNOLOGY>\n");
  printf(" (c) Hidemi Ishihara <hidemi(at)aquaxis.com>\n");
  printf("\n");
  printf("Usage:\n");
  printf(" asuna [-d] [-h] -f C_SOURCE_FILENAME\n");
  printf("  -d: Debug Mode(output a debug file)\n");
  printf("  -h: This message\n");
}

extern char* optarg;
extern int optind, opterr, optopt;

/*!
 * @brief  メイン関数
 */
int main(int argc,char *argv[])
{
  char *filename;

  // 引数の解析
  int opt;
  opterr = 0; // getopt()のエラーメッセージを無効にする。
  while ((opt = getopt(argc, argv, "fhd")) != -1) {
    // コマンドライン引数のオプションがなくなるまで繰り返す
    switch (opt) {
      case 'f':
        filename = argv[optind++];
        break;
      case 'd':
        debug_mode = 1;
        break;
      case 'h':
        help();
        exit(0);
        break;
      default:
        help();
        exit(0);
        break;
    }
  }

  // トップファイル名の取得
  topname = calloc(strlen(filename) +10, 1);
  strcpy(topname, filename);
  topname[strlen(topname)-2] = 0;
  strcat(topname, "_top");
  printf("topname: %s\n", topname);

  // 高位合成の実行
  process(filename);

  exit(0);
}
