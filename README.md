# AiStackChanEx
AiStackChanEx Ver1.01 2023-04-18

RoboさんのVer0.0.7 のソフトをベースにのんちらさんの機能を統合しました。
タイマー機能とLEDピカピカが楽しいです。

独自に次の変更をしています。
・タイマーの時間設定ができる。
・外部からタイマー起動/停止ができる。
・独り言開始ボタンと被らないように、タイマー開始/停止ボタンは、A(左)からB(中央)に移動。

M5Stack Core2 for AWS 専用のソフト

<使用方法>

------------------------------------------------------------------------
外部インターフェースについて記載します。
※IPアドレスを「192.168.0.100」として記述していますが、各自読み替えてください。

〇timer

タイマーの時間設定ができます。 30秒から60分未満で秒単位の値を指定してください。
setTime= 30 - 3599(秒)   
http://192.168.0.100/timer?setTime=40

setTimeを指定しないと初期値(=180秒)にタイマーの時間を戻します。
http://192.168.0.100/timer


〇timerGo

タイマーの時間設定して、タイマーを開始します。
タイマーの時間設定は、上記のtimerコマンドと同じく、30秒から60分未満で秒単位の値を指定してください。
http://192.168.0.100/timerGo?setTime=90

setTimeを指定しないと、事前の設定時間で開始します。
http://192.168.0.100/timerGo


〇timerStop

実行中のタイマーを停止ます。
http://192.168.0.100/timerStop


〇version

ソフトウエアのバージョン情報を出力します。
http://192.168.0.100/version
  -> AiStackChanEx Ver1.01 2023-04-18
