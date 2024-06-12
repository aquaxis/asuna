# ASUNA(Algorithmic Synthesis for Unified Netlist Automation)

ASUNA は C 言語から VerilogHDL を生成する高位合成ツールです。

## ライセンス

* MIT を適用します

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

ASUNA は clang を使用するので事前に clang をインストールします。

```
$ sudo apt install clang
```

```
$ ./asuna CSOURCE_FINELANE TOPMODULENAME
```

TOPMODULENAME_top.v
FUNCTION.v
memory_map.txt

# 処理の流れ

1. 入力された C 言語のソースコードを関数単位に分離
2. LLVM で LLVM-IR を出力
3. LLVM-IR を VerilogHDL に変換

# 将来の予定

* AXI バス
* 浮動小数点処理
