// -----------------  AiStackChanEx Ver1.10 by NoRi ----------------------
const char *EX_VERSION = "AiStackChanEx_v110-230616";
#define USE_EXTEND
// -----------------------------------------------------------------------
// Extended from
//  AI_StackChan2                      : 2023-05-31  Robo8080さん
//  M5Unified_StackChan_ChatGPT_Google : 2023-05-24  Robo8080さん
//  M5Unified_StackChan_ChatGPT_Global : 2023-04-28  Robo8080さん
//  ai-stack-chan_wifi-selector        : 2023-04-22  ひろきち821さん
//  M5Unified_StackChan_ChatGPT v007   : 2023-04-16  Robo8080さん
//  AI-StackChan-GPT-Timer             : 2023-04-07  のんちらさん
//  ----------------------------------------------------------------------

#include <Arduino.h>
#include <SD.h>
#include <SPIFFS.h>
#include <M5Unified.h>
#include <nvs.h>
#include <Avatar.h>
// #define USE_DOGFACE
#ifdef USE_DOGFACE
#include <faces/DogFace.h>
#endif
#include <AudioOutput.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include "AudioFileSourceHTTPSStream.h"
#include "AudioFileSourceVoiceTextStream.h"
#include "AudioOutputM5Speaker.h"

#include <AudioFileSourcePROGMEM.h>
#include <google-tts.h>
#include <ServoEasing.hpp> // https://github.com/ArminJo/ServoEasing
#include "WebVoiceVoxTTS.h"

#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "rootCACertificate.h"
#include "rootCAgoogle.h"

#include <ArduinoJson.h>
#include <ESP32WebServer.h>
#include <ESPmDNS.h>
#include <deque>

const int MAX_HISTORY = 5;      // 保存する質問と回答の最大数
std::deque<String> chatHistory; // 過去の質問と回答を保存するデータ構造

// #define VOICEVOX_APIKEY "SET YOUR VOICEVOX APIKEY"
// #define STT_APIKEY "SET YOUR STT APIKEY"

//----------------------------------------------

/// set M5Speaker virtual channel (0-7)
static constexpr uint8_t m5spk_virtual_channel = 0;
using namespace m5avatar;
Avatar avatar;
const Expression expressions_table[] = {
    Expression::Neutral,
    Expression::Happy,
    Expression::Sleepy,
    Expression::Doubt,
    Expression::Sad,
    Expression::Angry};

ESP32WebServer server(80);

// *** [warning対策02] ***
char tts_parms1[] = "&emotion_level=4&emotion=happiness&format=mp3&speaker=takeru&volume=200&speed=100&pitch=130";
char tts_parms2[] = "&emotion=happiness&format=mp3&speaker=hikari&volume=200&speed=120&pitch=130";
char tts_parms3[] = "&emotion=anger&format=mp3&speaker=bear&volume=200&speed=120&pitch=100";
char tts_parms4[] = "&emotion_level=2&emotion=happiness&format=mp3&speaker=haruka&volume=200&speed=80&pitch=70";
char tts_parms5[] = "&emotion_level=4&emotion=happiness&format=mp3&speaker=santa&volume=200&speed=120&pitch=90";
char tts_parms6[] = "&emotion=happiness&format=mp3&speaker=hikari&volume=150&speed=110&pitch=140";
// char *tts_parms_table[6] = {tts_parms1, tts_parms2, tts_parms3, tts_parms4, tts_parms5, tts_parms6};
char *tts_parms_table[] = {tts_parms1, tts_parms2, tts_parms3, tts_parms4, tts_parms5, tts_parms6};

// TTS0(VoiceText)をChatGPTで使用する場合の感情表現用
String EXPRESSION_STRING[] = {"Neutral", "Happy", "Sleepy", "Doubt", "Sad", "Angry", ""};
int EXPRESSION_INDX = -1;

String tts_parms01 = "&format=mp3&speaker=takeru&volume=200&speed=100&pitch=130";
String tts_parms02 = "&format=mp3&speaker=hikari&volume=200&speed=120&pitch=130";
String tts_parms03 = "&format=mp3&speaker=bear&volume=200&speed=120&pitch=100";
String tts_parms04 = "&format=mp3&speaker=haruka&volume=200&speed=80&pitch=70";
String tts_parms05 = "&format=mp3&speaker=santa&volume=200&speed=120&pitch=90";
String tts_parms06 = "&format=mp3&speaker=hikari&volume=150&speed=110&pitch=140";
// String TTS_PARMS_STR[6] = {tts_parms01, tts_parms02, tts_parms03, tts_parms04, tts_parms05, tts_parms06};
String TTS_PARMS_STR[] = {tts_parms01, tts_parms02, tts_parms03, tts_parms04, tts_parms05, tts_parms06};
int TTS_PARMS_NO = 1;

String EMOTION_PARAMS[] = {
    "&emotion_level=2&emotion=happiness",
    "&emotion_level=3&emotion=happiness",
    "&emotion_level=2&emotion=sadness",
    "&emotion_level=1&emotion=sadness",
    "&emotion_level=4&emotion=sadness",
    "&emotion_level=4&emotion=anger"};
int TTS_EMOTION_NO = 0;

String random_words[18] = {"あなたは誰", "楽しい", "怒った", "可愛い", "悲しい", "眠い", "ジョークを言って", "泣きたい", "怒ったぞ", "こんにちは", "お疲れ様", "詩を書いて", "疲れた", "お腹空いた", "嫌いだ", "苦しい", "俳句を作って", "歌をうたって"};
int RANDOM_TIME = -1;
static int LASTMS1 = 0;

String INIT_BUFFER = "";
String Role_JSON = "";
String SPEECH_TEXT = "";
String SPEECH_TEXT_BUFFER = "";
DynamicJsonDocument CHAT_DOC(1024 * 10);
String json_ChatString = "{\"model\": \"gpt-3.5-turbo\",\"messages\": [{\"role\": \"user\", \"content\": \""
                         "\"}]}";

// C++11 multiline string constants are neato...
static const char HEAD[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html lang="ja">
<head>
  <meta charset="UTF-8">
  <title>StackChan</title>
</head>)KEWL";

static const char APIKEY_HTML[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <title>StackChan</title>
  </head>
  <body>
    <h1>API Key Settings</h1>
    <form>
      <label for="role1">OpenAI API Key</label>
      <input type="text" id="openai" name="openai" oninput="adjustSize(this)"><br>

      <label for="role2">VoiceText API Key</label>
      <input type="text" id="voicetext" name="voicetext" oninput="adjustSize(this)"><br>

      <label for="role3">VoiceVox API Key</label>
      <input type="text" id="voicevox" name="voicevox" oninput="adjustSize(this)"><br>

      <button type="button" onclick="sendData()">Submit</button>
    </form>
    <script>
      function adjustSize(input) {
        input.style.width = ((input.value.length + 1) * 8) + 'px';
      }
      function sendData() {
        // FormDataオブジェクトを作成
        const formData = new FormData();

        // 各ロールの値をFormDataオブジェクトに追加
        const openaiValue = document.getElementById("openai").value;
        if (openaiValue !== "") formData.append("openai", openaiValue);

        const voicetextValue = document.getElementById("voicetext").value;
        if (voicetextValue !== "") formData.append("voicetext", voicetextValue);

        const voicevoxValue = document.getElementById("voicevox").value;
        if (voicevoxValue !== "") formData.append("voicevox", voicevoxValue);

	    // POSTリクエストを送信
	    const xhr = new XMLHttpRequest();
	    xhr.open("POST", "/apikey_set");
	    xhr.onload = function() {
	      if (xhr.status === 200) {
	        alert("データを送信しました！");
	      } else {
	        alert("送信に失敗しました。");
	      }
	    };
	    xhr.send(formData);
	  }
	</script>
  </body>
</html>)KEWL";

static const char ROLE_HTML[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html>
<head>
	<title>role-setting</title>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<style>
		textarea {
			width: 80%;
			height: 200px;
			resize: both;
		}
	</style>
</head>
<body>
	<h1>role-setting</h1>
	<form onsubmit="postData(event)">
		<label for="textarea">ここにロールを記述してください。:</label><br>
		<textarea id="textarea" name="textarea"></textarea><br><br>
		<input type="submit" value="Submit">
	</form>
	<script>
		function postData(event) {
			event.preventDefault();
			const textAreaContent = document.getElementById("textarea").value.trim();
//			if (textAreaContent.length > 0) {
				const xhr = new XMLHttpRequest();
				xhr.open("POST", "/role_set", true);
				xhr.setRequestHeader("Content-Type", "text/plain;charset=UTF-8");
			// xhr.onload = () => {
			// 	location.reload(); // 送信後にページをリロード
			// };
			xhr.onload = () => {
				document.open();
				document.write(xhr.responseText);
				document.close();
			};
				xhr.send(textAreaContent);
//        document.getElementById("textarea").value = "";
				alert("Data sent successfully!");
//			} else {
//				alert("Please enter some text before submitting.");
//			}
		}
	</script>
</body>
</html>)KEWL";

#ifdef USE_EXTEND
static const char ROLE1_HTML[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html>
<head>
	<title>role1-setting</title>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<style>
		textarea {
			width: 80%;
			height: 200px;
			resize: both;
		}
	</style>
</head>
<body>
	<h1>Role1-Setting</h1>
	<form onsubmit="postData(event)">
		<label for="textarea">ここにロールを記述してください。（空欄の場合は初期データになります。）:</label><br>
		<textarea id="textarea" name="textarea"></textarea><br><br>
		<input type="submit" value="Submit">
	</form>
	<script>
		function postData(event) {
			event.preventDefault();
			const textAreaContent = document.getElementById("textarea").value.trim();
				const xhr = new XMLHttpRequest();
				xhr.open("POST", "/role1_set", true);
				xhr.setRequestHeader("Content-Type", "text/plain;charset=UTF-8");
			xhr.onload = () => {
				document.open();
				document.write(xhr.responseText);
				document.close();
			};
				xhr.send(textAreaContent);
				alert("Data sent successfully!");
		}
	</script>
</body>
</html>)KEWL";

// --------------- TTS(Text-to-Speech) 関連 ----------------------------
/// set M5Speaker virtual channel (0-7)
// static constexpr uint8_t m5spk_virtual_channel = 0;
// static AudioOutputM5Speaker out(&M5.Speaker, m5spk_virtual_channel);
AudioOutputM5Speaker out(&M5.Speaker, m5spk_virtual_channel);
AudioGeneratorMP3 *mp3;

// for TTS0(VoiceText) and TTS2(VOICEVOX)
AudioFileSourceVoiceTextStream *file_TTS0 = nullptr;
AudioFileSourceHTTPSStream *file_TTS2 = nullptr;
int TTS02mp3buffSize = 30 * 1024;
uint8_t *TTS02mp3buff;
AudioFileSourceBuffer *BUFF = nullptr;
void playMP3(AudioFileSourceBuffer *buff)
{
  mp3->begin(buff, &out);
}
// for TTS1(GoogleTTS) mp3Buff
AudioFileSourcePROGMEM *file_TTS1 = nullptr;
#define TTS1_mp3buffSize 20 * 1024 // GoogleTTS用のmp3buffer Size
uint8_t TTS1_mp3buff[TTS1_mp3buffSize];

TTS tts;
HTTPClient http;
WiFiClient client;
String OPENAI_API_KEY = "";
String VOICEVOX_API_KEY = "";
String STT_API_KEY = "";
String TTS2_SPEAKER_NO = "";
String TTS2_SPEAKER = "&speaker=";
String TTS2_PARMS = TTS2_SPEAKER + TTS2_SPEAKER_NO;
String KEYWORDS[] = {"(Neutral)", "(Happy)", "(Sleepy)", "(Doubt)", "(Sad)", "(Angry)"};

// -------- SERVO ------------------------------------------------------------------
#define USE_SERVO
#define SV_PIN_X_CORE2_PA 33 // Core2 PORT A
#define SV_PIN_Y_CORE2_PA 32
#define SV_PIN_X_CORE2_PC 13 // Core2 PORT C
#define SV_PIN_Y_CORE2_PC 14
int SERVO_PIN_X;
int SERVO_PIN_Y;
bool SERVO_HOME = false;

ServoEasing servo_x;
ServoEasing servo_y;

struct box_t
{
  int x;
  int y;
  int w;
  int h;
  int touch_id = -1;

  void setupBox(int x, int y, int w, int h)
  {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
  }
  bool contain(int x, int y)
  {
    return this->x <= x && x < (this->x + this->w) && this->y <= y && y < (this->y + this->h);
  }
};
static box_t BOX_SERVO;
// static box_t box_stt;
// static box_t box_BtnA;
// static box_t box_BtnC;

// --------------------------- AiStackChanEx Original ----------------------------
#include "AiStackChanEx.h"
// グローバル変数宣言
const char EX_SETTING_NVS[] = "setting";    // setting --NVS の設定用ファイル
const char EX_APIKEY_NVS[] = "apikey";      // apikey  -- NVS の設定用ファイル
const char EX_CHATDOC_SPI[] = "/data.json"; // chatDoc in SPIFFS
// const char EX_APIKEY_SD[] = "/apikey.txt"; // apikey.txt  -- SD の設定用ファイル
// const char EX_WIFI_SD[] = "/wifi.txt";
const char EX_STARTUP_SD[] = "/startup.json";
const char EX_WIFISELECT_SD[] = "/wifi-select.json";

#define EX_WIFIJSON_SIZE 5 * 1024
#define EX_STARTUPJSON_SIZE 10 * 1024

bool EX_SYSINFO_DISP = false;
String EX_SYSINFO_MSG = "";
String EX_IP_ADDR = "";
String EX_SSID = "*****";
String EX_SSID_PASSWD = "";
String EX_VOICETEXT_API_KEY = "";

uint16_t EX_TIMER_SEC = EX_TIMER_INIT; // Timer時間設定(sec)
bool EX_TIMER_STARTED = false;
uint32_t EX_TIMER_START_MILLIS = 0;
uint16_t EX_TIMER_ELEAPSE_SEC = 0;
bool EX_TIMER_STOP_GET = false;
bool EX_TIMER_GO_GET = false;

bool EX_RANDOM_SPEAK_STATE = false; // 独り言モード　true -> on  false -> off
bool EX_RANDOM_SPEAK_ON_GET = false;
// bool EX_RANDOM_SPEAK_OFF_GET = false;
// bool EX_SELF_INTRO_GET = false;
bool EX_SHUTDOWN_REQ_GET = false;
size_t EX_VOLUME;
bool EX_MUTE_ON = false;
uint8_t EX_TONE_MODE = 1; // 0:allOff 1:buttonOn 2:extCommOn 3:allOn
// 「わかりません」対策用
int EX_WK_CNT = 0;
int EX_WK_ERROR_NO = 0;
int EX_WK_ERROR_CODE = 0;
int EX_LAST_WK_ERROR_NO = 0;
int EX_LAST_WK_ERROR_CODE = 0;

bool EX_LED_OnOff_STATE = true;

uint8_t EX_TTS_TYPE = 2; // default "VOICEVOX"
String LANG_CODE = "";
// const char EX_ttsName[3][15] = {"VoiceText", "GoogleTTS", "VOICEVOX"};
const char *EX_ttsName[] = {"VoiceText", "GoogleTTS", "VOICEVOX"};
const char LANG_CODE_JP[] = "ja-JP";
const char LANG_CODE_EN[] = "en-US";

// ---- 初期ロール設定 --------------------
String EX_json_ChatString = " { \"model\":\"gpt-3.5-turbo\",\"messages\": [ { \"role\": \"user\",\"content\": \"\" }, { \"role\": \"system\", \"content\": \"あなたは「スタックちゃん」と言う名前の小型ロボットとして振る舞ってください。あなたはの使命は人々の心を癒すことです。\" } ] } ";

//-----Ver1.11 ------------------------------------------
/*
// ---------------------------------------------------------------------
int servo_offset_x = 0; // X軸サーボのオフセット（90°からの+-で設定）
int servo_offset_y = 0; // Y軸サーボのオフセット（90°からの+-で設定）

void moveX(int x, uint32_t millis_for_move = 0)
{
  if (millis_for_move == 0)
  {
    servo_x.easeTo(x + servo_offset_x);
  }
  else
  {
    servo_x.easeToD(x + servo_offset_x, millis_for_move);
  }
}

void moveY(int y, uint32_t millis_for_move = 0)
{
  if (millis_for_move == 0)
  {
    servo_y.easeTo(y + servo_offset_y);
  }
  else
  {
    servo_y.easeToD(y + servo_offset_y, millis_for_move);
  }
}

void moveXY(int x, int y, uint32_t millis_for_move = 0)
{
  if (millis_for_move == 0)
  {
    servo_x.setEaseTo(x + servo_offset_x);
    servo_y.setEaseTo(y + servo_offset_y);
  }
  else
  {
    servo_x.setEaseToD(x + servo_offset_x, millis_for_move);
    servo_y.setEaseToD(y + servo_offset_y, millis_for_move);
  }
  // サーボが停止するまでウェイトします。
  synchronizeAllServosStartAndWaitForAllServosToStop();
}

void EX_servoTextSwing()
{
  moveX(90);
  moveY(90);

  avatar.setSpeechText("X 90 -> 0  ");
  moveX(0);
  avatar.setSpeechText("X 0 -> 180  ");
  moveX(180);
  avatar.setSpeechText("X 180 -> 90  ");
  moveX(90);
  avatar.setSpeechText("Y 90 -> 50  ");
  moveY(50);
  avatar.setSpeechText("Y 50 -> 90  ");
  moveY(90);
  avatar.setSpeechText("");
}
// ---------------------------------------------------------------------
*/

//-----Ver1.10 ------------------------------------------
bool EX_KEYLOCK_STATE = false;
bool SV_USE = true;
String SV_PORT = "";
const char *SV_MD_Name[] = {"MOVING", "STOP", "HOME", "CENTER", "POINT", "DELTA", "SWING"};
int SV_MD = 0; // moving
#define SV_MD_MOVING 0
#define SV_MD_STOP 1
#define SV_MD_HOME 2
#define SV_MD_CENTER 3
#define SV_MD_POINT 4
#define SV_MD_DELTA 5
#define SV_MD_SWING 6
#define SV_HOME_X 90
// #define SV_HOME_Y 85
#define SV_HOME_Y 80
#define SV_CENTER_X 90
#define SV_CENTER_Y 90
int SV_POINT_X = SV_HOME_X;
int SV_POINT_Y = SV_HOME_Y;
int SV_PREV_POINT_X = SV_HOME_X;
int SV_PREV_POINT_Y = SV_HOME_Y;
int SV_NEXT_POINT_X;
int SV_NEXT_POINT_Y;
bool SV_STOP_STATE = false;
// int SV_SWING_CNT = 0;
// int SV_SWING_MAX = 3;

void sv_setX(int x)
{
  SV_PREV_POINT_X = SV_POINT_X;
  SV_POINT_X = x;
  servo_x.setEaseTo(x);
}

void sv_setY(int y)
{
  SV_PREV_POINT_Y = SV_POINT_Y;
  SV_POINT_Y = y;
  servo_y.setEaseTo(y);
}

void sv_setXY(int x, int y)
{
  sv_setX(x);
  sv_setY(y);
}

#define SV_MD_NONE 99
void EX_servo(void *args)
{
  float gazeX, gazeY;
  DriveContext *ctx = (DriveContext *)args;
  Avatar *avatar = ctx->getAvatar();
  for (;;)
  {
    if (SV_USE)
    {
      char msg[50];
      int mode = SV_MD;

      switch (mode)
      {
      case SV_MD_MOVING:
        // ------------------------------------------------------------------
        avatar->getGaze(&gazeY, &gazeX);
        sv_setX(SV_HOME_X + (int)(15.0 * gazeX));
        if (gazeY < 0)
        {
          int tmp = (int)(10.0 * gazeY);
          if (tmp > 10)
            tmp = 10;
          sv_setY(SV_HOME_Y + tmp);
        }
        else
        {
          sv_setY(SV_HOME_Y + (int)(10.0 * gazeY));
        }
        // ------------------------------------------------------------------
        synchronizeAllServosStartAndWaitForAllServosToStop();
        break;

      case SV_MD_STOP:
        SV_MD = SV_MD_NONE;
        sprintf(msg, "SV_STOP: Servo point x= %d  y = %d", SV_POINT_X, SV_POINT_Y);
        Serial.println(msg);
        break;

      case SV_MD_HOME:
        SV_MD = SV_MD_NONE;
        sv_setXY(SV_HOME_X, SV_HOME_Y);
        sprintf(msg, "SV_HOME: Servo point x= %d  y = %d", SV_POINT_X, SV_POINT_Y);
        Serial.println(msg);
        synchronizeAllServosStartAndWaitForAllServosToStop();
        break;

      case SV_MD_CENTER:
        SV_MD = SV_MD_NONE;
        sv_setXY(SV_CENTER_X, SV_CENTER_Y);
        sprintf(msg, "SV_CENTER: Servo point x= %d  y = %d", SV_POINT_X, SV_POINT_Y);
        Serial.println(msg);
        synchronizeAllServosStartAndWaitForAllServosToStop();
        break;

      case SV_MD_POINT:
        SV_MD = SV_MD_NONE;
        sv_setXY(SV_NEXT_POINT_X, SV_NEXT_POINT_Y);
        sprintf(msg, "SV_POINT: Servo point x= %d  y = %d", SV_POINT_X, SV_POINT_Y);
        Serial.println(msg);
        synchronizeAllServosStartAndWaitForAllServosToStop();
        break;

      case SV_MD_DELTA:
        SV_MD = SV_MD_NONE;
        sv_setXY(SV_NEXT_POINT_X, SV_NEXT_POINT_Y);
        sprintf(msg, "SV_DELTA: Servo point x= %d  y = %d", SV_POINT_X, SV_POINT_Y);
        Serial.println(msg);
        synchronizeAllServosStartAndWaitForAllServosToStop();
        break;

      case SV_MD_SWING:
        break;

      case SV_MD_NONE:
        break;

      default:

        return;
      }
      // synchronizeAllServosStartAndWaitForAllServosToStop();
    }
    delay(50);
  }
}




// void EX_servo(void *args)
// {
//   float gazeX, gazeY;
//   DriveContext *ctx = (DriveContext *)args;
//   Avatar *avatar = ctx->getAvatar();
//   for (;;)
//   {
//     if (SV_USE)
//     {
//       char msg[50];

//       switch (SV_MD)
//       {
//       case SV_MD_MOVING:
//         // ------------------------------------------------------------------
//         avatar->getGaze(&gazeY, &gazeX);
//         sv_setX(SV_HOME_X + (int)(15.0 * gazeX));
//         if (gazeY < 0)
//         {
//           int tmp = (int)(10.0 * gazeY);
//           if (tmp > 10)
//             tmp = 10;
//           sv_setY(SV_HOME_Y + tmp);
//         }
//         else
//         {
//           sv_setY(SV_HOME_Y + (int)(10.0 * gazeY));
//         }
//         // ------------------------------------------------------------------
//         break;

//       case SV_MD_STOP:
//         if (!SV_STOP_STATE)
//         {
//           sv_setXY(SV_POINT_X, SV_POINT_Y);
//           SV_STOP_STATE = true;

//           sprintf(msg, "SV_STOP: Servo point x= %d  y = %d", SV_POINT_X, SV_POINT_Y);
//           Serial.println(msg);
//         }
//         break;

//       case SV_MD_HOME:
//         sv_setXY(SV_HOME_X, SV_HOME_Y);

//         if ((SV_PREV_POINT_X != SV_POINT_X) || (SV_PREV_POINT_Y != SV_POINT_Y))
//         {
//           sprintf(msg, "SV_HOME: Servo point x= %d  y = %d", SV_POINT_X, SV_POINT_Y);
//           Serial.println(msg);
//         }
//         break;

//       case SV_MD_CENTER:
//         sv_setXY(SV_CENTER_X, SV_CENTER_Y);

//         if ((SV_PREV_POINT_X != SV_POINT_X) || (SV_PREV_POINT_Y != SV_POINT_Y))
//         {
//           sprintf(msg, "SV_CENTER: Servo point x= %d  y = %d", SV_POINT_X, SV_POINT_Y);
//           Serial.println(msg);
//         }
//         break;

//       case SV_MD_POINT:
//         sv_setXY(SV_NEXT_POINT_X, SV_NEXT_POINT_Y);

//         if ((SV_PREV_POINT_X != SV_POINT_X) || (SV_PREV_POINT_Y != SV_POINT_Y))
//         {
//           sprintf(msg, "SV_POINT: Servo point x= %d  y = %d", SV_POINT_X, SV_POINT_Y);
//           Serial.println(msg);
//         }
//         break;

//       case SV_MD_DELTA:
//         sv_setXY(SV_NEXT_POINT_X, SV_NEXT_POINT_Y);

//         if ((SV_PREV_POINT_X != SV_POINT_X) || (SV_PREV_POINT_Y != SV_POINT_Y))
//         {
//           sprintf(msg, "SV_DELTA: Servo point x= %d  y = %d", SV_POINT_X, SV_POINT_Y);
//           Serial.println(msg);
//         }
//         break;

//       case SV_MD_SWING:
//         // if (SV_SWING_CNT < SV_SWING_MAX)
//         // {
//         //   SV_SWING_CNT++;
//         //   EX_servoTextSwing();
//         // }
//         // else
//         // {
//         //   // avatar.setSpeechText("");
//         //   SV_MD = SV_MD_HOME;
//         // }
//         break;

//       default:
//         return;
//       }
//       synchronizeAllServosStartAndWaitForAllServosToStop();
//     }
//     delay(50);
//   }
// }

#define SV_X_MIN 0
#define SV_X_MAX 180
#define SV_Y_MIN 50
#define SV_Y_MAX 100

void EX_handle_servo()
{
  EX_tone(2);
  char msg[100];

  // ---- pointX, pointY -------
  String pointX_str = server.arg("pointX");
  String pointY_str = server.arg("pointY");
  // sprintf(msg, "pointX = %s  pointY = %s", pointX_str.c_str(), pointY_str.c_str());
  // Serial.println(msg);
  // // pointX_str.toUpperCase();
  // pointY_str.toUpperCase();
  if ((pointX_str != "") || (pointY_str != ""))
  { // pointX
    // ----------------------------------------------
    int x_val = SV_POINT_X; // default

    if (pointX_str != "")
      x_val = pointX_str.toInt();

    SV_NEXT_POINT_X = x_val;

    if (SV_NEXT_POINT_X < SV_X_MIN)
      SV_NEXT_POINT_X = SV_X_MIN;

    if (SV_NEXT_POINT_X > SV_X_MAX)
      SV_NEXT_POINT_X = SV_X_MAX;

    // y-value
    // ----------------------------------------------
    int y_val = SV_POINT_Y; // default

    if (pointY_str != "")
      y_val = pointY_str.toInt();

    SV_NEXT_POINT_Y = y_val;

    if (SV_NEXT_POINT_Y < SV_Y_MIN) // 50 to 100
      SV_NEXT_POINT_Y = SV_Y_MIN;

    if (SV_NEXT_POINT_Y > SV_Y_MAX)
      SV_NEXT_POINT_Y = SV_Y_MAX;

    // sprintf(msg, "SV_MD_POINT: x_str = %s  y_str = %s", x_str.c_str(),y_str.c_str() );
    // Serial.println(msg);

    SV_MD = SV_MD_POINT;
    sprintf(msg, "SERVO: x = %d , y = %d", SV_NEXT_POINT_X, SV_NEXT_POINT_Y);
    Serial.println(msg);
    server.send(200, "text/plain", msg);
    return;
  }

  // ---- deltaX, deltaY -------
  String deltaX_str = server.arg("deltaX");
  String deltaY_str = server.arg("deltaY");
  // sprintf(msg, "deltaX = %s  deltaY = %s", deltaX_str.c_str(), deltaY_str.c_str());
  // Serial.println(msg);

  // deltaX_str.toUpperCase();
  // deltaY_str.toUpperCase();
  if ((deltaX_str != "") || (deltaY_str != ""))
  {
    // deltaX
    // ----------------------------------------------
    int x_delta = 0;
    SV_NEXT_POINT_X = SV_POINT_X;

    if (deltaX_str != "")
      x_delta = deltaX_str.toInt();

    SV_NEXT_POINT_X += x_delta;

    if (SV_NEXT_POINT_X < SV_X_MIN) // 0 to 180
      SV_NEXT_POINT_X = SV_X_MIN;

    if (SV_NEXT_POINT_X > SV_X_MAX)
      SV_NEXT_POINT_X = SV_X_MAX;

    // deltaY
    // ----------------------------------------------
    int y_delta = 0;
    SV_NEXT_POINT_Y = SV_POINT_Y;

    if (deltaY_str != "")
      y_delta = deltaY_str.toInt();

    SV_NEXT_POINT_Y += y_delta;

    if (SV_NEXT_POINT_Y < SV_Y_MIN) // 50 to 100
      SV_NEXT_POINT_Y = SV_Y_MIN;

    if (SV_NEXT_POINT_Y > SV_Y_MAX)
      SV_NEXT_POINT_Y = SV_Y_MAX;

    SV_MD = SV_MD_DELTA;
    sprintf(msg, "SERVO: x = %d , y = %d", SV_NEXT_POINT_X, SV_NEXT_POINT_Y);
    Serial.println(msg);
    server.send(200, "text/plain", msg);
    return;
  }

  // ---- TX -------
  String tx_str = server.arg("tx");
  tx_str.toUpperCase();
  if (tx_str != "")
  { // TX
    sprintf(msg, "NG");

    if (tx_str == "XY")
      sprintf(msg, "SERVO: x = %d , y = %d", SV_POINT_X, SV_POINT_Y);

    else if (tx_str == "X")
      sprintf(msg, "SERVO: x = %d", SV_POINT_X);

    else if (tx_str == "Y")
      sprintf(msg, "SERVO: y = %d", SV_POINT_Y);

    Serial.println(msg);
    server.send(200, "text/plain", msg);
    return;
  }

  // ---- MODE -------
  String mode_str = server.arg("mode");
  mode_str.toUpperCase();
  if (mode_str != "")
  {
    if (mode_str == String(SV_MD_Name[0]))
    { // moving
      SV_MD = SV_MD_MOVING;

      server.send(200, "text/plain", String("OK"));
      return;
    }

    else if (mode_str == String(SV_MD_Name[1]))
    { // stop
      SV_MD = SV_MD_STOP;

      server.send(200, "text/plain", String("OK"));
      return;
    }

    else if (mode_str == String(SV_MD_Name[2]))
    { // home
      SV_MD = SV_MD_HOME;
      sprintf(msg, "SERVO: x = %d , y = %d", SV_HOME_X, SV_HOME_Y);
      Serial.println(msg);
      server.send(200, "text/plain", msg);
      return;
    }

    else if (mode_str == String(SV_MD_Name[3]))
    { // center
      SV_MD = SV_MD_CENTER;

      sprintf(msg, "SERVO: x = %d , y = %d", SV_CENTER_X, SV_CENTER_Y);
      Serial.println(msg);
      server.send(200, "text/plain", msg);
      return;
    }

    else if (mode_str == String(SV_MD_Name[6]))
    { // swing
      // int repeat = 1; // default

      // String repeat_str = server.arg("repeat"); // 1 to 30
      // if (repeat_str != "")
      // {
      //   repeat = repeat_str.toInt();
      //   if (repeat < 1 || repeat > 30)
      //     repeat = 1;
      // }

      // SV_SWING_CNT = 0;
      // SV_SWING_MAX = repeat;
      // SV_MD = SV_MD_SWING;
    }
    Serial.println("servo?mode = " + mode_str);
  }

  server.send(200, "text/plain", String("NG"));
}

void EX_handle_setting()
{
  EX_tone(2);

  uint32_t nvs_handle;

  // ---- lang -------
  String lang_str = server.arg("lang");
  if (lang_str != "")
  {
    LANG_CODE = lang_str;

    if (ESP_OK == nvs_open(EX_SETTING_NVS, NVS_READWRITE, &nvs_handle))
    {
      nvs_set_str(nvs_handle, "lang", (char *)lang_str.c_str());
    }
    nvs_close(nvs_handle);
  }

  // ---- ttsSelect -------
  String ttsName_str = server.arg("ttsSelect");
  if (ttsName_str != "")
  {
    ttsName_str.toUpperCase();
    // if (ttsName_str == EX_ttsName[0])
    if (ttsName_str == "VOICETEXT")
      EX_TTS_TYPE = 0;
    else if (ttsName_str == "VOICEVOX")
      EX_TTS_TYPE = 2;
    else
      EX_TTS_TYPE = 1;

    String ttsName_tmp = EX_ttsName[EX_TTS_TYPE];
    if (ESP_OK == nvs_open(EX_SETTING_NVS, NVS_READWRITE, &nvs_handle))
    {
      nvs_set_str(nvs_handle, "ttsSelect", (char *)ttsName_tmp.c_str());
    }
    nvs_close(nvs_handle);
  }

  // ---- volume -------

  String volume_val_str = server.arg("volume");
  if (volume_val_str != "")
  {
    Serial.println("setting?volume=" + volume_val_str);
    int volumeVal = volume_val_str.toInt();
    if (volumeVal > 255)
      volumeVal = 255;
    if (volumeVal <= 0)
      volumeVal = 0;

    EX_VOLUME = volumeVal;
    M5.Speaker.setVolume(EX_VOLUME);
    M5.Speaker.setChannelVolume(m5spk_virtual_channel, EX_VOLUME);
    if (ESP_OK == nvs_open(EX_SETTING_NVS, NVS_READWRITE, &nvs_handle))
    {
      nvs_set_u32(nvs_handle, "volume", EX_VOLUME);
    }
    nvs_close(nvs_handle);
  }

  // ---- mute -------
  String mute_str = server.arg("mute");
  if (mute_str != "")
  {
    if (mute_str == "on")
    {
      if (!EX_MUTE_ON)
      {
        EX_muteOn();
        Serial.println("setting?mute=" + mute_str);
      }
    }
    else if (mute_str == "off")
    {
      if (EX_MUTE_ON)
      {
        EX_muteOff();
        Serial.println("setting?mute=" + mute_str);
        if (EX_SYSINFO_DISP)
        {
          avatar.start();
          delay(200);
          EX_SYSINFO_DISP = false;
        }
      }
    }
  }

  // ---- keyLock -------
  String keyLock_str = server.arg("keyLock");
  if (keyLock_str != "")
  {
    keyLock_str.toLowerCase();
    if (keyLock_str == "on")
    {
      EX_KEYLOCK_STATE = true;
      if (ESP_OK == nvs_open(EX_SETTING_NVS, NVS_READWRITE, &nvs_handle))
        nvs_set_str(nvs_handle, "keyLock", "on");
      nvs_close(nvs_handle);
    }
    else if (keyLock_str == "off")
    {
      EX_KEYLOCK_STATE = false;
      if (ESP_OK == nvs_open(EX_SETTING_NVS, NVS_READWRITE, &nvs_handle))
        nvs_set_str(nvs_handle, "keyLock", "off");
      nvs_close(nvs_handle);
    }
    Serial.println("setting?keyLock = " + keyLock_str);
  }

  // ---- toneMode -------
  String toneMode_val_str = server.arg("toneMode");
  if (toneMode_val_str != "")
  {
    EX_TONE_MODE = toneMode_val_str.toInt();
    if ((EX_TONE_MODE < 0) || (EX_TONE_MODE > 3))
    {
      EX_TONE_MODE = 1;
    }
  }
  if (ESP_OK == nvs_open(EX_SETTING_NVS, NVS_READWRITE, &nvs_handle))
  {
    nvs_set_u32(nvs_handle, "toneMode", EX_TONE_MODE);
  }
  nvs_close(nvs_handle);

  // ---- led -------
  String led_str = server.arg("led");
  uint8_t led_onoff;
  if (led_str != "")
  {
    if (led_str == "off")
    {
      EX_LED_allOff();
      EX_LED_OnOff_STATE = false;
      led_onoff = 0;
    }
    else if (led_str == "on")
    {
      EX_LED_OnOff_STATE = true;
      led_onoff = 1;
    }

    if (ESP_OK == nvs_open(EX_SETTING_NVS, NVS_READWRITE, &nvs_handle))
    {
      nvs_set_u8(nvs_handle, "led", led_onoff);
    }
    nvs_close(nvs_handle);
  }

  // ---- Speaker -------
  String speaker = server.arg("speaker");
  Serial.println(speaker);
  size_t speaker_no;
  if (speaker != "")
  {
    speaker_no = speaker.toInt();
    if (speaker_no > 66)
    {
      speaker_no = 3;
    }
    TTS2_SPEAKER_NO = String(speaker_no);
    TTS2_PARMS = TTS2_SPEAKER + TTS2_SPEAKER_NO;

    if (ESP_OK == nvs_open(EX_SETTING_NVS, NVS_READWRITE, &nvs_handle))
    {
      nvs_set_u8(nvs_handle, "speaker", speaker_no);
      Serial.print("NVS Write : speaker_no = ");
      Serial.println(speaker_no, DEC);
    }
    nvs_close(nvs_handle);
  }

  server.send(200, "text/plain", String("OK"));
}

void EX_report_batt_level()
{
  char buff[100];
  int level = M5.Power.getBatteryLevel();
  if (M5.Power.isCharging())
  {
    if (EX_isJP())
      sprintf(buff, "充電中、バッテリーのレベルは%d％です。", level);
    else
      sprintf(buff, "Charging, battery level is %d percent .", level);
  }
  else
  {
    if (EX_isJP())
      sprintf(buff, "バッテリーのレベルは%d％です。", level);
    else
      sprintf(buff, "battery level is %d . ", level);
  }
  avatar.setExpression(Expression::Happy);
  EX_ttsDo(buff, tts_parms2);
  avatar.setExpression(Expression::Neutral);
  Serial.println("mp3 begin");
}

void EX_RandomSpeakOperation()
{
  LASTMS1 = millis();                      // 今回のRandomSpeak起動した時刻
  RANDOM_TIME = 40000 + 1000 * random(30); // 次回のRandomSpeak起動までの時間
  if ((!mp3->isRunning()) && (SPEECH_TEXT == "") && (SPEECH_TEXT_BUFFER == ""))
  {
    exec_chatGPT(random_words[random(18)]);
  }
}

void EX_ServoOperation()
{
  auto t = M5.Touch.getDetail();
  if (t.wasPressed())
  {
    if (SV_USE)
    {
      if (BOX_SERVO.contain(t.x, t.y))
      {
        EX_tone(1);

        SERVO_HOME = !SERVO_HOME;
        if (SERVO_HOME)
          Serial.println("Servo: HOME ");
        else
          Serial.println("Servo: MOVE ");
      }
    }
  }
}

// ---------------------------
// 最初のSpeechText処理
// ---------------------------
void EX_SpeechText1st()
{
  String sentence;
  int dotIndex;

  switch (EX_TTS_TYPE)
  {
  case 0: // VoiceText
    SPEECH_TEXT_BUFFER = SPEECH_TEXT;
    SPEECH_TEXT = "";

    // 感情表現keyWordの前に、「。」を挿入
    addPeriodBeforeKeyword(SPEECH_TEXT_BUFFER, KEYWORDS, 6);

    Serial.println("-----------------------------");
    Serial.println(SPEECH_TEXT_BUFFER);
    //---------------------------------
    sentence = SPEECH_TEXT_BUFFER;
    dotIndex = SPEECH_TEXT_BUFFER.indexOf("。");

    if (dotIndex != -1)
    {
      dotIndex += 3;
      sentence = SPEECH_TEXT_BUFFER.substring(0, dotIndex);
      Serial.println(sentence);
      SPEECH_TEXT_BUFFER = SPEECH_TEXT_BUFFER.substring(dotIndex);
    }
    else
    {
      SPEECH_TEXT_BUFFER = "";
    }

    //----------------
    getExpression(sentence, EXPRESSION_INDX);
    //----------------
    if (EXPRESSION_INDX < 0)
    {
      avatar.setExpression(Expression::Happy);
      EX_ttsDo((char *)sentence.c_str(), tts_parms_table[TTS_PARMS_NO]);
      avatar.setExpression(Expression::Neutral);
    }
    else
    { // 音声に感情表現
      String tmp = EMOTION_PARAMS[EXPRESSION_INDX] + TTS_PARMS_STR[TTS_PARMS_NO];
      EX_ttsDo((char *)sentence.c_str(), (char *)tmp.c_str());
    }

    break;

  case 1: // GoogleTTS
    SPEECH_TEXT_BUFFER = SPEECH_TEXT;
    SPEECH_TEXT = "";

    // addPeriodBeforeKeyword(SPEECH_TEXT_BUFFER, KEYWORDS, 6);

    Serial.println("-----------------------------");
    Serial.println(SPEECH_TEXT_BUFFER);
    //---------------------------------
    sentence = SPEECH_TEXT_BUFFER;

    if (EX_isJP())
      dotIndex = search_separator(SPEECH_TEXT_BUFFER, 0);
    else
      dotIndex = search_separator(SPEECH_TEXT_BUFFER, 1);

    if (dotIndex != -1)
    {
      if (EX_isJP())
        dotIndex += 3;
      else
        dotIndex += 2; // ** Global版では、(+= 1) by NoRi ***

      sentence = SPEECH_TEXT_BUFFER.substring(0, dotIndex);
      Serial.println(sentence);
      SPEECH_TEXT_BUFFER = SPEECH_TEXT_BUFFER.substring(dotIndex);
    }
    else
    {
      SPEECH_TEXT_BUFFER = "";
    }

    // //----------------
    // getExpression(sentence, EXPRESSION_INDX);
    // //----------------
    // if (EXPRESSION_INDX < 0)
    //   avatar.setExpression(Expression::Happy);
    // EX_ttsDo((char *)sentence.c_str(), (char *)LANG_CODE.c_str());
    // if (EXPRESSION_INDX < 0)
    //   avatar.setExpression(Expression::Neutral);
    //  ---- 感情表現の削除 ----
    avatar.setExpression(Expression::Happy);
    EX_ttsDo((char *)sentence.c_str(), (char *)LANG_CODE.c_str());
    avatar.setExpression(Expression::Neutral);

    break;
    // ------------------------------------------------------------------------

  case 2: // VOICEVOX
    SPEECH_TEXT_BUFFER = SPEECH_TEXT;
    SPEECH_TEXT = "";
    avatar.setExpression(Expression::Happy);
    EX_ttsDo((char *)SPEECH_TEXT_BUFFER.c_str(), (char *)TTS2_PARMS.c_str());
    break;
  }
  //  --------------------------------------------------------------------------
}

// ---------------------------
// ２回目以降のSpeechText処理
// ---------------------------
void EX_SpeechTextNext()
{
  // **********************
  // ***** mp3　STOP　*****
  // **********************
  mp3->stop();
  Serial.println("mp3 stop");

  switch (EX_TTS_TYPE)
  {

  case 0: // voiceText
    if (file_TTS0 != nullptr)
    {
      delete file_TTS0;
      file_TTS0 = nullptr;
    }

    // *******************************************
    // ------- sentence切出し(TTS0 VoiceText) ----
    // *******************************************
    if (SPEECH_TEXT_BUFFER != "")
    {
      String sentence = SPEECH_TEXT_BUFFER;
      int dotIndex;
      dotIndex = SPEECH_TEXT_BUFFER.indexOf("。");

      if (dotIndex != -1)
      {
        dotIndex += 3;
        sentence = SPEECH_TEXT_BUFFER.substring(0, dotIndex);
        Serial.println(sentence);
        SPEECH_TEXT_BUFFER = SPEECH_TEXT_BUFFER.substring(dotIndex);
      }
      else
      {
        SPEECH_TEXT_BUFFER = "";
      }

      //----------------
      getExpression(sentence, EXPRESSION_INDX);
      //----------------
      if (EXPRESSION_INDX < 0)
      {
        avatar.setExpression(Expression::Happy);
        EX_ttsDo((char *)sentence.c_str(), tts_parms_table[TTS_PARMS_NO]);
        avatar.setExpression(Expression::Neutral);
      }
      else
      { // 音声の感情表現
        String tmp = EMOTION_PARAMS[EXPRESSION_INDX] + TTS_PARMS_STR[TTS_PARMS_NO];
        EX_ttsDo((char *)sentence.c_str(), (char *)tmp.c_str());
      }
    }

    else
    { // 最終回
      avatar.setExpression(Expression::Neutral);
      EXPRESSION_INDX = -1;
    }
    // --- end of TTS0 ---
    break;

  case 1: // GoogleTTS
    if (file_TTS1 != nullptr)
    {
      delete file_TTS1;
      file_TTS1 = nullptr;
    }

    // *******************************************
    // ------- sentence切出し(TTS1 GoogleTTS) ----
    // *******************************************
    if (SPEECH_TEXT_BUFFER != "")
    {
      String sentence = SPEECH_TEXT_BUFFER;
      int dotIndex;
      if (EX_isJP())
        dotIndex = search_separator(SPEECH_TEXT_BUFFER, 0);
      else
        dotIndex = search_separator(SPEECH_TEXT_BUFFER, 1);

      if (dotIndex != -1)
      {
        if (EX_isJP())
          dotIndex += 3;
        else
          dotIndex += 2;

        sentence = SPEECH_TEXT_BUFFER.substring(0, dotIndex);
        Serial.println(sentence);
        SPEECH_TEXT_BUFFER = SPEECH_TEXT_BUFFER.substring(dotIndex);
      }
      else
      {
        SPEECH_TEXT_BUFFER = "";
      }

      // 感情表現の検出を削除 ----------------------------------------
      avatar.setExpression(Expression::Happy);
      EX_ttsDo((char *)sentence.c_str(), (char *)LANG_CODE.c_str());
      avatar.setExpression(Expression::Neutral);
      // //----------------
      // getExpression(sentence, EXPRESSION_INDX);
      // //----------------
      // if (EXPRESSION_INDX < 0)
      // {
      //   avatar.setExpression(Expression::Happy);
      //   EX_ttsDo((char *)sentence.c_str(), (char *)LANG_CODE.c_str());
      //   avatar.setExpression(Expression::Neutral);
      // }
      // else
      // {
      //   EX_ttsDo((char *)sentence.c_str(), (char *)LANG_CODE.c_str());
      // }
    }

    else
    { // 最終回
      avatar.setExpression(Expression::Neutral);
      EXPRESSION_INDX = -1;
    } // --- end of TTS1 ---

    break;

  case 2: // voicevox
    if (file_TTS2 != nullptr)
    {
      delete file_TTS2;
      file_TTS2 = nullptr;
    }

    // 最終回
    avatar.setExpression(Expression::Neutral);
    SPEECH_TEXT_BUFFER = "";

    break;
  } // end of case
}

//-----Ver1.09 ------------------------------------------

const char *EX_stupItem[] = {"openAiApiKey", "voiceTextApiKey", "voicevoxApiKey",
                             "volume", "lang", "voicevoxSpeakerNo", "ttsSelect", "servoPort", "servo",
                             "randomSpeak", "mute", "led", "toneMode", "timer", "keyLock"};

// const char EX_stupItem[15][30] = {"openAiApiKey", "voiceTextApiKey", "voicevoxApiKey",
//                                   "volume", "lang", "voicevoxSpeakerNo", "ttsSelect", "servoPort", "servo",
//                                   "randomSpeak", "mute", "led", "toneMode", "timer", "keyLock"};

// const char EX_stupItemNVM[15][30] = {"openai", "voicetext", "voicevox",
//                                      "volume", "lang", "speaker", "ttsSelect", "servoPort", "servo",
//                                      "randomSpeak", "mute", "ledEx", "toneMode", "timer","keyLock"};

bool EX_StartSetting()
{
  // ****** 初期値設定　**********
  OPENAI_API_KEY = "*****";
  EX_VOICETEXT_API_KEY = "*****";
  VOICEVOX_API_KEY = "*****";
  EX_VOLUME = 180;
  LANG_CODE = String(LANG_CODE_JP);
  TTS2_SPEAKER_NO = "3";
  EX_TTS_TYPE = 2; // VOICEVOX

  SV_PORT = "portA";
  SERVO_PIN_X = SV_PIN_X_CORE2_PA;
  SERVO_PIN_Y = SV_PIN_Y_CORE2_PA;
  SV_USE = true;

  RANDOM_TIME = -1;
  EX_RANDOM_SPEAK_ON_GET = false;
  EX_RANDOM_SPEAK_STATE = false;

  EX_MUTE_ON = false;
  EX_LED_OnOff_STATE = true;
  EX_TONE_MODE = 1;
  EX_TIMER_SEC = 180;
  EX_KEYLOCK_STATE = false;
  //----------------------------------

  if (!SD.begin(GPIO_NUM_4, SPI, 25000000))
  { // SD無効な時
    Serial.println("SD disable ");
    SD.end();
    return false;
  }

  auto fl_SD = SD.open(EX_STARTUP_SD, FILE_READ);
  if (!fl_SD)
  {
    Serial.println("startup.json not open ");
    SD.end();
    return false;
  }

  DynamicJsonDocument startupJson(EX_STARTUPJSON_SIZE);
  DeserializationError error = deserializeJson(startupJson, fl_SD);
  if (error)
  {
    Serial.println("DeserializationError in EX_startupRD func");
    SD.end();
    return false;
  }
  SD.end();
  JsonArray jsonArray = startupJson["startup"];
  JsonObject object = jsonArray[0];

  int cnt = 0;
  char msg[200];
  // String getStr="";

  // openAiApiKey
  String getStr0 = object[EX_stupItem[0]];
  if (getStr0 != "" && (getStr0 != "null"))
  {
    OPENAI_API_KEY = getStr0;
    sprintf(msg, "%s = %s", EX_stupItem[0], getStr0.c_str());
    Serial.println(msg);
    cnt++;
  }

  // voiceTextApiKey
  String getStr1 = object[EX_stupItem[1]];
  if (getStr1 != "" && (getStr1 != "null"))
  {
    tts_user = getStr1;
    EX_VOICETEXT_API_KEY = tts_user;
    sprintf(msg, "%s = %s", EX_stupItem[1], getStr1.c_str());
    Serial.println(msg);
    cnt++;
  }

  // voicevoxApiKey
  String getStr2 = object[EX_stupItem[2]];
  if (getStr2 != "" && (getStr2 != "null"))
  {
    VOICEVOX_API_KEY = getStr2;
    sprintf(msg, "%s = %s", EX_stupItem[2], getStr2.c_str());
    Serial.println(msg);
    cnt++;
  }

  // volume
  String getStr3 = object[EX_stupItem[3]];
  if ((getStr3 != "") && (getStr3 != "-1") && (getStr3 != "null"))
  {
    int getVal = getStr3.toInt();
    if (getVal <= 0)
      getVal = 0;
    if (getVal >= 255)
      getVal = 255;

    EX_VOLUME = (size_t)getVal;
    sprintf(msg, "%s = %s", EX_stupItem[3], getStr3.c_str());
    Serial.println(msg);
    cnt++;
  }
  else
  {
    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("setting", NVS_READONLY, &nvs_handle))
    {
      size_t volume;
      nvs_get_u32(nvs_handle, "volume", &volume);
      if (volume > 255)
        volume = 255;
      EX_VOLUME = volume;
      nvs_close(nvs_handle);
      sprintf(msg, "NVS read %s = %d", EX_stupItem[3], volume);
      Serial.println(msg);
      cnt++;
    }
  }
  M5.Speaker.setVolume(EX_VOLUME);
  M5.Speaker.setChannelVolume(m5spk_virtual_channel, EX_VOLUME);

  // lang
  String getStr4 = object[EX_stupItem[4]];
  if (getStr4 != "" && (getStr4 != "null"))
  {
    LANG_CODE = getStr4;
    sprintf(msg, "%s = %s", EX_stupItem[4], getStr4.c_str());
    Serial.println(msg);
    cnt++;
  }

  // --- SPEAKER ---
  String getStr5 = object[EX_stupItem[5]];
  if ((getStr5 != "") && (getStr5 != "-1") && (getStr5 != "null"))
  {
    TTS2_SPEAKER_NO = getStr5;
    sprintf(msg, "%s = %s", EX_stupItem[5], getStr5.c_str());
    Serial.println(msg);
    cnt++;
  }
  else
  {
    uint8_t speaker_no;
    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("setting", NVS_READONLY, &nvs_handle))
    {
      nvs_get_u8(nvs_handle, "speaker", &speaker_no);
      if (speaker_no > 66)
        speaker_no = 3;
      TTS2_SPEAKER_NO = String(speaker_no);
      nvs_close(nvs_handle);
    }
    sprintf(msg, "NVS %s = %d", EX_stupItem[5], speaker_no);
    Serial.println(msg);
    cnt++;
  }
  TTS2_PARMS = TTS2_SPEAKER + TTS2_SPEAKER_NO;

  // ttsSelect
  String getStr6 = object[EX_stupItem[6]];
  if (getStr6 != "" && (getStr6 != "null"))
  {
    String getData = getStr6;
    getData.toUpperCase();
    if (getData == "VOICETEXT")
      EX_TTS_TYPE = 0;
    if (getData == "GOOGLETTS")
      EX_TTS_TYPE = 1;
    if (getData == "VOICEVOX")
      EX_TTS_TYPE = 2;
    sprintf(msg, "%s = %s", EX_stupItem[6], EX_ttsName[EX_TTS_TYPE]);
    Serial.println(msg);
    cnt++;
  }

  // servoPort
  String getStr7 = object[EX_stupItem[7]];
  if (getStr7 != "" && (getStr7 != "null"))
  {
    String getData = getStr7;
    getData.toUpperCase();
    if (getData == "PORTC")
    {
      SV_PORT = "portC";
      SERVO_PIN_X = SV_PIN_X_CORE2_PC;
      SERVO_PIN_Y = SV_PIN_Y_CORE2_PC;
    }
    sprintf(msg, "%s = %s", EX_stupItem[7], getStr7.c_str());
    Serial.println(msg);
    cnt++;
  }

  // servo
  String getStr8 = object[EX_stupItem[8]];
  if (getStr8 != "" && (getStr8 != "null"))
  {
    String getData = getStr8;
    getData.toLowerCase();
    if (getData == "off")
      SV_USE = false;
    sprintf(msg, "%s = %s", EX_stupItem[8], getStr8.c_str());
    Serial.println(msg);
    cnt++;
  }

  // randomSpeak
  String getStr9 = object[EX_stupItem[9]];
  if (getStr9 != "" && (getStr9 != "null"))
  {
    String getData = getStr9;
    getData.toLowerCase();
    if (getData == "on")
      EX_RANDOM_SPEAK_ON_GET = true;
    sprintf(msg, "%s = %s", EX_stupItem[9], getStr9.c_str());
    Serial.println(msg);
    cnt++;
  }

  // mute
  String getStr10 = object[EX_stupItem[10]];
  if (getStr10 != "" && (getStr10 != "null"))
  {
    String getData = getStr10;
    getData.toLowerCase();
    if (getData == "on")
      EX_muteOn();
    sprintf(msg, "%s = %s", EX_stupItem[10], getStr10.c_str());
    Serial.println(msg);
    cnt++;
  }

  // led
  String getStr11 = object[EX_stupItem[11]];
  // uint8_t led_onoff;
  if (getStr11 != "" && (getStr11 != "null"))
  {
    String getData = getStr11;
    getData.toLowerCase();
    if (getData == "on")
    {
      EX_LED_OnOff_STATE = false;
      // led_onoff = 0;
    }
    sprintf(msg, "%s = %s", EX_stupItem[11], getStr11.c_str());
    Serial.println(msg);
    cnt++;
  }
  else
  {
    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("setting", NVS_READONLY, &nvs_handle))
    {
      uint8_t led_onoff;
      nvs_get_u8(nvs_handle, "led", &led_onoff);
      nvs_close(nvs_handle);
      if (led_onoff == 0)
      {
        EX_LED_OnOff_STATE = false;
      }
      sprintf(msg, "NVS %s = %s", EX_stupItem[11], getStr11.c_str());
      Serial.println(msg);
      cnt++;
    }
  }

  // toneMode
  String getStr12 = object[EX_stupItem[12]];
  if (getStr12 != "" && (getStr12 != "-1") && (getStr12 != "null"))
  {
    int getVal = getStr12.toInt();
    if (getVal < 0 || getVal > 3)
      getVal = 1;
    EX_TONE_MODE = getVal;
    sprintf(msg, "%s = %s", EX_stupItem[12], getStr12.c_str());
    Serial.println(msg);
    cnt++;
  }
  else
  {
    uint32_t nvs_handle;
    if (ESP_OK == nvs_open("setting", NVS_READONLY, &nvs_handle))
    {
      size_t mode;
      nvs_get_u32(nvs_handle, "toneMode", &mode);
      if (mode < 0 || mode > 3)
        mode = 1;
      EX_TONE_MODE = mode;
      nvs_close(nvs_handle);
      sprintf(msg, "NVS read %s = %d", EX_stupItem[12], mode);
      Serial.println(msg);
      cnt++;
    }
  }

  // timer
  String getStr13 = object[EX_stupItem[13]];
  if (getStr13 != "" && (getStr13 != "null"))
  {
    int getVal = getStr13.toInt();
    if (getVal <= 30)
      getVal = 30;

    if (getVal >= 3599)
      getVal = 180;

    EX_TIMER_SEC = getVal;
    sprintf(msg, "%s = %s", EX_stupItem[13], getStr13.c_str());
    Serial.println(msg);
    cnt++;
  }

  // keyLock
  String getStr14 = object[EX_stupItem[14]];
  if ((getStr14 != "") && (getStr14 != "null"))
  {
    String getData = getStr14;
    getData.toLowerCase();
    if (getData == "on")
      EX_KEYLOCK_STATE = true;
    sprintf(msg, "%s = %s", EX_stupItem[14], getStr14.c_str());
    Serial.println(msg);
    cnt++;
  }
  else
  {
    sprintf(msg, "Error %s is VOID or NULL ", EX_stupItem[14]);
    Serial.println(msg);
  }

  sprintf(msg, "startup.json total %d item read ", cnt);
  Serial.println(msg);

  return true;
}

//-----Ver1.08 ------------------------------------------
// #define DEBUG_ROOT
const char EX_INDEX_HTML_SD[] = "/index.html";
void EX_handleRoot()
{
  // *************************************************************
  //   http://xxx.xxx.xxx.xxx/ で htmlファイルをWEBで表示します。
  //   SD直下に "index.html"　を設置してください。
  //  -----------------------------------------------------------
  //   "xxx.xxx.xxx.xxx"　を接続したIPアドレスに変換します。
  // *************************************************************
  String msg;

  if (!SD.begin(GPIO_NUM_4, SPI, 25000000))
  {
    SD.end();

    msg = "Error handleRoot : cannot begin SD ";
    Serial.println(msg);
    server.send(200, "text/plain", msg);
    return;
  }

  auto fp = SD.open(EX_INDEX_HTML_SD, FILE_READ);
  if (!fp)
  {
    fp.close();
    SD.end();

    msg = "Error handleRoot : cannot open index.html in SD **";
    Serial.println(msg);
    server.send(200, "text/plain", msg);
    return;
  }

  // *** Buffer確保 ******
  size_t sz = fp.size();
  Serial.print("index.html file size = ");
  Serial.println(sz, DEC);

  char *buff;
  buff = (char *)malloc(sz + 1);
  if (!buff)
  {
    char msg[200];
    sprintf(msg, "ERROR:  Unable to malloc %d bytes for app\n", sz);
    server.send(200, "text/plain", String("NG"));
  }

  fp.read((uint8_t *)buff, sz);
  buff[sz] = 0;
  fp.close();
  SD.end();

  String htmlBuff = String(buff);

#ifndef DEBGU_ROOT
  // ** 本体のIP_ADDRに変換 **
  const char *findStr = "xxx.xxx.xxx.xxx";
  htmlBuff.replace(findStr, (const char *)EX_IP_ADDR.c_str());
#endif

  server.send(200, "text/html", htmlBuff);
  free(buff);
}

bool EX_initWifiJosn(DynamicJsonDocument &wifiJson)
{
  String wifiJsonInitStr = " { \"timeout\": 10, \"accesspoint\": [ ] }";
  DeserializationError error = deserializeJson(wifiJson, wifiJsonInitStr);
  if (error)
  {
    Serial.println("DeserializationError");
    return false;
  }
  String json_str;
  serializeJsonPretty(wifiJson, json_str);
  Serial.println(json_str);
  return true;
}

bool EX_wifiSelctFLSv(DynamicJsonDocument &wifiJson)
{
  // EX_isWifiSelectFLEnable = false;

  if (!SD.begin(GPIO_NUM_4, SPI, 25000000))
  { // SD無効な時
    Serial.println("SD disable ");
    SD.end();
    return false;
  }

  auto fl_SD = SD.open(EX_WIFISELECT_SD, FILE_WRITE);
  if (!fl_SD)
  {
    Serial.println("wifi-select.json cannot open ");
    SD.end();
    return false;
  }

  // JSONデータをシリアル化して書き込む
  serializeJsonPretty(wifiJson, fl_SD);
  fl_SD.close();
  SD.end();
  // EX_isWifiSelectFLEnable = true;
  return true;
}

bool EX_wifiSelctFLRd(DynamicJsonDocument &wifiJson)
{
  Serial.println("** EX_wifiSelectRD ***");

  // EX_isWifiSelectFLEnable = false;

  if (!SD.begin(GPIO_NUM_4, SPI, 25000000))
  { // SD無効な時
    Serial.println("SD disable ");
    SD.end();
    return false;
  }

  // "wifi-select.json"  file read
  auto fl_SD = SD.open(EX_WIFISELECT_SD, FILE_READ);
  if (!fl_SD)
  {
    Serial.println("wifi-select.json not open ");
    SD.end();
    return false;
  }

  DeserializationError error = deserializeJson(wifiJson, fl_SD);
  if (error)
  {
    Serial.println("DeserializationError in EX_wifiSelectRD func");
    SD.end();
    return false;
  }

  SD.end();
  // EX_isWifiSelectFLEnable = true;
  return true;
}

bool EX_wifiSelectConnect()
{
  DynamicJsonDocument wifiJson(EX_WIFIJSON_SIZE);

  if (!EX_wifiSelctFLRd(wifiJson))
  {
    Serial.println("wifi-selec.json file no read!");
    return false;
  }

  // ---------------------------------------
  int timeOut = wifiJson["timeout"];

  JsonArray jsonArray = wifiJson["accesspoint"];
  if (jsonArray.size() < 1)
  {
    Serial.println("no AP information in wifi-selec.json file!");
    return false;
  }

  // EX_isWifiSelectFLEnable = true;
  for (int index = 0; index < jsonArray.size(); ++index)
  {
    JsonObject object = jsonArray[index];
    String ssid = object["ssid"];
    String passWord = object["passwd"];

    // --- fix IP mode -------
    String ip_str = object["ip"];
    String gateway_str = object["gateway"];
    String subnet_str = object["subnet"];
    String dns_str = object["dns"];

    M5.Lcd.print("\nConnecting ");
    M5.Lcd.print(ssid);

    if (ip_str != "")
    { // 固定IPモードの処理 ----------------------------------------------
      int ip[4], gateway[4], subnet[4], dns[4];

      if (EX_strIPtoIntArray(ip_str, ip) && EX_strIPtoIntArray(gateway_str, gateway) && EX_strIPtoIntArray(subnet_str, subnet))
      {
        IPAddress fix_ip(ip[0], ip[1], ip[2], ip[3]);
        IPAddress fix_gateway(gateway[0], gateway[1], gateway[2], gateway[3]);
        IPAddress fix_subnet(subnet[0], subnet[1], subnet[2], subnet[3]);

        if (EX_strIPtoIntArray(dns_str, dns))
        { // DNS情報が有効ならば、４つの情報で接続
          IPAddress fix_dns(dns[0], dns[1], dns[2], dns[3]);

          if (!WiFi.config(fix_ip, fix_gateway, fix_subnet, fix_dns))
          {
            Serial.println("Failed to FixIP with DNS configure!");
            return false;
          }
          else
          {
            Serial.println("try connect fixIP : ip-gateway-subnet-dns ");
          }
        }
        else
        { // DNS情報が無効ならば、３つの情報で接続
          if (!WiFi.config(fix_ip, fix_gateway, fix_subnet))
          {
            Serial.println("Failed to Fix-WiFi no DNS configure!");
            return false;
          }
          else
          {
            Serial.println("try connect fixIP : ip-gateway-subnet ");
          }
        }
      }
    } // -------------------------------------------------------------------------

    WiFi.begin(ssid.c_str(), passWord.c_str());
    int loopCount = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
      M5.Display.print(".");
      Serial.print(".");
      delay(500);
      // 設定されたタイムアウト秒接続できなかったら抜ける
      if (loopCount++ > timeOut * 2)
      {
        break;
      }
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      EX_SSID = ssid;
      EX_SSID_PASSWD = passWord;
      return true;
    }
  }
  return false;
}

void EX_handle_wifiSelect()
{
  EX_tone(2);
  DynamicJsonDocument wifiJson(EX_WIFIJSON_SIZE);

  String init_get_str = "";
  String ssid_get_str = "";
  String passwd_get_str = "";
  String remove_get_str = "";
  init_get_str = server.arg("init");
  ssid_get_str = server.arg("ssid");
  passwd_get_str = server.arg("passwd");
  remove_get_str = server.arg("remove");

  // fix IP mode ----
  String ip_get_str = "";
  String gateway_get_str = "";
  String subnet_get_str = "";
  String dns_get_str = "";
  ip_get_str = server.arg("ip");
  gateway_get_str = server.arg("gateway");
  subnet_get_str = server.arg("subnet");
  dns_get_str = server.arg("dns");

  // SDからデータを読む
  if (init_get_str == "on")
  {
    if (!EX_initWifiJosn(wifiJson))
    {
      server.send(200, "text/plain", String("NG"));
      Serial.println("faile to init wifiSelectJson file");
      return;
    }

    if (!EX_wifiSelctFLSv(wifiJson))
    {
      server.send(200, "text/plain", String("NG"));
      Serial.println("faile to Save to SD");
      return;
    }

    if ((ssid_get_str == "") && (passwd_get_str == ""))
    {
      server.send(200, "text/plain", String("OK"));
      return;
    }
  }

  // SDからデータを読む
  if (!EX_wifiSelctFLRd(wifiJson))
  {
    server.send(200, "text/plain", String("NG"));
    Serial.println("faile to Read from SD");
    return;
  }

  if ((ssid_get_str == "") && (passwd_get_str == "") && (remove_get_str == ""))
  {
    // HTMLデータを出力する
    String html = "<html><body><pre>";
    serializeJsonPretty(wifiJson, html);
    html += "</pre></body></html>";
    Serial.println(html);
    server.send(200, "text/html", String(HEAD) + html);
    return;
  }

  if (remove_get_str != "")
  {
    uint8_t ap_no = 0;
    ap_no = remove_get_str.toInt();
    String msg = "remove accesspoint : " + remove_get_str;
    Serial.println(msg);

    JsonArray jsonArray = wifiJson["accesspoint"];
    uint8_t arraySize = jsonArray.size();
    Serial.print("arraySize = ");
    Serial.println(arraySize, DEC);

    if ((ap_no >= 0) && (ap_no < arraySize))
    {
      jsonArray.remove(ap_no); // データ削除
      // SDに保存

      if (!EX_wifiSelctFLSv(wifiJson))
      {
        server.send(200, "text/plain", String("NG"));
        Serial.println("faile to Save to SD");
        return;
      }
      String msgJson = "";
      serializeJsonPretty(wifiJson, msgJson);
      Serial.println(msgJson);
      server.send(200, "text/plain", String("OK"));
      return;
    }
    else
    {
      server.send(200, "text/plain", String("NG"));
      Serial.print("faile ap_no = ");
      Serial.println(ap_no, DEC);
      return;
    }
  }

  if ((ssid_get_str != "") && (passwd_get_str != ""))
  {
    JsonArray jsonArray = wifiJson["accesspoint"];
    JsonObject new_ap = jsonArray.createNestedObject();
    new_ap["ssid"] = ssid_get_str;
    new_ap["passwd"] = passwd_get_str;
    new_ap["ip"] = ip_get_str;
    new_ap["gateway"] = gateway_get_str;
    new_ap["subnet"] = subnet_get_str;
    new_ap["dns"] = dns_get_str;

    if (!EX_wifiSelctFLSv(wifiJson))
    {
      server.send(200, "text/plain", String("NG"));
      Serial.println("faile to Save to SD");
      return;
    }
    server.send(200, "text/plain", String("OK"));
    return;
  }

  // never
  server.send(200, "text/plain", String("NG"));
}

void EX_handle_startup()
{
  EX_tone(2);
  DynamicJsonDocument startupJson(EX_STARTUPJSON_SIZE);

  // -------------------------------------------------------
  String get_str = server.arg("tx");
  if (get_str != "")
  {
    String val_str = "";
    char msg[100] = "";

    bool success = EX_getStartup(get_str, val_str, startupJson);
    if (success)
    {
      sprintf(msg, "%s = %s", get_str.c_str(), val_str.c_str());
      Serial.println(msg);
      server.send(200, "text/plain", String(msg));
      return;
    }
    else
    {
      server.send(200, "text/plain", String("NG"));
      return;
    }
  }

  if (EX_setGetStrToStartSetting("openAiApiKey", startupJson))
  {
    server.send(200, "text/plain", String("OK"));
    return;
  }

  if (EX_setGetStrToStartSetting("voiceTextApiKey", startupJson))
  {
    server.send(200, "text/plain", String("OK"));
    return;
  }

  if (EX_setGetStrToStartSetting("voicevoxApiKey", startupJson))
  {
    server.send(200, "text/plain", String("OK"));
    return;
  }

  if (EX_setGetStrToStartSetting("volume", startupJson))
  {
    server.send(200, "text/plain", String("OK"));
    return;
  }

  if (EX_setGetStrToStartSetting("lang", startupJson))
  {
    server.send(200, "text/plain", String("OK"));
    return;
  }

  if (EX_setGetStrToStartSetting("voicevoxSpeakerNo", startupJson))
  {
    server.send(200, "text/plain", String("OK"));
    return;
  }

  if (EX_setGetStrToStartSetting("ttsSelect", startupJson))
  {
    server.send(200, "text/plain", String("OK"));
    return;
  }

  if (EX_setGetStrToStartSetting("servoPort", startupJson))
  {
    server.send(200, "text/plain", String("OK"));
    return;
  }

  if (EX_setGetStrToStartSetting("servo", startupJson))
  {
    server.send(200, "text/plain", String("OK"));
    return;
  }

  if (EX_setGetStrToStartSetting("randomSpeak", startupJson))
  {
    server.send(200, "text/plain", String("OK"));
    return;
  }

  if (EX_setGetStrToStartSetting("mute", startupJson))
  {
    server.send(200, "text/plain", String("OK"));
    return;
  }

  if (EX_setGetStrToStartSetting("led", startupJson))
  {
    server.send(200, "text/plain", String("OK"));
    return;
  }

  if (EX_setGetStrToStartSetting("toneMode", startupJson))
  {
    server.send(200, "text/plain", String("OK"));
    return;
  }

  if (EX_setGetStrToStartSetting("timer", startupJson))
  {
    server.send(200, "text/plain", String("OK"));
    return;
  }

  // -------------------------------------------------------
  // HTML形式でファイルを表示する
  {
    String val_str = "";
    char msg[100] = "";

    // SDからデータを読む
    if (!EX_startupFLRd(startupJson))
    {
      Serial.println("faile to Read startup.json from SD");
      server.send(200, "text/plain", String("NG"));
      return;
    }
    // 整形したJSONデータを出力するHTMLデータを作成する
    String html = "<html><body><pre>";
    serializeJsonPretty(startupJson, html);
    html += "</pre></body></html>";

    // HTMLデータをシリアルに出力する
    Serial.println(html);
    server.send(200, "text/html", html);
    return;
  }
  // -------------------------------------------------------

  server.send(200, "text/plain", String("NG"));
}

bool EX_startupFLRd(DynamicJsonDocument &startupJson)
{
  if (!SD.begin(GPIO_NUM_4, SPI, 25000000))
  { // SD無効な時
    Serial.println("SD disable ");
    SD.end();
    return false;
  }

  auto fl_SD = SD.open(EX_STARTUP_SD, FILE_READ);
  if (!fl_SD)
  {
    Serial.println("startup.json not open ");
    SD.end();
    return false;
  }

  DeserializationError error = deserializeJson(startupJson, fl_SD);
  if (error)
  {
    Serial.println("DeserializationError in EX_startupRD func");
    SD.end();
    return false;
  }

  SD.end();
  return true;
}

bool EX_startupFLSv(DynamicJsonDocument &startupJson)
{

  if (!SD.begin(GPIO_NUM_4, SPI, 25000000))
  { // SD無効な時
    Serial.println("SD disable ");
    SD.end();
    return false;
  }

  auto fl_SD = SD.open(EX_STARTUP_SD, FILE_WRITE);
  if (!fl_SD)
  {
    Serial.println("startup.json cannot open ");
    SD.end();
    return false;
  }

  // JSONデータをシリアル化して書き込む
  serializeJsonPretty(startupJson, fl_SD);
  fl_SD.close();
  SD.end();
  // EX_isWifiSelectFLEnable = true;
  return true;
}

bool EX_setStartup(String item, String data, DynamicJsonDocument &startupJson)
{
  // SDからデータを読む
  if (!EX_startupFLRd(startupJson))
  {
    Serial.println("faile to Read startup.json from SD");
    return false;
  }

  JsonArray jsonArray = startupJson["startup"];
  JsonObject object = jsonArray[0];
  object[item] = data;

  bool success = EX_startupFLSv(startupJson);
  if (!success)
  {
    return false;
  }
  return true;
}

bool EX_getStartup(String item, String &data, DynamicJsonDocument &startupJson)
{
  // SDからデータを読む
  if (!EX_startupFLRd(startupJson))
  {
    Serial.println("faile to Read startup.json from SD");
    return false;
  }

  JsonArray jsonArray = startupJson["startup"];
  JsonObject object = jsonArray[0];
  String data_tmp = object[item];
  if (data_tmp != "")
  {
    data = data_tmp;
    return true;
  }
  data = "";
  return false;
}

bool EX_setGetStrToStartSetting(const char *item, DynamicJsonDocument &startupJson)
{
  String get_str = server.arg(item);
  if (get_str != "")
  {
    return (EX_setStartup(String(item), get_str, startupJson));
  }
  return false;
}

//-----Ver1.06 ----------------------------------------------------------
void EX_handle_sysInfo()
{
  EX_tone(2);

  // -- system Information 個別情報送信　-----
  String tx_arg = server.arg("tx");
  String tx_data = "";

  if (tx_arg != "")
  {
    if (EX_sysInfoGet(tx_arg, tx_data))
    {
      server.send(200, "text/plain", tx_data);
      return;
    }
    else
    {
      server.send(200, "text/plain", String("NG"));
      return;
    }
  }

  String disp_str = server.arg("disp");
  if (disp_str == "off")
  {
    EX_sysInfoDispEnd();
    EX_tone(2);
    server.send(200, "text/plain", String("OK"));
    return;
  }

  uint8_t mode_no = 0;
  EX_SYSINFO_MSG = "";
  String mode_str = server.arg("mode");
  if (mode_str != "")
  {
    if (mode_str == "all")
    {
      EX_sysInfoDispMake(0);
      EX_SYSINFO_MSG += "\n";
      EX_sysInfoDispMake(1);
      server.send(200, "text/plain", EX_SYSINFO_MSG);
      return;
    }
    mode_no = mode_str.toInt();
  }

  if (disp_str == "on")
  {
    EX_sysInfoDispStart(mode_no);
    server.send(200, "text/plain", EX_SYSINFO_MSG);
    return;
  }
  else
  {
    if (mode_str == "")
    {
      // send all sysInfo
      EX_sysInfoDispMake(0);
      EX_SYSINFO_MSG += "\n";
      EX_sysInfoDispMake(1);
      server.send(200, "text/plain", EX_SYSINFO_MSG);
      return;
    }
    else
    {
      EX_sysInfoDispMake(mode_no);
      server.send(200, "text/plain", EX_SYSINFO_MSG);
      return;
    }
  }

  server.send(200, "text/plain", String("OK"));
  return;
}

void EX_ttsDo(char *text, char *tts_parms)
{
  switch (EX_TTS_TYPE)
  {
  case 0: // VoiceText
    VoiceText_tts(text, tts_parms);
    break;

  case 1: // GoogleTTS
    google_tts(text, (char *)LANG_CODE.c_str());
    break;

  case 2: // VOICEVOX
    Voicevox_tts(text, (char *)TTS2_PARMS.c_str());
    break;

  default:
    // Voicevox_tts(text, (char *)TTS2_PARMS.c_str());
    break;
  }
}

bool EX_isJP()
{
  if (LANG_CODE == String(LANG_CODE_JP) || (EX_TTS_TYPE == 0) || (EX_TTS_TYPE == 2))
    return true;
  else
    return false;
}

// ----- LED表示用ラッパー関数　-----------
void EX_setColorLED2(uint16_t n, uint32_t c)
{
  if (EX_LED_OnOff_STATE)
    pixels.setPixelColor(n, c);
}

void EX_setColorLED4(uint16_t n, uint8_t r, uint8_t g, uint8_t b)
{
  if (EX_LED_OnOff_STATE)
    pixels.setPixelColor(n, r, g, b);
}

static uint32_t EX_ColorLED3(uint8_t r, uint8_t g, uint8_t b)
{
  if (EX_LED_OnOff_STATE)
    return (pixels.Color(r, g, b));
  else
    return 0;
}

void EX_showLED()
{
  if (EX_LED_OnOff_STATE)
    pixels.show();
}

void EX_clearLED()
{
  if (EX_LED_OnOff_STATE)
    pixels.clear();
}

//-----Ver1.05 ----------------------------------------------------------
void EX_errStop(const char *msg)
{
  M5.Display.println();
  M5.Display.println(msg);
  Serial.println("");
  Serial.println("Stop : Fatal Error Occurred!");
  delay(5000);

  for (;;)
  {
    delay(1000);
  }
}

void EX_errReboot(const char *msg)
{
  M5.Display.println();
  M5.Display.println(msg);
  Serial.println("");
  Serial.println("Reset : Fatal Error Occurred!");
  delay(5000);
  ESP.restart();
}

bool EX_chatDocInit()
{
  if (!SPIFFS.begin(true))
  {
    String errorMsg1 = "*** An Error has occurred while mounting SPIFFS *** ";
    String errorMsg2 = "*** FATAL ERROR : cannot READ/WRITE CHAT_DOC FILE !!!! ***";
    Serial.println(errorMsg1);
    Serial.println(errorMsg2);
    M5.Lcd.print(errorMsg1);
    M5.Lcd.print(errorMsg2);
    return false;
  }

  File fl_SPIFFS = SPIFFS.open(EX_CHATDOC_SPI, "r");
  if (!fl_SPIFFS)
  {
    fl_SPIFFS.close();
    String errorMsg1 = "*** Failed to open file for reading *** ";
    String errorMsg2 = "*** FATAL ERROR : cannot READ CHAT_DOC FILE !!!! ***";
    Serial.println(errorMsg1);
    Serial.println(errorMsg2);
    M5.Lcd.print(errorMsg1);
    M5.Lcd.print(errorMsg2);

    init_chat_doc(EX_json_ChatString.c_str());
    return false;
  }

  DeserializationError error = deserializeJson(CHAT_DOC, fl_SPIFFS);
  fl_SPIFFS.close();

  if (error)
  { // ファイルの中身が壊れていた時の処理 ---------
    Serial.println("Failed to deserialize JSON");
    // if (!init_chat_doc(json_ChatString.c_str()))
    if (!init_chat_doc(EX_json_ChatString.c_str()))
    {
      String errorMsg1 = "*** Failed to init chat_doc JSON in SPIFFS *** ";
      String errorMsg2 = "*** FATAL ERROR : cannot READ/WRITE CHAT_DOC FILE !!!! ***";
      Serial.println(errorMsg1);
      Serial.println(errorMsg2);
      M5.Lcd.print(errorMsg1);
      M5.Lcd.print(errorMsg2);
      return false;
    }
    else
    { // JSONファイルをSPIFF に保存
      fl_SPIFFS = SPIFFS.open(EX_CHATDOC_SPI, "w");
      if (!fl_SPIFFS)
      {
        fl_SPIFFS.close();
        String errorMsg1 = "*** Failed to open file for writing *** ";
        String errorMsg2 = "*** FATAL ERROR : cannot WRITE CHAT_DOC FILE !!!! ***";
        Serial.println(errorMsg1);
        Serial.println(errorMsg2);
        M5.Lcd.print(errorMsg1);
        M5.Lcd.print(errorMsg2);
        return false;
      }
      serializeJson(CHAT_DOC, fl_SPIFFS);
      fl_SPIFFS.close();
      Serial.println("initial chat_doc data store in SPIFFS");
    }
  }

  serializeJson(CHAT_DOC, INIT_BUFFER);
  Role_JSON = INIT_BUFFER;
  String json_str;
  serializeJsonPretty(CHAT_DOC, json_str); // 文字列をシリアルポートに出力する
  Serial.println(json_str);
  return true;
}

// void EX_handle_test()
// {
//   EX_tone(2);
//   String arg_str = "";
//   String val_str = "";
//   char msg[100] = "";

//   arg_str = server.arg("tx");
//   Serial.println(arg_str);

//   if (arg_str == "")
//   {
//     server.send(200, "text/plain", String("NG"));
//   }

//   bool success = EX_getStartup(arg_str, val_str);
//   if (success)
//   {
//     sprintf(msg, "%s = %s", arg_str.c_str(), val_str.c_str());
//     Serial.println(msg);
//     server.send(200, "text/plain", String(msg));
//   }
//   else
//   {
//     server.send(200, "text/plain", String("NG"));
//   }
// }

void EX_handle_role1()
{
  // ファイルを読み込み、クライアントに送信する
  EX_tone(2);
  server.send(200, "text/html", ROLE1_HTML);
}

void EX_handle_role1_set()
{
  // POST以外は拒否
  if (server.method() != HTTP_POST)
  {
    return;
  }

  String role = server.arg("plain");
  if (role != "")
  {
    init_chat_doc(INIT_BUFFER.c_str());
    JsonArray jsonArray = CHAT_DOC["messages"];

    if (jsonArray.size() != 2)
    { // role-user, role-systemのデータ各１つづなければ初期化
      Serial.println("EX_json_ChatString init done! ");
      init_chat_doc(EX_json_ChatString.c_str());
      jsonArray = CHAT_DOC["messages"];
    }

    // role-system-content に登録
    JsonObject systemMessage = jsonArray[1];
    systemMessage["content"] = role;
  }
  else
  {
    init_chat_doc(EX_json_ChatString.c_str());
    // 会話履歴をクリア
    // chatHistory.clear();
  }
  // 会話履歴をクリア
  chatHistory.clear();

  INIT_BUFFER = "";
  serializeJson(CHAT_DOC, INIT_BUFFER);
  Serial.println("INIT_BUFFER = " + INIT_BUFFER);
  Role_JSON = INIT_BUFFER;

  // JSONデータをspiffsへ出力する
  save_json();

  // 整形したJSONデータを出力するHTMLデータを作成する
  String html = "<html><body><pre>";
  serializeJsonPretty(CHAT_DOC, html);
  html += "</pre></body></html>";

  // HTMLデータをシリアルに出力する
  Serial.println(html);
  server.send(200, "text/html", html);
}

bool EX_strIPtoIntArray(String strIPaddr, int *iAddr)
{ // in  : String strIPaddr --> "192.168.0.1"
  // out : int iAddr[4]     --> iAddr[0]=192,iAddr[1]=168,iAddr[2]=0,iAddr[3]=1
  // ret : true(success), false(error)
  // ---------------------------------------------------------------------------

  long addr = 0;
  struct in_addr tag;
  unsigned char szIp[4];

  // IPアドレス文字列を変換
  addr = inet_addr(strIPaddr.c_str());
  if (addr == -1)
  {
    Serial.print("IP addr Conver Error : ");
    Serial.println(strIPaddr);
    return false;
  }

  // バイナリ形式からunsigned charに変換
  memcpy(szIp, &addr, sizeof(long));

  // int型にキャスト
  iAddr[0] = (int)szIp[0];
  iAddr[1] = (int)szIp[1];
  iAddr[2] = (int)szIp[2];
  iAddr[3] = (int)szIp[3];

  // char tmp_str[40];
  // sprintf(tmp_str, "Convert [%d].[%d].[%d].[%d]", iAddr[0], iAddr[1], iAddr[2], iAddr[3]);
  // Serial.println(tmp_str);
  return true;
}

//-----Ver1.04 ----------------------------------------------------------
void EX_toneOn()
{
  M5.Speaker.tone(1000, 100);
}

void EX_tone(int mode)
{
  switch (EX_TONE_MODE)
  {
  case 0: // always toneOff
    break;

  case 1: // toneOn when buttons pressed
    if (mode == EX_TONE_MODE)
      EX_toneOn();
    break;

  case 2: // toneOn whenn external command rcv
    if (mode == EX_TONE_MODE)
      EX_toneOn();
    break;

  case 3: // toneOn every time
    EX_toneOn();
    break;

  default:
    break;
  }
}

void EX_handle_shutdown()
{
  EX_tone(2);

  String reboot_get_str = server.arg("reboot");
  String time_get_str = server.arg("time");
  uint16_t time_sec = EX_SHUTDOWN_MIN_TM;

  if (time_get_str != "")
  {
    time_sec = time_get_str.toInt();
    String msg = "time = " + time_get_str;
    Serial.println(msg);

    if ((time_sec < 0) || (time_sec > 60))
    {
      server.send(200, "text/plain", String("NG"));
      Serial.print("faile time_sec = ");
      Serial.println(time_sec, DEC);
      return;
    }
  }

  if (reboot_get_str == "on")
  {
    server.send(200, "text/plain", String("OK"));

    // min ...  sec Wait
    if (time_sec < EX_SHUTDOWN_MIN_TM)
    {
      time_sec = EX_SHUTDOWN_MIN_TM;
    }

    // --- reboot
    delay(time_sec * 1000);
    ESP.restart();
    // never
    return;
  }

  // --- shutdown
  server.send(200, "text/plain", String("OK"));

  // min ... sec Wait
  if (time_sec < EX_SHUTDOWN_MIN_TM)
  {
    time_sec = EX_SHUTDOWN_MIN_TM;
  }

  delay(time_sec * 1000);
  M5.Power.powerOff();
  // never
  return;
}

// bool EX_wifiFLRd()
// {
//   if (!SD.begin(GPIO_NUM_4, SPI, 25000000))
//   { // SD無効な時
//     Serial.println("SD disable ");
//     SD.end();
//     return false;
//   }

//   auto fl_SD = SD.open(EX_WIFI_SD, FILE_READ);
//   if (!fl_SD)
//   {
//     SD.end();
//     Serial.println("wifi.txt not open ");
//     return false;
//   }

//   size_t sz = fl_SD.size();
//   char buf[sz + 1];
//   fl_SD.read((uint8_t *)buf, sz);
//   buf[sz] = 0;
//   fl_SD.close();
//   SD.end();

//   int y = 0;
//   for (int x = 0; x < sz; x++)
//   {
//     if (buf[x] == 0x0a || buf[x] == 0x0d)
//       buf[x] = 0;
//     else if (!y && x > 0 && !buf[x - 1] && buf[x])
//       y = x;
//   }

//   EX_SSID = buf;
//   EX_SSID_PASSWD = &buf[y];

//   Serial.print("\nSSID = ");
//   Serial.println(EX_SSID);
//   Serial.print("SSID_PASSWD = ");
//   Serial.println(EX_SSID_PASSWD);
//   if ((EX_SSID == "") || (EX_SSID_PASSWD == ""))
//   {
//     Serial.println("ssid or passwd is void ");
//     return false;
//   }

//   return true;
// }

bool EX_wifiNoSetupFileConnect()
{
  // 設定ファイルが無効の場合は、前回接続情報での接続
  WiFi.begin();

  // 待機
  int loopCount10sec = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    M5.Display.print(".");
    Serial.print(".");
    delay(500);
    // 10秒以上接続できなかったら false
    if (loopCount10sec++ > 10 * 2)
    {
      return false;
    }
  }
  return true;
}

bool EX_wifiSmartConfigConnect()
{
  // ---------------------------------------
  // SmartConfig接続
  M5.Display.println("");
  Serial.println("");
  WiFi.mode(WIFI_STA);
  WiFi.beginSmartConfig();
  M5.Display.println("Waiting for SmartConfig");
  Serial.println("Waiting for SmartConfig");

  // ---------------------------------------
  int loopCount30sec = 0;
  while (!WiFi.smartConfigDone())
  {
    delay(500);
    M5.Display.print("#");
    Serial.print("#");
    // 30秒以上 smartConfigDoneできなかったら false
    if (loopCount30sec++ > 30 * 2)
    {
      return false;
    }
  }

  // ---------------------------------------
  // Wi-fi接続待ち
  M5.Display.println("");
  Serial.println("");
  M5.Display.println("Waiting for WiFi");
  Serial.println("Waiting for WiFi");

  int loopCount60sec = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    M5.Display.print(".");
    Serial.print(".");
    // 60秒以上接続できなかったら false
    if (loopCount60sec++ > 60 * 2)
    {
      return false;
    }
  }

  return true;
}

// #define EX_SMART_CONFIG_TEST
bool EX_wifiConnect()
{
  Serial.println("Connecting to WiFi");
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  M5.Lcd.print("Connecting");

#ifndef EX_SMART_CONFIG_TEST
  // "wifi-select.json" の接続
  if (EX_wifiSelectConnect())
  {
    Serial.println("\nwifi-select.json wifi CONNECT");
    return true;
  }

  // "wifi.txt" の接続
  // if (EX_wifiTxtConnect())
  // {
  //   Serial.println("\nwifi.txt wifi CONNECT");
  //   return true;
  // }

  // 前回接続情報での接続
  if (EX_wifiNoSetupFileConnect())
  {
    Serial.println("\nprivious Setup wifi CONNECT");
    return true;
  }
#endif

  // SmartConfigでの接続
  if (EX_wifiSmartConfigConnect())
  {
    Serial.println("\nSmartConfigConnect wifi CONNECT");
    return true;
  }

  // 全てに失敗した場合
  Serial.println("\n*** ALL FAIL to wifi CONNECT ***");
  return false;
}

bool EX_sysInfoGet(String txArg, String &txData)
{
  String msg = "";
  char msg2[100];

  if (txArg == "version")
  {
    txData = EX_VERSION;
  }
  else if (txArg == "IP_Addr")
  {
    txData = "IP_Addr = " + EX_IP_ADDR;
    Serial.println(txData);
  }
  else if (txArg == "SSID")
  {
    txData = "SSID = " + EX_SSID;
  }
  else if (txArg == "batteryLevel")
  {
    sprintf(msg2, "batteryLevel = %d %%", EX_getBatteryLevel());
    txData = msg2;
  }
  else if (txArg == "volume")
  {
    sprintf(msg2, "volume = %d", EX_VOLUME);
    txData = msg2;
  }
  else if (txArg == "timer")
  {
    sprintf(msg2, "timer = %d sec", EX_TIMER_SEC);
    txData = msg2;
  }
  else if (txArg == "mute")
  {
    if (EX_MUTE_ON)
    {
      msg = "mute = on";
      txData = msg;
    }
    else
    {
      msg = "mute = off";
      txData = msg;
    }
  }
  else if (txArg == "randomSpeak")
  {
    if (EX_RANDOM_SPEAK_STATE)
    {
      msg = "randomSpeak = on";
      txData = msg;
    }
    else
    {
      msg = "randomSpeak = off";
      txData = msg;
    }
  }
  else if (txArg == "uptime")
  {
    uint32_t uptime = millis() / 1000;
    uint16_t up_sec = uptime % 60;
    uint16_t up_min = (uptime / 60) % 60;
    uint16_t up_hour = uptime / (60 * 60);
    sprintf(msg2, "uptime = %02d:%02d:%02d", up_hour, up_min, up_sec);
    txData = msg2;
  }
  else if (txArg == "toneMode")
  {
    sprintf(msg2, "toneMode = %d", EX_TONE_MODE);
    txData = msg2;
  }
  else if (txArg == "servo")
  {
    if (SV_USE)
    {
      msg = "servo = on";
      txData = msg;
    }
    else
    {
      msg = "servo = off";
      txData = msg;
    }
  }
  else if (txArg == "servoPort")
  {
    if (SV_PORT == "portC")
    {
      msg = "servoPort = portC";
      txData = msg;
    }
    else
    {
      msg = "servo = portA";
      txData = msg;
    }
  }
  else if (txArg == "led")
  {
    if (EX_LED_OnOff_STATE == true)
    {
      msg = "led = on";
      txData = msg;
    }
    else
    {
      msg = "led = off";
      txData = msg;
    }
  }
  else if (txArg == "WK_errorNo")
  {
    sprintf(msg2, "WK_errorNo = %d", EX_LAST_WK_ERROR_NO);
    txData = msg2;
  }
  else if (txArg == "WK_errorCode")
  {
    sprintf(msg2, "WK_errorCode = %d", EX_LAST_WK_ERROR_CODE);
    txData = msg2;
  }

  // ---- Network Settings ---
  else if (txArg == "SSID_PASSWD")
  {
    txData = "SSID_PASSWD = " + EX_SSID_PASSWD;
  }
  else if (txArg == "openAiApiKey")
  {
    txData = "openAiApiKey = " + OPENAI_API_KEY;
  }
  else if (txArg == "voiceTextApiKey")
  {
    txData = "voiceTextApiKey = " + EX_VOICETEXT_API_KEY;
  }
  else if (txArg == "voicevoxApiKey")
  {
    txData = "voicevoxApiKey = " + VOICEVOX_API_KEY;
  }
  else
  {
    Serial.println(txArg);
    Serial.println(txData);
    return false;
  }

  Serial.println(txArg);
  Serial.println(txData);
  return true;
}

// ------- Ver1.03 --------------------------------------------
void EX_LED_allOff()
{ // 全てのLEDを消灯

  // 一時的にLEDを制御できるようにする。
  bool tmpState = EX_LED_OnOff_STATE;
  EX_LED_OnOff_STATE = true;

  for (int i = 0; i < NUM_LEDS; i++)
  {
    EX_setColorLED2(i, EX_ColorLED3(0, 0, 0));
  }
  EX_showLED();

  EX_LED_OnOff_STATE = tmpState;
  // delay(500); // 0.5秒待機
}

void EX_sysInfoDispStart(uint8_t mode_no)
{
  if (!EX_SYSINFO_DISP)
  {
    // EX_muteOn();
    avatar.stop();
    EX_randomSpeakStop2();
    EX_timerStop2();
    M5.Display.setTextFont(1);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(WHITE, BLACK);
    M5.Display.setTextDatum(0);
  }

  M5.Display.setCursor(0, 0);
  delay(50);
  M5.Display.fillScreen(BLACK);
  delay(50);

  EX_sysInfoDispMake(mode_no);
  M5.Display.print(EX_SYSINFO_MSG);

  if (mode_no == 99)
  { // Test mode Display
    // EX_errStop("EX_errFatal_STOP Called !!");
    // EX_errReboot("EX_errFatal_REBOOT Called !!");
  }
  EX_SYSINFO_DISP = true;
}

void EX_sysInfoDispEnd()
{
  if (!EX_SYSINFO_DISP)
  {
    return;
  }

  avatar.start();
  delay(200);
  // EX_muteOff();
  EX_SYSINFO_DISP = false;
}

uint8_t EX_getBatteryLevel()
{
  return (M5.Power.getBatteryLevel());
}

void EX_sysInfoDispMake(uint8_t mode_no)
{
  switch (mode_no)
  {
  case 0:
    EX_sysInfo_m00_DispMake();
    break;

  case 1:
    EX_sysInfo_m01_DispMake();
    break;

  case 99:
    EX_sysInfo_m99_DispMake();
    break;

  default:
    EX_sysInfo_m00_DispMake();
    break;
  }
}

void EX_sysInfo_m01_DispMake()
{
  String msg = "";
  char msg2[100];

  EX_SYSINFO_MSG += "\nSSID_PASSWD = " + EX_SSID_PASSWD;
  EX_SYSINFO_MSG += "\nopenAiApiKey = " + OPENAI_API_KEY;
  EX_SYSINFO_MSG += "\nvoiceTextApiKey = " + EX_VOICETEXT_API_KEY;
  EX_SYSINFO_MSG += "\nvoicevoxApiKey = " + VOICEVOX_API_KEY;
  sprintf(msg2, "\ntimer = %d sec", EX_TIMER_SEC);
  EX_SYSINFO_MSG += msg2;

  if (EX_MUTE_ON)
  {
    msg = "\nmute = on";
    EX_SYSINFO_MSG += msg;
  }
  else
  {
    msg = "\nmute = off";
    EX_SYSINFO_MSG += msg;
  }

  if (EX_KEYLOCK_STATE)
  {
    msg = "\nkeyLock = on";
    EX_SYSINFO_MSG += msg;
  }
  else
  {
    msg = "\nkeyLock = off";
    EX_SYSINFO_MSG += msg;
  }

  sprintf(msg2, "\ntoneMode = %d", EX_TONE_MODE);
  EX_SYSINFO_MSG += msg2;
}

void EX_sysInfo_m00_DispMake()
{
  String msg = "";
  char msg2[100];

  // EX_SYSINFO_MSG = "*** System Information ***\n";
  EX_SYSINFO_MSG = "";
  EX_SYSINFO_MSG += EX_VERSION;
  EX_SYSINFO_MSG += "\nIP_Addr = " + EX_IP_ADDR;
  EX_SYSINFO_MSG += "\nSSID = " + EX_SSID;

  sprintf(msg2, "\nbatteryLevel = %d %%", EX_getBatteryLevel());
  EX_SYSINFO_MSG += msg2;

  sprintf(msg2, "\nvolume = %d", EX_VOLUME);
  EX_SYSINFO_MSG += msg2;
  EX_SYSINFO_MSG += "\nttsSelect = " + String(EX_ttsName[EX_TTS_TYPE]);
  EX_SYSINFO_MSG += "\nvoicevoxSpeakerNo = " + TTS2_SPEAKER_NO;
  EX_SYSINFO_MSG += "\nlang = " + LANG_CODE;

  if (EX_RANDOM_SPEAK_STATE)
  {
    msg = "\nrandomSpeak = on";
    EX_SYSINFO_MSG += msg;
  }
  else
  {
    msg = "\nrandomSpeak = off";
    EX_SYSINFO_MSG += msg;
  }

  if (SV_USE)
  {
    sprintf(msg2, "\nservo = on");
    EX_SYSINFO_MSG += msg2;
  }
  else
  {
    sprintf(msg2, "\nservo = off");
    EX_SYSINFO_MSG += msg2;
  }

  sprintf(msg2, "\nservoPort = %s", SV_PORT);
  EX_SYSINFO_MSG += msg2;

  if (EX_LED_OnOff_STATE)
  {
    sprintf(msg2, "\nled = on");
    EX_SYSINFO_MSG += msg2;
  }
  else
  {
    sprintf(msg2, "\nled = off");
    EX_SYSINFO_MSG += msg2;
  }

  uint32_t uptime = millis() / 1000;
  uint16_t up_sec = uptime % 60;
  uint16_t up_min = (uptime / 60) % 60;
  uint16_t up_hour = uptime / (60 * 60);
  sprintf(msg2, "\nuptime = %02d:%02d:%02d", up_hour, up_min, up_sec);
  EX_SYSINFO_MSG += msg2;

  sprintf(msg2, "\nWK_ERR: No = %d Code = %d", EX_LAST_WK_ERROR_NO, EX_LAST_WK_ERROR_CODE);
  EX_SYSINFO_MSG += msg2;
}

void EX_sysInfo_m99_DispMake()
{
  EX_SYSINFO_MSG = "*** Test Mode  *** ";
}

void EX_randomSpeakStop2()
{
  if ((RANDOM_TIME != -1) || (EX_RANDOM_SPEAK_STATE == false))
  {
    return;
  }

  RANDOM_TIME = -1;
  EX_RANDOM_SPEAK_STATE = false;
  EX_RANDOM_SPEAK_ON_GET = false;
  // EX_RANDOM_SPEAK_OFF_GET = false;
}

void EX_timerStop2()
{
  if (!EX_TIMER_STARTED)
  {
    return;
  }

  // --- Timer を途中で停止 ------
  EX_TIMER_STARTED = false;
  EX_TIMER_GO_GET = false;
  EX_TIMER_STOP_GET = false;
  EX_TIMER_ELEAPSE_SEC = 0;

  // 全てのLEDを消灯
  EX_LED_allOff();
  delay(50); // 0.5秒待機
}

void EX_muteOn()
{
  M5.Speaker.setVolume(0);
  M5.Speaker.setChannelVolume(m5spk_virtual_channel, 0);
  EX_MUTE_ON = true;
}

void EX_muteOff()
{
  M5.Speaker.setVolume(EX_VOLUME);
  M5.Speaker.setChannelVolume(m5spk_virtual_channel, EX_VOLUME);
  EX_MUTE_ON = false;
}

// ------ Ver1.02 ---------------------------------------------------------------
void EX_handle_randomSpeak()
{
  // 独り言モードの開始・停止 -- random speak
  String message = "randomSpeak : ";
  String arg_str = server.arg("mode");

  if (arg_str == "on")
  {
    if (!EX_RANDOM_SPEAK_STATE)
    {
      EX_timerStop2();
      EX_randomSpeak(true);
      message += "mode=on";
    }
  }
  else if (arg_str == "off")
  {
    if (EX_RANDOM_SPEAK_STATE)
    {
      EX_randomSpeak(false);
      message += "mode=off";
    }
  }
  else
  {
    message += "invalid argument";
  }
  Serial.println(message);
  EX_tone(2);
  server.send(200, "text/plain", String("OK"));
}

void EX_randomSpeak(bool mode)
{
  String tmp;

  if (mode)
  {
    if (EX_isJP())
      tmp = "独り言始めます。";
    else
      tmp = "Talk to myself.";

    LASTMS1 = millis();
    RANDOM_TIME = 40000 + 1000 * random(30);
    EX_RANDOM_SPEAK_STATE = true;
  }
  else
  {
    if (EX_isJP())
      tmp = "独り言やめます。";
    else
      tmp = "Stop talking to myself.";

    RANDOM_TIME = -1;
    EX_RANDOM_SPEAK_STATE = false;
  }

  // random_speak = !random_speak;
  avatar.setExpression(Expression::Happy);
  EX_ttsDo((char *)tmp.c_str(), tts_parms2);
  avatar.setExpression(Expression::Neutral);
  Serial.println("mp3 begin");

  EX_RANDOM_SPEAK_ON_GET = false;
  // EX_RANDOM_SPEAK_OFF_GET = false;
}

// ------ Ver1.01 ---------------------------------------------------------------
void EX_handle_timer()
{
  // timerの時間を設定
  EX_tone(2);

  String timer_str = server.arg("setTime");
  if (timer_str != "")
  {
    // timer_str = timer_str + "\n";
    EX_TIMER_SEC = timer_str.toInt();
  }
  else
  {
    // timer_str = "default_time\n";
    timer_str = "default_time";
    EX_TIMER_SEC = EX_TIMER_INIT;
  }

  EX_TIMER_STOP_GET = false;
  EX_TIMER_GO_GET = false;

  String message = "Timer:SetTIme = " + timer_str;
  Serial.println(message);
  server.send(200, "text/plain", String("OK"));
}

void EX_handle_timerGo()
{
  // timerの時間を設定し、すぐに 開始。
  String timer_str = server.arg("setTime");
  if (timer_str != "")
  {
    // timer_str = timer_str + "\n";
    EX_TIMER_SEC = timer_str.toInt();
  }
  else
  {
    // timer_str = "setup_time\n";
    timer_str = "setup_time";
  }

  String message = "TimerGO:SetTime";
  message += " = " + timer_str;

  if (!EX_TIMER_STARTED)
  {
    if (EX_SYSINFO_DISP)
      EX_sysInfoDispEnd();

    EX_randomSpeakStop2();
    EX_timerStart();
    // EX_TIMER_STOP_GET = false;
    // EX_TIMER_GO_GET = true;
  }
  else
  {
    message += ": is ignore !";
  }

  Serial.println(message);
  EX_tone(2);
  server.send(200, "text/plain", String("OK"));
}

void EX_handle_timerStop()
{
  // timerの停止
  EX_tone(2);
  String message = "TimerSTOP";

  if (EX_TIMER_STARTED)
  {
    EX_TIMER_STOP_GET = true;
    EX_TIMER_GO_GET = false;
    message += " will be done ";
  }
  else
  {
    message += " is ignore !";
  }
  Serial.println(message);
  server.send(200, "text/plain", String("OK"));
}

void EX_handle_selfIntro()
{
  // 自己紹介 -- Speak self-introduction
  EX_tone(2);
  String message = "speakSelfIntro";
  // EX_SELF_INTRO_GET = true;
  Serial.println(message);

  avatar.setExpression(Expression::Happy);
  char text1[] = "みなさんこんにちは、私の名前はスタックチャンです、よろしくね。";
  EX_ttsDo(text1, tts_parms2);
  avatar.setExpression(Expression::Neutral);
  Serial.println("mp3 begin");
  // EX_SELF_INTRO_GET = false;

  server.send(200, "text/plain", String("OK"));
}

void EX_timerStart()
{
  // ---- Timer 開始 ----------------
  if ((EX_TIMER_SEC < EX_TIMER_MIN) || (EX_TIMER_SEC > EX_TIMER_MAX))
  {
    Serial.println(EX_TIMER_SEC, DEC);
    return;
  }

  // Timer Start Go ----
  EX_TIMER_START_MILLIS = millis();

  for (int i = 0; i < NUM_LEDS; i++)
  {
    EX_setColorLED2(i, EX_ColorLED3(0, 0, 0));
  }

  EX_showLED();
  EX_setColorLED2(2, EX_ColorLED3(0, 0, 255));
  EX_setColorLED2(7, EX_ColorLED3(0, 0, 255));

  int timer_min = EX_TIMER_SEC / 60;
  int timer_sec = EX_TIMER_SEC % 60;
  char timer_min_str[10] = "";
  char timer_sec_str[10] = "";
  char timer_msg_str[100] = "";

  if (timer_min >= 1)
  {
    sprintf(timer_min_str, "%d分", timer_min);
  }
  if (timer_sec >= 1)
  {
    sprintf(timer_sec_str, "%d秒", timer_sec);
  }

  char EX_TmrSTART_TXT[] = "の、タイマーを開始します。";
  sprintf(timer_msg_str, "%s%s%s", timer_min_str, timer_sec_str, EX_TmrSTART_TXT);
  Serial.println(timer_msg_str);
  EX_ttsDo(timer_msg_str, tts_parms2);
  EX_showLED();

  delay(3000); // 3秒待機
  EX_TIMER_STARTED = true;
  EX_TIMER_GO_GET = false;
  EX_TIMER_STOP_GET = false;
}

void EX_timerStop()
{
  // --- Timer を途中で停止 ------
  EX_TIMER_STARTED = false;
  EX_TIMER_GO_GET = false;
  EX_TIMER_STOP_GET = false;
  EX_TIMER_ELEAPSE_SEC = 0;

  for (int i = 0; i < NUM_LEDS; i++)
  {
    EX_setColorLED2(i, EX_ColorLED3(0, 0, 0));
  }
  EX_showLED();

  EX_setColorLED2(2, EX_ColorLED3(255, 0, 0));
  EX_setColorLED2(7, EX_ColorLED3(255, 0, 0));

  char EX_TmrSTOP_TXT[] = "タイマーを停止します。";
  EX_ttsDo(EX_TmrSTOP_TXT, tts_parms2);
  EX_showLED();
  delay(2000); // 2秒待機

  // 全てのLEDを消灯
  for (int i = 0; i < NUM_LEDS; i++)
  {
    EX_setColorLED2(i, EX_ColorLED3(0, 0, 0));
  }
  EX_showLED();
  delay(500); // 0.5秒待機
}

void EX_timerStarted()
{
  // timer開始後の途中経過の処理

  // 0.5秒ごとにLEDを更新する処理を追加
  int phase = (EX_TIMER_ELEAPSE_SEC / 5) % 2; // 往復の方向を決定
  int pos = EX_TIMER_ELEAPSE_SEC % 5;
  int ledIndex1, ledIndex2;

  if (phase == 0)
  { // 前進
    ledIndex1 = pos;
    ledIndex2 = NUM_LEDS - 1 - pos;
  }
  else
  { // 後退
    ledIndex1 = 4 - pos;
    ledIndex2 = 5 + pos;
  }

  EX_clearLED();                         // すべてのLEDを消す
  EX_setColorLED4(ledIndex1, 0, 0, 255); // 現在のLEDを青色で点灯
  EX_setColorLED4(ledIndex2, 0, 0, 255); // 現在のLEDを青色で点灯
  EX_showLED();                          // LEDの状態を更新

  // 10秒間隔で読み上げ
  if ((EX_TIMER_ELEAPSE_SEC % 10 == 0) && (EX_TIMER_ELEAPSE_SEC < EX_TIMER_SEC))
  {
    char buffer[64];
    if (EX_TIMER_ELEAPSE_SEC < 60)
    {
      sprintf(buffer, "%d秒。", EX_TIMER_ELEAPSE_SEC);
    }
    else
    {
      int minutes = EX_TIMER_ELEAPSE_SEC / 60;
      int seconds = EX_TIMER_ELEAPSE_SEC % 60;
      if (seconds != 0)
      {
        sprintf(buffer, "%d分%d秒。", minutes, seconds);
      }
      else
      {
        sprintf(buffer, "%d分経過。", minutes);
      }
    }
    avatar.setExpression(Expression::Happy);
    EX_ttsDo(buffer, tts_parms6);
    avatar.setExpression(Expression::Neutral);
  }
}

void EX_timerEnd()
{
  // 指定時間が経過したら終了
  // 全てのLEDを消す処理を追加
  for (int i = 0; i < NUM_LEDS; i++)
  {
    EX_setColorLED2(i, EX_ColorLED3(0, 0, 0));
  }
  EX_showLED();
  EX_setColorLED2(2, EX_ColorLED3(0, 255, 0));
  EX_setColorLED2(7, EX_ColorLED3(0, 255, 0));
  EX_showLED();

  avatar.setExpression(Expression::Happy);
  char EX_TmrEND_TXT[] = "設定時間になりました。";
  EX_ttsDo(EX_TmrEND_TXT, tts_parms2);
  avatar.setExpression(Expression::Neutral);

  // 全てのLEDを消す処理を追加
  for (int i = 0; i < NUM_LEDS; i++)
  {
    EX_setColorLED4(i, 0, 0, 0);
  }
  EX_showLED(); // LEDの状態を更新

  // カウントダウンをリセット
  EX_TIMER_STARTED = false;
  EX_TIMER_GO_GET = false;
  EX_TIMER_STOP_GET = false;
  // elapsedMinutes = 0;
  EX_TIMER_ELEAPSE_SEC = 0;
}
#endif
//------------------- < end of USE_EXTEND > --------------------------------------

bool init_chat_doc(const char *data)
{
  DeserializationError error = deserializeJson(CHAT_DOC, data);
  if (error)
  {
    Serial.println("DeserializationError");
    return false;
  }
  String json_str;                         //= JSON.stringify(chat_doc);
  serializeJsonPretty(CHAT_DOC, json_str); // 文字列をシリアルポートに出力する
  // Serial.println(json_str);
  return true;
}

// void handleRoot()
// {
//   server.send(200, "text/plain", "hello from m5stack!");
// }

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  //  server.send(404, "text/plain", message);
  server.send(404, "text/html", String(HEAD) + String("<body>") + message + String("</body>"));
}

void handle_speech()
{
  String message = server.arg("say");
  String expression = server.arg("expression");
  String speaker = server.arg("voice");
  int expr = 0;
  int parms_no = 1;
  Serial.println(expression);
  if (expression != "")
  {
    expr = expression.toInt();
    if (expr < 0)
      expr = 0;
    if (expr > 5)
      expr = 5;
  }

  if (speaker != "")
  {
    if (EX_TTS_TYPE == 0)
    { // voiceText
      parms_no = speaker.toInt();
      if (parms_no < 0)
        parms_no = 0;
      if (parms_no > 4)
        parms_no = 4;
    }

    if (EX_TTS_TYPE == 2)
    { // voicevox
      TTS2_PARMS = TTS2_SPEAKER + speaker;
    }
  }

  //  message = message + "\n";
  Serial.println(message);
  ////////////////////////////////////////
  // 音声の発声
  ////////////////////////////////////////
  avatar.setExpression(expressions_table[expr]);
  EX_ttsDo((char *)message.c_str(), tts_parms_table[parms_no]);

  server.send(200, "text/plain", String("OK"));
}

String https_post_json(const char *url, const char *json_string, const char *root_ca)
{
  EX_WK_ERROR_NO = 0;
  EX_WK_ERROR_CODE = 0;

  String payload = "";
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client)
  {
    client->setCACert(root_ca);
    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is
      HTTPClient https;

      // *** [わかりません対策01] ***
      https.setTimeout(UINT16_MAX); // 最大値の約65秒にタイムアウトを設定

      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, url))
      { // HTTPS
        Serial.print("[HTTPS] POST...\n");
        // start connection and send HTTP header
        https.addHeader("Content-Type", "application/json");
        https.addHeader("Authorization", String("Bearer ") + OPENAI_API_KEY);
        int httpCode = https.POST((uint8_t *)json_string, strlen(json_string));

        EX_WK_ERROR_CODE = httpCode;
        Serial.print(" httpCode = ");
        Serial.println(httpCode, DEC);

        // httpCode will be negative on error
        if (httpCode > 0)
        {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
          {
            payload = https.getString();
            Serial.println("//////////////");
            Serial.println(payload);
            Serial.println("//////////////");

            if (payload == "")
            {
              Serial.println("CODE_OK or CODE_MOVED_PERMANENTLY and payload is void ");
              EX_WK_ERROR_NO = 1;
            }
          }
          else
          {
            Serial.println("httpCode other error code number get ");
            // 「わかりません」問題で、SPIFFS内の"/data.json"ファイルが正常でない場合は、ここのパスを通る。
            EX_WK_ERROR_NO = 2;
          }
        }
        else
        {
          Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
          EX_WK_ERROR_NO = 3;
        }
        https.end();
      }
      else
      {
        Serial.printf("[HTTPS] Unable to connect\n");
        EX_WK_ERROR_NO = 4;
      }
      // End extra scoping block
    }
    delete client;
  }
  else
  {
    Serial.println("Unable to create client");
    EX_WK_ERROR_NO = 5;
  }
  return payload;
}

#define EX_DOC_SIZE 1024 * 2
String chatGpt(String json_string)
{
  String response = "";
  avatar.setExpression(Expression::Doubt);

  if (EX_isJP())
    avatar.setSpeechText("考え中…");
  else
    avatar.setSpeechText("I'm thinking...");

  // LED 3番と7番を黄色に光らせる
  EX_setColorLED4(2, 255, 255, 255); // 白色
  EX_setColorLED4(7, 255, 255, 255); // 白色
  EX_showLED();

  String ret = https_post_json("https://api.openai.com/v1/chat/completions", json_string.c_str(), root_ca_openai);
  avatar.setExpression(Expression::Neutral);
  avatar.setSpeechText("");
  Serial.println(ret);

  // 音声が再生された後にLEDを消灯
  EX_setColorLED4(2, 0, 0, 0); // 黒（消灯）
  EX_setColorLED4(7, 0, 0, 0); // 黒（消灯）
  EX_showLED();

  if (ret != "")
  {
    EX_WK_CNT = 0;
    // DynamicJsonDocument doc(2000);
    DynamicJsonDocument doc(EX_DOC_SIZE);
    DeserializationError error = deserializeJson(doc, ret.c_str());

    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      avatar.setExpression(Expression::Sad);

      if (EX_isJP())
      {
        avatar.setSpeechText("エラーです");
        response = "エラーです";
      }
      else
      {
        avatar.setSpeechText("Error.");
        response = "Error.";
      }

      delay(1000);
      avatar.setSpeechText("");
      avatar.setExpression(Expression::Neutral);
    }
    else
    {
      const char *data = doc["choices"][0]["message"]["content"];
      Serial.println(data);
      response = String(data);
      std::replace(response.begin(), response.end(), '\n', ' ');
    }
  }
  else
  {
    // 音声が再生された後にLEDを消灯
    EX_setColorLED4(2, 0, 0, 0); // 黒（消灯）
    EX_setColorLED4(7, 0, 0, 0); // 黒（消灯）
    EX_showLED();

    // ---「わかりません」エラー番号とコード情報の発声 ---
    char msg1[200];

    if (EX_isJP())
    {
      if (EX_WK_ERROR_NO != 0)
      {
        if (EX_WK_ERROR_CODE < 0)
          sprintf(msg1, "わかりません、番号 %d、コード・マイナス %d です。", EX_WK_ERROR_NO, abs(EX_WK_ERROR_CODE));
        else
          sprintf(msg1, "わかりません、番号 %d、コード %d です。", EX_WK_ERROR_NO, abs(EX_WK_ERROR_CODE));
      }
      else
      {
        sprintf(msg1, "わかりません、番号 %d です。", EX_WK_ERROR_NO);
      }
    }
    else
    {
      if (EX_WK_ERROR_NO != 0)
      {
        if (EX_WK_ERROR_CODE < 0)
          sprintf(msg1, "I don't understand. number %d , code minus %d .", EX_WK_ERROR_NO, abs(EX_WK_ERROR_CODE));
        else
          sprintf(msg1, "I don't understand. number %d , code %d .", EX_WK_ERROR_NO, EX_WK_ERROR_CODE);
      }
      else
      {
        sprintf(msg1, "I don't understand. number %d .", EX_WK_ERROR_NO);
      }
    }

    EX_LAST_WK_ERROR_NO = EX_WK_ERROR_NO;
    EX_LAST_WK_ERROR_CODE = EX_WK_ERROR_CODE;
    EX_WK_ERROR_NO = 0;
    EX_WK_ERROR_CODE = 0;

    // --- 「わかりません」が指定回数続いたら、ロール設定を初期化する ---
    EX_WK_CNT++;
    Serial.print("EX_WK_CNT = ");
    Serial.println(EX_WK_CNT, DEC);
    char msg2[200] = "";
    if (EX_WK_CNT >= EX_WK_CNT_MAX)
    {
      EX_WK_CNT = 0;
      sprintf(msg2, "ロール設定を初期化。");
      init_chat_doc(EX_json_ChatString.c_str());
      // init_chat_doc(json_ChatString.c_str());
      INIT_BUFFER = "";
      serializeJson(CHAT_DOC, INIT_BUFFER);
      Role_JSON = INIT_BUFFER;
      save_json();
    }
    // ---------------------------------------------------------------

    char msg[200];
    sprintf(msg, "%s %s", msg1, msg2);
    avatar.setExpression(Expression::Sad);
    // avatar.setSpeechText("わかりません");
    // response = "わかりません";
    avatar.setSpeechText(msg);
    response = msg;
    Serial.println(msg);
    // delay(1000);
    delay(2000); // *** [わかりません対策01] ***
    avatar.setSpeechText("");
    avatar.setExpression(Expression::Neutral);

    // 音声が再生された後にLEDを消灯
    EX_setColorLED4(2, 0, 0, 0); // 黒（消灯）
    EX_setColorLED4(7, 0, 0, 0); // 黒（消灯）
    EX_showLED();
  }
  return response;
}

void handle_chat()
{
  static String response = "";
  String text = server.arg("text");

  String voice = server.arg("voice");
  if (voice != "")
  { // voiceText
    if (EX_TTS_TYPE == 1)
    {
      TTS_PARMS_NO = 1;
      TTS_PARMS_NO = voice.toInt();
      if (TTS_PARMS_NO < 0)
        TTS_PARMS_NO = 0;
      if (TTS_PARMS_NO > 4)
        TTS_PARMS_NO = 4;
    }

    if (EX_TTS_TYPE == 2)
    { // voicevox
      TTS2_PARMS = TTS2_SPEAKER + voice;
    }
  }

  Serial.println(INIT_BUFFER);
  init_chat_doc(INIT_BUFFER.c_str());

  // 質問をチャット履歴に追加
  chatHistory.push_back(text);
  // チャット履歴が最大数を超えた場合、古い質問と回答を削除
  if (chatHistory.size() > MAX_HISTORY * 2)
  {
    chatHistory.pop_front();
    chatHistory.pop_front();
  }

  for (int i = 0; i < chatHistory.size(); i++)
  {
    JsonArray messages = CHAT_DOC["messages"];
    // messages = chat_doc["messages"];

    JsonObject systemMessage1 = messages.createNestedObject();
    if (i % 2 == 0)
    {
      systemMessage1["role"] = "user";
    }
    else
    {
      systemMessage1["role"] = "assistant";
    }
    systemMessage1["content"] = chatHistory[i];
  }

  String json_string;
  serializeJson(CHAT_DOC, json_string);
  if (SPEECH_TEXT == "" && SPEECH_TEXT_BUFFER == "")
  {
    response = chatGpt(json_string);
    SPEECH_TEXT = response;
    // 返答をチャット履歴に追加
    chatHistory.push_back(response);
  }
  else
  {
    response = "busy";
  }

  serializeJsonPretty(CHAT_DOC, json_string);
  Serial.println("====================");
  Serial.println(json_string);
  Serial.println("====================");
  server.send(200, "text/html", String(HEAD) + String("<body>") + response + String("</body>"));
}

void exec_chatGPT(String text)
{
  static String response = "";
  Serial.println(INIT_BUFFER);
  init_chat_doc(INIT_BUFFER.c_str());
  // 質問をチャット履歴に追加
  chatHistory.push_back(text);
  // チャット履歴が最大数を超えた場合、古い質問と回答を削除
  if (chatHistory.size() > MAX_HISTORY * 2)
  {
    chatHistory.pop_front();
    chatHistory.pop_front();
  }

  for (int i = 0; i < chatHistory.size(); i++)
  {
    JsonArray messages = CHAT_DOC["messages"];
    JsonObject systemMessage1 = messages.createNestedObject();
    if (i % 2 == 0)
    {
      systemMessage1["role"] = "user";
    }
    else
    {
      systemMessage1["role"] = "assistant";
    }
    systemMessage1["content"] = chatHistory[i];
  }

  String json_string;
  serializeJson(CHAT_DOC, json_string);
  if (SPEECH_TEXT == "" && SPEECH_TEXT_BUFFER == "")
  {
    response = chatGpt(json_string);
    SPEECH_TEXT = response;
    // 返答をチャット履歴に追加
    chatHistory.push_back(response);
  }
  else
  {
    response = "busy";
  }
  // Serial.printf("chatHistory.max_size %d \n",chatHistory.max_size());
  // Serial.printf("chatHistory.size %d \n",chatHistory.size());
  // for (int i = 0; i < chatHistory.size(); i++)
  // {
  //   Serial.print(i);
  //   Serial.println("= "+chatHistory[i]);
  // }
  serializeJsonPretty(CHAT_DOC, json_string);
  Serial.println("====================");
  Serial.println(json_string);
  Serial.println("====================");
}

void handle_apikey()
{
  // ファイルを読み込み、クライアントに送信する
  EX_tone(2);
  server.send(200, "text/html", APIKEY_HTML);
}

void handle_apikey_set()
{
  EX_tone(2);
  // POST以外は拒否
  if (server.method() != HTTP_POST)
  {
    return;
  }

  // openai
  String openai = server.arg("openai");
  // voicetxt
  String voicevox = server.arg("voicevox");
  // voicetxt
  String voicetext = server.arg("voicetext");

  OPENAI_API_KEY = openai;
  VOICEVOX_API_KEY = voicevox;
  EX_VOICETEXT_API_KEY = voicetext;
  Serial.println(openai);
  Serial.println(voicevox);
  Serial.println(voicetext);

  uint32_t nvs_handle;
  if (ESP_OK == nvs_open("apikey", NVS_READWRITE, &nvs_handle))
  {
    nvs_set_str(nvs_handle, "openai", openai.c_str());
    nvs_set_str(nvs_handle, "voicevox", voicevox.c_str());
    nvs_set_str(nvs_handle, "voicetext", voicetext.c_str());
    nvs_close(nvs_handle);
  }
  server.send(200, "text/plain", String("OK"));
}

void handle_role()
{
  // ファイルを読み込み、クライアントに送信する
  EX_tone(2);
  server.send(200, "text/html", ROLE_HTML);
}

bool save_json()
{
  // SPIFFSをマウントする
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return false;
  }

  // JSONファイルを作成または開く
  File fl_SPIFFS = SPIFFS.open("/data.json", "w");
  if (!fl_SPIFFS)
  {
    Serial.println("Failed to open file for writing");
    return false;
  }

  // JSONデータをシリアル化して書き込む
  serializeJson(CHAT_DOC, fl_SPIFFS);
  fl_SPIFFS.close();
  return true;
}

void handle_role_set()
{
  // POST以外は拒否
  if (server.method() != HTTP_POST)
  {
    return;
  }
  String role = server.arg("plain");
  if (role != "")
  {
    init_chat_doc(INIT_BUFFER.c_str());
    JsonArray messages = CHAT_DOC["messages"];
    JsonObject systemMessage1 = messages.createNestedObject();
    systemMessage1["role"] = "system";
    systemMessage1["content"] = role;
    //    serializeJson(chat_doc, InitBuffer);
  }
  else
  {
    init_chat_doc(json_ChatString.c_str());
  }
  // 会話履歴をクリア
  chatHistory.clear();

  INIT_BUFFER = "";
  serializeJson(CHAT_DOC, INIT_BUFFER);
  Serial.println("INIT_BUFFER = " + INIT_BUFFER);
  Role_JSON = INIT_BUFFER;

  // JSONデータをspiffsへ出力する
  save_json();

  // 整形したJSONデータを出力するHTMLデータを作成する
  String html = "<html><body><pre>";
  serializeJsonPretty(CHAT_DOC, html);
  html += "</pre></body></html>";

  // HTMLデータをシリアルに出力する
  Serial.println(html);
  server.send(200, "text/html", html);
}

// 整形したJSONデータを出力するHTMLデータを作成する
void handle_role_get()
{
  String html = "<html><body><pre>";
  serializeJsonPretty(CHAT_DOC, html);
  html += "</pre></body></html>";

  // HTMLデータをシリアルに出力する
  Serial.println(html);
  EX_tone(2);
  server.send(200, "text/html", String(HEAD) + html);
};

void handle_face()
{
  EX_tone(2);
  String expression = server.arg("expression");
  expression = expression + "\n";
  Serial.println(expression);
  switch (expression.toInt())
  {
  case 0:
    avatar.setExpression(Expression::Neutral);
    break;
  case 1:
    avatar.setExpression(Expression::Happy);
    break;
  case 2:
    avatar.setExpression(Expression::Sleepy);
    break;
  case 3:
    avatar.setExpression(Expression::Doubt);
    break;
  case 4:
    avatar.setExpression(Expression::Sad);
    break;
  case 5:
    avatar.setExpression(Expression::Angry);
    break;
  }
  server.send(200, "text/plain", String("OK"));
}

// /// set M5Speaker virtual channel (0-7)
// // static constexpr uint8_t m5spk_virtual_channel = 0;
// static AudioOutputM5Speaker out(&M5.Speaker, m5spk_virtual_channel);
// AudioGeneratorMP3 *mp3;

// // for TTS00 --VoiceText(HOYA)
// AudioFileSourceVoiceTextStream *file_TTS00 = nullptr;
// AudioFileSourceBuffer *buff_TTS00 = nullptr;
// // for TTS01 -- GoogleTTS
// AudioFileSourcePROGMEM *file_TTS01 = nullptr;
// const int mp3buffSize = 50 * 1024;
// uint8_t mp3buff[mp3buffSize];

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void)isUnicode; // Punt this ball for now
  // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1) - 1] = 0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2) - 1] = 0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  Serial.flush();
}

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  // Note that the string may be in PROGMEM, so copy it to RAM for printf
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1) - 1] = 0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}

void lipSync(void *args)
{
  float gazeX, gazeY;
  int level = 0;
  DriveContext *ctx = (DriveContext *)args;
  Avatar *avatar = ctx->getAvatar();
  for (;;)
  {
    level = abs(*out.getBuffer());
    if (level < 100)
      level = 0;
    if (level > 15000)
    {
      level = 15000;
    }
    float open = (float)level / 15000.0;
    avatar->setMouthOpenRatio(open);
    avatar->getGaze(&gazeY, &gazeX);
    avatar->setRotation(gazeX * 5);
    delay(50);
  }
}

// void servo(void *args)
// {
//   float gazeX, gazeY;
//   DriveContext *ctx = (DriveContext *)args;
//   Avatar *avatar = ctx->getAvatar();
//   for (;;)
//   {
//     // #ifdef USE_SERVO
//     if (EX_SERVO_USE)
//     {
//       if (!SERVO_HOME)
//       {
//         avatar->getGaze(&gazeY, &gazeX);
//         servo_x.setEaseTo(SV_HOME_DEG_X + (int)(15.0 * gazeX));
//         if (gazeY < 0)
//         {
//           int tmp = (int)(10.0 * gazeY);
//           if (tmp > 10)
//             tmp = 10;
//           servo_y.setEaseTo(START_DEGREE_VALUE_Y + tmp);
//         }
//         else
//         {
//           servo_y.setEaseTo(START_DEGREE_VALUE_Y + (int)(10.0 * gazeY));
//         }
//       }
//       else
//       {
//         servo_x.setEaseTo(START_DEGREE_VALUE_X);
//         servo_y.setEaseTo(START_DEGREE_VALUE_Y);
//       }
//       synchronizeAllServosStartAndWaitForAllServosToStop();
//       // #endif
//     }

//     delay(50);
//   }
// }

void Servo_setup()
{
  if (SV_USE)
  {
    if (servo_x.attach(SERVO_PIN_X, SV_HOME_X, DEFAULT_MICROSECONDS_FOR_0_DEGREE, DEFAULT_MICROSECONDS_FOR_180_DEGREE))
    {
      Serial.print("Error attaching servo x");
    }
    if (servo_y.attach(SERVO_PIN_Y, SV_HOME_Y, DEFAULT_MICROSECONDS_FOR_0_DEGREE, DEFAULT_MICROSECONDS_FOR_180_DEGREE))
    {
      Serial.print("Error attaching servo y");
    }
    servo_x.setEasingType(EASE_QUADRATIC_IN_OUT);
    servo_y.setEasingType(EASE_QUADRATIC_IN_OUT);
    setSpeedForAllServos(30);

    sv_setX(SV_HOME_X);
    sv_setY(SV_HOME_Y);
    synchronizeAllServosStartAndWaitForAllServosToStop();
  }
}

// global -String separator_tbl[2][7] = {{"。", "？", "！", "、", "", "　", ""}, {":", ",", ".", "?", "!", "\n", ""}};
String separator_tbl[2][10] = {{"。", "？", "！", "、", "　", "♪", "\n", ""}, {":", ",", ".", "?", "!", "\n", ""}};

int search_separator(String text, int tbl)
{
  int i = 0;
  int dotIndex_min = 1000;
  int dotIndex;
  while (separator_tbl[tbl][i] != "")
  {
    dotIndex = text.indexOf(separator_tbl[tbl][i++]);
    if ((dotIndex != -1) && (dotIndex < dotIndex_min))
      dotIndex_min = dotIndex;
  }
  if (dotIndex_min == 1000)
    return -1;
  else
    return dotIndex_min;
}

// ------- 完全版 google_tts() ----------------------
void google_tts(char *text, char *lang)
{
  Serial.println("tts Start");
  String link = "http" + tts.getSpeechUrl(text, lang).substring(5);
  Serial.println(link);

  http.begin(client, link);
  http.setReuse(true);
  int code = http.GET();
  if (code != HTTP_CODE_OK)
  {
    http.end();
    //    cb.st(STATUS_HTTPFAIL, PSTR("Can't open HTTP request"));
    return;
  }
  WiFiClient *ttsclient = http.getStreamPtr();
  ttsclient->setTimeout(10000);
  if (ttsclient->available() > 0)
  {
    int i = 0;
    int len = sizeof(TTS1_mp3buff);
    int count = 0;

    bool data_end = false;
    while (!data_end)
    {
      if (ttsclient->available() > 0)
      {

        int bytesread = ttsclient->read(&TTS1_mp3buff[i], len);
        // Serial.printf("%d Bytes Read\n",bytesread);
        i = i + bytesread;
        if (i > sizeof(TTS1_mp3buff))
        {
          break;
        }
        else
        {
          len = len - bytesread;
          if (len <= 0)
            break;
        }
      }
      {
        Serial.printf(" %d Bytes Read\n", i);
        int lastms = millis();
        data_end = true;
        while (millis() - lastms < 600)
        { // データ終わりか待ってみる
          if (ttsclient->available() > 0)
          {
            data_end = false;
            break;
          }
          yield();
        }
      }
    }

    Serial.printf("Total %d Bytes Read\n", i);
    ttsclient->stop();
    http.end();

    file_TTS1 = new AudioFileSourcePROGMEM(TTS1_mp3buff, i);
    mp3->begin(file_TTS1, &out);
  }
}

void VoiceText_tts(char *text, char *tts_parms)
{
  file_TTS0 = new AudioFileSourceVoiceTextStream(text, tts_parms);
  BUFF = new AudioFileSourceBuffer(file_TTS0, TTS02mp3buff, TTS02mp3buffSize);
  mp3->begin(BUFF, &out);
}

// void info_spiffs(){
//   FSInfo fs_info;
//   SPIFFS.info(fs_info);
//   Serial.print("SPIFFS Total bytes: ");
//   Serial.println(fs_info.totalBytes);
//   Serial.print("SPIFFS Used bytes: ");
//   Serial.println(fs_info.usedBytes);
//   Serial.print("SPIFFS Free bytes: ");
//   Serial.println(fs_info.totalBytes - fs_info.usedBytes);
// }

void addPeriodBeforeKeyword(String &input, String keywords[], int numKeywords)
{ // keyword の前に「。」または、「.」を挿入する。
  // String keywords[] = {"(Neutral)", "(Happy)", "(Sleepy)", "(Doubt)", "(Sad)", "(Angry)"};

  int prevIndex = 0;
  for (int i = 0; i < numKeywords; i++)
  {
    int index = input.indexOf(keywords[i]);
    while (index != -1)
    {
      if (EX_isJP())
      {
        // *** [warning対策03] ***
        // if (index > 0 && input.charAt(index - 1) != '。')
        //  { input = input.substring(0, index) + "。" + input.substring(index); }
        // ---------------------------------------------------------------------
        if (index > 0)
        {
          String strLast = input.charAt(index - 1) + "";
          if (strLast != "。")
          {
            input = input.substring(0, index) + "。" + input.substring(index);
          }
        }
      }
      else
      {
        if (index > 0 && input.charAt(index - 1) != '.')
        {
          input = input.substring(0, index) + "." + input.substring(index);
        }
      }
      prevIndex = index + keywords[i].length() + 1; // update prevIndex to after the keyword and period
      index = input.indexOf(keywords[i], prevIndex);
    }
  }
  //  Serial.println(input);
}

// ----------------------------------------------------------------------------------
// sentence に　(Happy)などの感情表現が含まれているとその位置を expressionIndx　で返す。
// 　見つからなかったら、 expressionIndx = -1 を返す
void getExpression(String &sentence, int &expressionIndx)
{
  Serial.println("sentence=" + sentence);
  int startIndex = sentence.indexOf("(");
  if (startIndex >= 0)
  {
    int endIndex = sentence.indexOf(")", startIndex);
    if (endIndex > 0)
    {
      String extractedString = sentence.substring(startIndex + 1, endIndex); // 括弧を含まない部分文字列を抽出
                                                                             //        Serial.println("extractedString="+extractedString);
      sentence.remove(startIndex, endIndex - startIndex + 1);                // 括弧を含む部分文字列を削除
                                                                             //        Serial.println("sentence="+sentence);
      if (extractedString != "")
      {
        expressionIndx = 0;
        while (1)
        {
          if (EXPRESSION_STRING[expressionIndx] == extractedString)
          {
            avatar.setExpression(expressions_table[expressionIndx]);
            break;
          }
          if (EXPRESSION_STRING[expressionIndx] == "")
          {
            expressionIndx = -1;
            break;
          }
          expressionIndx++;
        }
      }
      else
      {
        expressionIndx = -1;
      }
    }
  }
}

// ------------------------ < start of setup() > -----------------------
void setup()
{
  // ********** M5 config **************
  auto cfg = M5.config();
  cfg.external_spk = true; // use external speaker (SPK HAT / ATOMIC SPK)
  M5.begin(cfg);
  M5.Lcd.setTextSize(2);

  // *** TTS0,TTS2用 mp3Buffer確保 ******
  TTS02mp3buff = (uint8_t *)malloc(TTS02mp3buffSize);
  if (!TTS02mp3buff)
  {
    char msg[200];
    sprintf(msg, "FATAL ERROR:  Unable to preallocate %d bytes for app\n", TTS02mp3buffSize);
    EX_errStop(msg);
    // ****Stop ****
  }

  // ******* SPEAKER setup **************
  // {
  auto spk_cfg = M5.Speaker.config();
  spk_cfg.sample_rate = 96000;
  spk_cfg.task_pinned_core = APP_CPU_NUM;
  M5.Speaker.config(spk_cfg);
  // }
  M5.Speaker.begin();

  // *** startup.json SETUP begin ! *******
  EX_StartSetting();
  EX_LED_allOff();
  Servo_setup();

  // *** wifi setup **************
  bool success = EX_wifiConnect();
  if (!success)
  {
    EX_errReboot("wifi : cannot connected !!");
    // **** Reboot ****
  }
  EX_IP_ADDR = WiFi.localIP().toString();
  Serial.println("\n*** IP_ADDR and SSID ***");
  Serial.println(EX_IP_ADDR);
  Serial.println(EX_SSID);
  M5.Lcd.println("\nConnected");
  Serial.printf_P(PSTR("Go to http://"));
  M5.Lcd.print("Go to http://");
  Serial.println(WiFi.localIP());
  M5.Lcd.println(WiFi.localIP());
  // ----------------------------------------

  if (MDNS.begin("m5stack"))
  {
    Serial.println("MDNS responder started");
    M5.Lcd.println("MDNS responder started");
  }
  delay(1000);

  // **** SERVER SETUP *********
  server.on("/inline", []()
            { server.send(200, "text/plain", "this works as well"); });

  // And as regular external functions:
  server.on("/speech", handle_speech);
  server.on("/face", handle_face);
  server.on("/chat", handle_chat);
  server.on("/apikey", handle_apikey);
  server.on("/apikey_set", HTTP_POST, handle_apikey_set);
  server.on("/role", handle_role);
  server.on("/role_set", HTTP_POST, handle_role_set);
  server.on("/role_get", handle_role_get);

  // -- AiStackChanEx original handler ----
  // server.on("/test", EX_handle_test);
  server.on("/", EX_handleRoot);
  server.on("/servo", EX_handle_servo);
  server.on("/setting", EX_handle_setting);
  server.on("/startup", EX_handle_startup);
  server.on("/role1", EX_handle_role1);
  server.on("/role1_set", HTTP_POST, EX_handle_role1_set);
  server.on("/shutdown", EX_handle_shutdown);
  server.on("/wifiSelect", EX_handle_wifiSelect);
  server.on("/sysInfo", EX_handle_sysInfo);
  server.on("/timer", EX_handle_timer);
  server.on("/timerGo", EX_handle_timerGo);
  server.on("/timerStop", EX_handle_timerStop);
  server.on("/randomSpeak", EX_handle_randomSpeak);
  server.on("/speakSelfIntro", EX_handle_selfIntro);

  server.onNotFound(handleNotFound);

  // *****  chat_doc initialize  *****
  success = EX_chatDocInit();
  if (!success)
  {
    EX_errReboot("cannnot init chat_doc! ");
    // **** Reboot ****
  }
  server.begin();
  Serial.println("HTTP server started");
  M5.Lcd.println("HTTP server started");
  Serial.printf_P(PSTR("/ to control the chatGpt Server.\n"));
  M5.Lcd.print("/ to control the chatGpt Server.\n");
  delay(3000);
  // ----------------------------------

  audioLogger = &Serial;
  mp3 = new AudioGeneratorMP3();

#ifdef USE_DOGFACE
  static Face *face = new DogFace();
  static ColorPalette *cp = new ColorPalette();
  cp->set(COLOR_PRIMARY, TFT_BLACK); // AtaruFace
  cp->set(COLOR_SECONDARY, TFT_WHITE);
  cp->set(COLOR_BACKGROUND, TFT_WHITE);
  avatar.setFace(face);
  avatar.setColorPalette(*cp);
  avatar.init(8); // Color Depth8
#else
  avatar.init();
#endif
  avatar.addTask(lipSync, "lipSync");
  avatar.addTask(EX_servo, "servo");
  avatar.setSpeechFont(&fonts::efontJA_16);
  BOX_SERVO.setupBox(80, 120, 80, 80);
  // box_stt.setupBox(0, 0, M5.Display.width(), 60);
  // box_BtnA.setupBox(0, 100, 40, 60);
  // box_BtnC.setupBox(280, 100, 40, 60);
}
// ----------------------- < end of setup() > ----------------------------

// ----------- start of <loop> -------------------------
void loop()
{
  // **** RandomSpeak処理 ****
  if ((RANDOM_TIME >= 0) && (millis() - LASTMS1 > RANDOM_TIME))
    EX_RandomSpeakOperation();

  M5.update();

  // *** 顔タッチ: SERVO HOME/MOVE切替え ***
  if (!EX_KEYLOCK_STATE)
  {
#if defined(ARDUINO_M5STACK_Core2) || defined(ARDUINO_M5STACK_CORES3)
    auto count = M5.Touch.getCount();
    if (count)
      EX_ServoOperation();
#endif
  }

  // *** (BtnA) RandomSpeak Start/Stop ***
  if (M5.BtnA.wasPressed())
  {
    if ((!EX_KEYLOCK_STATE) && (!EX_SYSINFO_DISP))
    {
      EX_tone(1);
      if (!EX_RANDOM_SPEAK_STATE)
        EX_randomSpeak(true);
      else
        EX_randomSpeak(false);
    }
  }

  // ** (BtnB) Timer Start/Stop
  if (M5.BtnB.wasPressed())
  {
    if ((!EX_KEYLOCK_STATE) && (!EX_SYSINFO_DISP))
    {
      EX_tone(1);

      if (!EX_TIMER_STARTED)
      { // ---- Timer 開始 ------
        EX_randomSpeakStop2();
        EX_timerStart();
      }
      else
      { // --- Timer 停止 ------
        EX_timerStop();
      }
    }
  }

  // ** (BtnC) batteryLeve and SysInfoDisp
  if (M5.BtnC.wasPressed())
  {
    if (!EX_KEYLOCK_STATE)
    {
      EX_tone(1);
      if (!EX_SYSINFO_DISP)
      {
        EX_randomSpeakStop2();
        EX_timerStop2();
        EX_report_batt_level();
        EX_sysInfoDispStart(0);
      }
      else
      {
        EX_sysInfoDispEnd();
      }
    }
  }

  EX_netCommand();

  // *** SPEECH_TEXT 処理 ****
  if (SPEECH_TEXT != "")
    EX_SpeechText1st();

  // ***** mp3 Running *****
  if (mp3->isRunning())
  { // -- mp3 loop終了 ------
    if (!mp3->loop())
      EX_SpeechTextNext();

    delay(1);
  }
  else
  { //*** mp3が走っていない時に実行 ***
    server.handleClient();
  }
}
// ---------------- end of <loop> ------------------

void EX_netCommand()
{
  if (EX_TIMER_STARTED)
  { // Timer 起動中
    if (EX_SYSINFO_DISP)
      EX_sysInfoDispEnd();

    uint32_t elapsedTimeMillis = millis() - EX_TIMER_START_MILLIS;
    uint16_t currentElapsedSeconds = elapsedTimeMillis / 1000;

    if (currentElapsedSeconds >= EX_TIMER_SEC)
    { // 指定時間が経過したら終了
      EX_timerEnd();
    }
    else if (EX_TIMER_STOP_GET)
    { // ---Timer停止---
      EX_timerStop();
    }
    else if (currentElapsedSeconds != EX_TIMER_ELEAPSE_SEC)
    { // --- Timer途中経過の処理------
      EX_TIMER_ELEAPSE_SEC = currentElapsedSeconds;
      EX_timerStarted();
    }
  }

  if (EX_RANDOM_SPEAK_ON_GET)
  {
    if (EX_SYSINFO_DISP)
      EX_sysInfoDispEnd();

    EX_randomSpeak(true);
  }
}