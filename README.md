# AiStackChanEx

=====================================================

AiStackChanEx Ver1.02 2023-04-25

ボタンＡ、Ｃの機能に対応する外部インターフェースを設けました。
スタックチャンをケースに入れた時や離れた場所からの操作が可能になります。
Ver1.01 でボタンＢの機能に対応しています。


<使用方法>
※IPアドレスを「192.168.0.100」として記述していますが、各自読み替えてください。
------------------------------------------------------------------------

〇randomSpeak (ボタンA：左側の機能)
　：独り言モードのＯＮ／ＯＦＦ

独り言モードが ON となります。

http://192.168.0.100/randomSpeak?mode=on


独り言モードが OFF となります。

http://192.168.0.100/randomSpeak?mode=off




〇speakSelfIntro (ボタンC：左側の機能)
　：スタックチャンが自己紹介します。

http://192.168.0.100/speakSelfIntro






=======================================================

AiStackChanEx Ver1.01 2023-04-18

RoboさんのVer0.0.7 のソフトをベースにのんちらさんの機能を統合しました。
タイマー機能とLEDピカピカが楽しいです。

独自に次の変更をしています。
・タイマーの時間設定ができる。
・外部からタイマー起動/停止ができる。
・独り言開始ボタンと被らないように、
 タイマー開始/停止ボタンは、A(左)からB(中央)に移動。
 「独り言」中にも、タイマーは有効です。

M5Stack Core2 for AWS 専用のソフト

<使用方法>
------------------------------------------------------------------------
外部インターフェースについて記載します。
※IPアドレスを「192.168.0.100」として記述していますが、各自読み替えてください。

〇timer

　：タイマーの時間設定できます。

30秒から60分未満で秒単位の値を指定してください。
setTime= 30 - 3599(単位：秒)   

http://192.168.0.100/timer?setTime=40


setTimeを指定しないと初期値(=180秒)にタイマーの時間を戻します。

http://192.168.0.100/timer



〇timerGo 　(ボタンＢ：中央の機能)

　：タイマーの開始と時間設定が同時にできます。

タイマーの時間設定して、タイマーを開始します。
時間設定は、上記のtimerコマンドと同じく、30秒から60分未満で秒単位の値を指定してください。

http://192.168.0.100/timerGo?setTime=90


setTimeを指定しないと、事前の設定値でタイマーが開始します。

http://192.168.0.100/timerGo



〇timerStop 　(ボタンＢ：中央の機能)

　：実行中のタイマーを停止ます。

http://192.168.0.100/timerStop



〇version

　：ソフトウエアのバージョン情報を出力します。

http://192.168.0.100/version

  -> AiStackChanEx Ver1.01 2023-04-18

==============================================================
