/*
 * Copyright (C)2005-2024 AQUAXIS TECHNOLOGY.
 *  Don't remove this header.
 * When you use this source, there is a need to inherit this header.
 *
 * This software is released under the MIT License.
 * https://opensource.org/license/mit
 */

/*!
 * @file  common.c
 * @brief  共通関数
 * @author  Hidemi Ishiahra
 *
 * @note
 * 共通関数を置く
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>

#include "common.h"

/*!
 * @brief  ビットサイズを返す
 */
int get_width(char *buf)
{
  int size = 0;
  
  if(!strcmp(buf, "void")){
    size = 0;
  }else if(!strcmp(buf, "i1")){
    size = 1;
  }else if(!strcmp(buf, "i8")){
    size = 8;
  }else if(!strcmp(buf, "i16")){
    size = 16;
  }else if(!strcmp(buf, "i32")){
    size = 32;
  }else if(buf[strlen(buf)-1] == '*'){
    // ポインタ宣言(ポインタは最後に'*'が付く)
    // ポインタはi8*, i16*, i32*, %構造体名*, ARRAY宣言*, を想定
    size = 32;
  }else if(!strcmp(buf, "ptr")){
    // ポインタ宣言(ポインタは最後に'*'が付く)
    // ポインタはi8*, i16*, i32*, %構造体名*, ARRAY宣言*, を想定
    size = 32;
  }else{
    /*
     * ToDo:
     * ちゃんと処理すること。いまのところ暫定だよ。
     */
    size = 32;
    printf("[WARNING] get_width(): %s¥n", buf);
  }

  return size;
}

/*!
 * @brief  文字列の領域取得と登録
 */
char *charalloc(char *in)
{
  char *buf;

  buf = calloc(strlen(in)+1,1);
  strcpy(buf, in);

  return (char *)buf;
}

/*!
 * @brief  変数名の変換
 *
 * @note
 * 変数名の先頭の'%', '@'を取り除く
 * 変数の'.'を'_'に置き換える
 */
char *convname(char *buf)
{
  int i;
  if((buf[0] == '%') || (buf[0] == '@')){
    for(i=0;i<strlen(buf)-1;i++){
      buf[i] = buf[i+1];
    }
    buf[strlen(buf)-1] = 0;
  }
  
  for(i=0;i<strlen(buf);i++){
    if(buf[i] == '.') buf[i] = '_';
  }

  return buf;
}

/*!
 * @brief  タイプの変換
 *
 * @note
 * タイプの末尾の'*'を削除する
 */
char *convtype(char *buf)
{
  if(buf[strlen(buf)-1] == '*'){
    buf[strlen(buf)-1] = 0;
  }
  return buf;
}

#ifdef DEBUG
int dbgvprintf(char const *format, va_list ap)
{
  int ret = 0;

    static FILE	*logfile;

    if (logfile == NULL) {
      char ctmp[256];

      sprintf(ctmp, "debug-%ld.log", (long)getpid());
      logfile = fopen(ctmp, "w");
      assert(logfile != NULL);
    }
    vfprintf(stdout, format, ap);
    ret = vfprintf(logfile, format, ap);
    fflush(logfile);

  return ret;
}
#endif

int dbgprintf(const char *format, ...)
{
  int ret = 0;
#ifdef DEBUG
  va_list ap;

  va_start(ap, format);
  ret = dbgvprintf(format, ap);
  va_end(ap);
#endif
  return ret;
}
