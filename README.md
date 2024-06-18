# ASUNA(Algorithmic Synthesis for Unified Netlist Automation)

## LICENSE

https://github.com/aquaxis/asuna/LICENSE

* MIT を適用します

## 概要

ASUNA は C 言語から VerilogHDL を生成する高位合成ツールです。

## コンセプト

* C 言語のみ対応
* 単純に C 言語を VerilogHDL に変換します
* 関数 ＝ モジュール

# 制約

## 一般的な制約

* C 言語のみ対応
* LLVM のフロントエンドでもなく、バックエンドでもありません
* 入力できる C 言語ソースコードは 1 ファイルのみです
* ソースコードのコメントは日本語 (UTF-8) です

## ソースコードに対する制約

 * 複雑な構造体は処理できないかもしれません
 * マクロは展開できない可能性が大きいです
 * 浮動小数点はサポートしていません(現時点では・・・)
 * 除算を使うと莫大な回路を生成します
 * 構造体は4Byteアライメントになるようにしてください
 * メモリを外出ししているので初期化が出来ません
 * 初期化や定数を設定するなどする場合は外から行う必要があります

## その他の制約

 * 出力したVerilog HDLの動作、性能などは一切、保証しません
 * 正常にVerilog HDLが生成されなくても文句は言わないで下さい
 * ソースコードが汚いのは許して下さい
 * メモリーリークしたらごめんなさい
 * LLVMのStandard C Libraryは使用頻度の高いものから対応になります
 * ソースコード内のメモリは一元管理になります

# ビルド手順

ASUNA は LLVM/clang を使用するので事前に LLVM/clang をインストールします。

## LLVM のビルド

```
$ LLVM_VERSION=llvmorg-18.1.7
$ git clone https://github.com/llvm/llvm-project.git -b ${LLVM_VERSION}
$ cd llvm-project
$ mkdir build
$ cd build/
$ cmake -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release -DLLVM_TARGETS_TO_BUILD="RISCV" -DCMAKE_INSTALL_PREFIX=${HOME}/${LLVM_VERSION} -G "Unix Makefiles" ../llvm
$ make -j`nproc`
$ make install
```

## ASUNA のビルド

```
$ git clone https://github.com/aquaxis/asuna.git
$ cd asuna
$ make asuna
$ cp build/asuna ./
```

# 使い方


```
$ ./asuna -f example.c
```

## 引数

```
$ asuna -f <FILENAME> [-d] [-h]
```

| 引数 | 概要 |
|------|------|
| -f   | 高位合成するファイル名を指定 |
| -d   | デバッグモード               |
| -h   | ヘルプの表示                 |

## 生成ファイル

FILENAME.c のファイルに対してつぎのファイルを生成します。

* FILENAME_top.v
* FUNCTION0.v
* memory_map.txt

FUNCTION.v は FILENAME.v の FILENAME 関数から辿れる関数名ごとにファイルが生成されます。

## 制約

* 最上位関数は FILENAME.c の FILENAME と同一にする必要がある


## 処理の流れ

1. 入力された C 言語のソースコードを関数単位に分離
2. LLVM で LLVM-IR を出力
3. LLVM-IR を VerilogHDL に変換

## 将来の予定

* AXI バス
* 浮動小数点処理
