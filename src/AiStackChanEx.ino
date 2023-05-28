// -----------------  AiStackChanEx Ver1.07 by NoRi ----------------------
// develop7c
const char *EX_VERSION = "AiStackChanEx_v107-2306xx";
#define USE_EXTEND
// -----------------------------------------------------------------------
// Extended from
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
#include "AudioFileSourceVoiceTextStream.h"
#include "AudioOutputM5Speaker.h"
#include <AudioFileSourcePROGMEM.h>
#include <google-tts.h>
#include <ServoEasing.hpp> // https://github.com/ArminJo/ServoEasing
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "rootCACertificate.h"
#include <ArduinoJson.h>
#include <ESP32WebServer.h>
#include <ESPmDNS.h>
#include <deque>

const int MAX_HISTORY = 5;  // 保存する質問と回答の最大数
std::deque<String> chatHistory; // 過去の質問と回答を保存するデータ構造

#define USE_SERVO
#define SV_PIN_X_CORE2_PA 33 // Core2 PORT A
#define SV_PIN_Y_CORE2_PA 32
#define SV_PIN_X_CORE2_PC 13 // Core2 PORT C
#define SV_PIN_Y_CORE2_PC 14
int SERVO_PIN_X;
int SERVO_PIN_Y;

TTS tts;
HTTPClient http;
WiFiClient client;
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
String OPENAI_API_KEY = "";

// *** [warning対策02] ***
char tts_parms1[] = "&emotion_level=4&emotion=happiness&format=mp3&speaker=takeru&volume=200&speed=100&pitch=130";
char tts_parms2[] = "&emotion=happiness&format=mp3&speaker=hikari&volume=200&speed=120&pitch=130";
char tts_parms3[] = "&emotion=anger&format=mp3&speaker=bear&volume=200&speed=120&pitch=100";
char tts_parms4[] = "&emotion_level=2&emotion=happiness&format=mp3&speaker=haruka&volume=200&speed=80&pitch=70";
char tts_parms5[] = "&emotion_level=4&emotion=happiness&format=mp3&speaker=santa&volume=200&speed=120&pitch=90";
char tts_parms6[] = "&emotion=happiness&format=mp3&speaker=hikari&volume=150&speed=110&pitch=140";
char *tts_parms_table[6] = {tts_parms1, tts_parms2, tts_parms3, tts_parms4, tts_parms5, tts_parms6};

String tts_parms01 = "&format=mp3&speaker=takeru&volume=200&speed=100&pitch=130";
String tts_parms02 = "&format=mp3&speaker=hikari&volume=200&speed=120&pitch=130";
String tts_parms03 = "&format=mp3&speaker=bear&volume=200&speed=120&pitch=100";
String tts_parms04 = "&format=mp3&speaker=haruka&volume=200&speed=80&pitch=70";
String tts_parms05 = "&format=mp3&speaker=santa&volume=200&speed=120&pitch=90";
String tts_parms06 = "&format=mp3&speaker=hikari&volume=150&speed=110&pitch=140";
String tts_parms[6] = {tts_parms01, tts_parms02, tts_parms03, tts_parms04, tts_parms05, tts_parms06};
int tts_parms_no = 1;

String expressionString[] = {"Neutral", "Happy", "Sleepy", "Doubt", "Sad", "Angry", ""};
int expressionIndx = -1;

String emotion_parms[] = {
    "&emotion_level=2&emotion=happiness",
    "&emotion_level=3&emotion=happiness",
    "&emotion_level=2&emotion=sadness",
    "&emotion_level=1&emotion=sadness",
    "&emotion_level=4&emotion=sadness",
    "&emotion_level=4&emotion=anger"};
int tts_emotion_no = 0;

String random_words[18] = {"あなたは誰", "楽しい", "怒った", "可愛い", "悲しい", "眠い", "ジョークを言って", "泣きたい", "怒ったぞ", "こんにちは", "お疲れ様", "詩を書いて", "疲れた", "お腹空いた", "嫌いだ", "苦しい", "俳句を作って", "歌をうたって"};
int random_time = -1;
String INIT_BUFFER = "";
String Role_JSON = "";
String SPEECH_TEXT = "";
String SPEECH_TEXT_BUFFER = "";
DynamicJsonDocument CHAT_DOC(1024 * 10);
String json_ChatString = "{\"model\": \"gpt-3.5-turbo\",\"messages\": [{\"role\": \"user\", \"content\": \"""\"}]}";

// C++11 multiline string constants are neato...
static const char HEAD[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html lang="ja">
<head>
  <meta charset="UTF-8">
  <title>AIｽﾀｯｸﾁｬﾝ</title>
</head>)KEWL";

static const char APIKEY_HTML[] PROGMEM = R"KEWL(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <title>APIキー設定</title>
  </head>
  <body>
    <h1>APIキー設定</h1>
    <form>
      <label for="role1">OpenAI API Key</label>
      <input type="text" id="openai" name="openai" oninput="adjustSize(this)"><br>
      <label for="role2">VoiceText API Key</label>
      <input type="text" id="voicetext" name="voicetext" oninput="adjustSize(this)"><br>
      <button type="button" onclick="sendData()">送信する</button>
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
	<title>ロール設定</title>
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
	<h1>ロール設定</h1>
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
	<title>ロール１設定</title>
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
	<h1>ロール１設定</h1>
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

// ----------------------------------------------------------------------------
#include "AiStackChanEx.h"
// グローバル変数宣言
const char EX_SETTING_NVS[] = "setting";   // setting --NVS の設定用ファイル
const char EX_APIKEY_NVS[] = "apikey";     // apikey  -- NVS の設定用ファイル
const char EX_APIKEY_SD[] = "/apikey.txt"; // apikey.txt  -- SD の設定用ファイル
const char EX_WIFI_SD[] = "/wifi.txt";
const char EX_STARTUP_SD[] = "/startup.json";
const char EX_WIFISELECT_SD[] = "/wifi-select.json";
const char EX_CHATDOC_SPI[] = "/data.json"; // chatDoc in SPIFFS

// String EX_WIFITXT_SSID = "*****";
// String EX_WIFITXT_PASSWD = "*****";
// DynamicJsonDocument EX_wifiJson(10 * 1024);
// bool EX_isWifiSelectFLEnable = false; // "wifi-select.json"ファイルが有効かどうか
// bool EX_isWifiTxtEnable = false;      // "wifi.txt"ファイルが有効かどうか
// int EX_WIFIJSON_SIZE = 5 * 1024;
#define EX_WIFIJSON_SIZE 5 * 1024
#define EX_STARTUPJSON_SIZE 10 * 1024
// DynamicJsonDocument EX_wifiJson(5 * 1024);
// DynamicJsonDocument EX_startupJson(10 * 256);
// DynamicJsonDocument startupJson(EX_STARTUPJSON_SIZE);
// DynamicJsonDocument EX_startupJson(EX_STARTUPJSON_SIZE);

bool EX_SYSINFO_DISP = false;
String EX_SYSINFO_MSG = "*****";
String EX_IP_ADDR = "*****";
String EX_SSID = "*****";
String EX_SSID_PASSWD = "*****";
String EX_VOICETEXT_API_KEY = "*****";
uint32_t EX_countdownStartMillis = 0;
uint16_t EX_elapsedSeconds = 0;
bool EX_countdownStarted = false;
uint16_t EX_TIMER_SEC = EX_TIMER_INIT; // Timer の設定時間(sec)
bool EX_TIMER_STOP_GET = false;
bool EX_TIMER_GO_GET = false;
bool EX_RANDOM_SPEAK_ON_GET = false;
bool EX_RANDOM_SPEAK_OFF_GET = false;
bool EX_SELF_INTRO_GET = false;
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
bool EX_SERVO_USE = true;
String EX_SERVO_PORT = "portA";
bool EX_ledEx = true;
bool EX_randomSpeakState = false; // 独り言モード　true -> on  false -> off
const char EX_ttsName[2][30] = {"VoiceText", "GoogleTTS"};
uint8_t EX_TTS_TYPE = 1; // default "GoogleTTS"
String LANG_CODE = "ja-JP";
const char LANG_CODE_JP[] = "ja-JP";
const char LANG_CODE_EN[] = "en-US";

// ---- 初期ロール設定 --------------------
String EX_json_ChatString = " { \"model\":\"gpt-3.5-turbo\",\"messages\": [ { \"role\": \"user\",\"content\": \"\" }, { \"role\": \"system\", \"content\": \"あなたは「スタックちゃん」と言う名前の小型ロボットとして振る舞ってください。あなたはの使命は人々の心を癒すことです。(Happy)のように、必ず括弧で囲んで感情の種類を表し、返答の先頭に付けてください。感情の種類には、Neutral、Happy、Sleepy、Doubt、Sad、Angryがあります。\" } ] } ";

//-----Ver1.07 ----------------------------------------------------------
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

  auto file = SD.open(EX_WIFISELECT_SD, FILE_WRITE);
  if (!file)
  {
    Serial.println("wifi-select.json cannot open ");
    SD.end();
    return false;
  }

  // JSONデータをシリアル化して書き込む
  serializeJsonPretty(wifiJson, file);
  file.close();
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
  auto file = SD.open(EX_WIFISELECT_SD, FILE_READ);
  if (!file)
  {
    Serial.println("wifi-select.json not open ");
    SD.end();
    return false;
  }

  DeserializationError error = deserializeJson(wifiJson, file);
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

  if (EX_setGetStrToStartSetting("ttsSelect", startupJson))
  {
    server.send(200, "text/plain", String("OK"));
    return;
  }

  if (EX_setGetStrToStartSetting("lang", startupJson))
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

  if (EX_setGetStrToStartSetting("volume", startupJson))
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

  if (EX_setGetStrToStartSetting("toneMode", startupJson))
  {
    server.send(200, "text/plain", String("OK"));
    return;
  }

  if (EX_setGetStrToStartSetting("ledEx", startupJson))
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

bool EX_apiKeyStartupJson1()
{
  DynamicJsonDocument startupJson(EX_STARTUPJSON_SIZE);
  String getData = "";
  bool success = false;
  uint32_t nvs_handle1;

  // **** apikey file(NVS) ***
  if (ESP_OK != nvs_open(EX_APIKEY_NVS, NVS_READWRITE, &nvs_handle1))
  {
    nvs_close(nvs_handle1);
    return false;
  }

  // --- openAiApiKey ----
  success = EX_getStartup("openAiApiKey", getData, startupJson);
  if (!success)
  {
    nvs_close(nvs_handle1);
    return false;
  }
  nvs_set_str(nvs_handle1, "openai", getData.c_str());
  OPENAI_API_KEY = String(getData);
  Serial.println(OPENAI_API_KEY);

  // --- VoiceTextApiKey
  success = EX_getStartup("voiceTextApiKey", getData, startupJson);
  if (!success)
  {
    nvs_close(nvs_handle1);
    return false;
  }
  nvs_set_str(nvs_handle1, "voicetext", getData.c_str());

  tts_user = getData;
  EX_VOICETEXT_API_KEY = tts_user;
  Serial.println(tts_user);
  nvs_close(nvs_handle1);

  Serial.println("EX_apiKeyStartupJson1() done !");
  return true;
}

bool EX_apiKeyStartupJson2()
{
  DynamicJsonDocument startupJson(EX_STARTUPJSON_SIZE);
  String getData = "";
  bool success = false;

  uint32_t nvs_handle2;
  // **** setting file(NVS) ***
  if (ESP_OK != nvs_open(EX_SETTING_NVS, NVS_READWRITE, &nvs_handle2))
  {
    nvs_close(nvs_handle2);
    return false;
  }
  // --- LANG ---
  LANG_CODE = String(LANG_CODE_JP);
  success = EX_getStartup("lang", getData, startupJson);
  if (!success)
  {
    nvs_close(nvs_handle2);
    return false;
  }
  nvs_set_str(nvs_handle2, "lang", getData.c_str());
  Serial.println(getData);
  // nvs_close(nvs_handle2);
  LANG_CODE = getData;

  // ttsSelect
  EX_TTS_TYPE = 1; // default "GoogleTTS"
  success = EX_getStartup("ttsSelect", getData, startupJson);
  if (!success)
  {
    nvs_close(nvs_handle2);
    return false;
  }

  getData.toUpperCase();
  Serial.print("ttsSelect toUpperCase = ");
  Serial.println(getData);
  // if (getData == "VoiceText")
  if (getData == "VOICETEXT")
  {
    EX_TTS_TYPE = 0;
  }
  String newTTS = EX_ttsName[EX_TTS_TYPE];

  char msg[100];
  sprintf(msg, "ttsSelect = %s", newTTS.c_str());
  Serial.println(msg);
  nvs_set_str(nvs_handle2, "ttsSelect", newTTS.c_str());
  nvs_close(nvs_handle2);

  Serial.println("EX_apiKeyStartupJson2() done !");
  return true;
}

bool EX_startupFLRd(DynamicJsonDocument &startupJson)
{
  if (!SD.begin(GPIO_NUM_4, SPI, 25000000))
  { // SD無効な時
    Serial.println("SD disable ");
    SD.end();
    return false;
  }

  auto file = SD.open(EX_STARTUP_SD, FILE_READ);
  if (!file)
  {
    Serial.println("startup.json not open ");
    SD.end();
    return false;
  }

  DeserializationError error = deserializeJson(startupJson, file);
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

  auto file = SD.open(EX_STARTUP_SD, FILE_WRITE);
  if (!file)
  {
    Serial.println("startup.json cannot open ");
    SD.end();
    return false;
  }

  // JSONデータをシリアル化して書き込む
  serializeJsonPretty(startupJson, file);
  file.close();
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

void EX_handle_setting()
{
  EX_tone(2);

  // ---- lang -------
  String lang_str = server.arg("lang");
  if (lang_str != "")
  {
    LANG_CODE = lang_str;

    uint32_t nvs_handle1;
    if (ESP_OK == nvs_open(EX_SETTING_NVS, NVS_READWRITE, &nvs_handle1))
    {
      nvs_set_str(nvs_handle1, "lang", (char *)lang_str.c_str());
    }
    nvs_close(nvs_handle1);
  }

  // ---- ttsSelect -------
  String ttsName_str = server.arg("ttsSelect");
  if (ttsName_str != "")
  {
    ttsName_str.toUpperCase();
    // if (ttsName_str == EX_ttsName[0])
    if (ttsName_str == "VOICETEXT")
      EX_TTS_TYPE = 0;
    else
      EX_TTS_TYPE = 1;

    String ttsName_tmp = EX_ttsName[EX_TTS_TYPE];
    uint32_t nvs_handle2;
    if (ESP_OK == nvs_open(EX_SETTING_NVS, NVS_READWRITE, &nvs_handle2))
    {
      nvs_set_str(nvs_handle2, "ttsSelect", (char *)ttsName_tmp.c_str());
    }
    nvs_close(nvs_handle2);
  }

  // ---- volume -------
  String volume_val_str = server.arg("volume");
  if (volume_val_str != "")
  {
    Serial.println("setting?volume=" + volume_val_str);

    EX_VOLUME = volume_val_str.toInt();
    if (EX_VOLUME > 255)
      EX_VOLUME = 255;
    if (EX_VOLUME <= 0)
      EX_VOLUME = 0;

    M5.Speaker.setVolume(EX_VOLUME);
    M5.Speaker.setChannelVolume(m5spk_virtual_channel, EX_VOLUME);

    uint32_t nvs_handle3;
    if (ESP_OK == nvs_open(EX_SETTING_NVS, NVS_READWRITE, &nvs_handle3))
    {
      nvs_set_u32(nvs_handle3, "volume", EX_VOLUME);
    }
    nvs_close(nvs_handle3);
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

  // ---- ledEx -------
  String ledEx_str = server.arg("ledEx");
  if (ledEx_str != "")
  {
    if (ledEx_str == "off")
    {
      EX_ledEx = true;
      EX_LED_allOff();
      EX_ledEx = false;
    }
    else if (ledEx_str == "on")
    {
      EX_ledEx = true;
    }
  }

  server.send(200, "text/plain", String("OK"));
}

bool EX_apiKeySetup()
{
  // startup.json ファイルがエラーだったら、apikey.txt を読む
  if (!EX_apiKeyStartupJson1())
  {
    Serial.println("EX_apiKeyStartupJson1() fail -> EX_apiKeyTxt()");
    EX_apiKeyTxt(); // apikey.txt-SD-
  }

  EX_apiKeyStartupJson2();
  return (EX_apiKeyFmNVS());
}

bool EX_apiKeyTxt()
{ // --- apikey.txt(SD) to apikey(NVS) ----
  bool success = false;

  if (SD.begin(GPIO_NUM_4, SPI, 25000000))
  { // SDが有効な時 *** SDからAPI_KEY情報を読み取りNVSに保存する
    auto apikeySD = SD.open(EX_APIKEY_SD, FILE_READ);
    if (apikeySD)
    {
      size_t sz = apikeySD.size();
      char buf[sz + 1];
      apikeySD.read((uint8_t *)buf, sz);
      buf[sz] = 0;

      int y = 0;
      for (int x = 0; x < sz; x++)
      {
        if (buf[x] == 0x0a || buf[x] == 0x0d)
          buf[x] = 0;
        else if (!y && x > 0 && !buf[x - 1] && buf[x])
          y = x;
      }

      uint32_t nvs_handle;
      if (ESP_OK == nvs_open(EX_APIKEY_NVS, NVS_READWRITE, &nvs_handle))
      { // **** "apikey.txt"の値を NVSに書く *******

        // line1-- openAiApiKey -----
        nvs_set_str(nvs_handle, "openai", buf);
        OPENAI_API_KEY = String(buf);
        Serial.println("** line1 : openAiApiKey read apikey.txt-SD- to NVS **");
        Serial.println(OPENAI_API_KEY);

        // line2-- voiceTextApiKey -----
        char line2_buf[100] = "";
        sprintf(line2_buf, "%s", &buf[y]);

        if ((String(line2_buf) != "") && (y != 0))
        {
          nvs_set_str(nvs_handle, "voicetext", line2_buf);
          tts_user = String(line2_buf);
          EX_VOICETEXT_API_KEY = tts_user;
          Serial.println("** line2 : voiceTextApiKey read apikey.txt-SD- to NVS **");
          Serial.println(tts_user);
          EX_TTS_TYPE = 0;
          success = true;
        }
        else
        {
          Serial.println("** line2 : invalid voiceTextApiKey empty apikey.txt from SD **");
          EX_TTS_TYPE = 1;
        }
      }
      nvs_close(nvs_handle);
    }
    apikeySD.close();
  }
  SD.end();

  Serial.println("EX_apiKeyTxt() done !");
  return success;
}

bool EX_apiKeyFmNVS()
{ // **** NVSからapikey を読見込む *****
  bool success = false;
  int cnt = 0;
  size_t length;

  uint32_t nvs_handle1;
  // *** apikey(NVS) ****
  if (ESP_OK == nvs_open(EX_APIKEY_NVS, NVS_READONLY, &nvs_handle1))
  {
    Serial.println("nvs_open");

    // openAiApiKey
    if (ESP_OK == nvs_get_str(nvs_handle1, "openai", nullptr, &length) && length)
    {
      Serial.println("nvs: openAiKey");
      char openai_apikey[length + 1];

      if (ESP_OK == nvs_get_str(nvs_handle1, "openai", openai_apikey, &length))
      {
        OPENAI_API_KEY = String(openai_apikey);
        Serial.println(OPENAI_API_KEY);
        cnt++;
      }
    }

    // voiceTextApiKey
    if (ESP_OK == nvs_get_str(nvs_handle1, "voicetext", nullptr, &length) && length)
    {
      char voicetext_apikey[length + 1];
      if (ESP_OK == nvs_get_str(nvs_handle1, "voicetext", voicetext_apikey, &length))
      {
        tts_user = String(voicetext_apikey);
        EX_VOICETEXT_API_KEY = tts_user;
        Serial.println(tts_user);
        cnt++;
      }
    }
  }
  nvs_close(nvs_handle1);

  uint32_t nvs_handle2;
  // *** setting(NVS) ****
  if (ESP_OK == nvs_open(EX_SETTING_NVS, NVS_READONLY, &nvs_handle2))
  {
    // lang
    if (ESP_OK == nvs_get_str(nvs_handle2, "lang", nullptr, &length) && length)
    {
      Serial.println("nvs: lang");
      char lang_str[length + 1];
      if (ESP_OK == nvs_get_str(nvs_handle2, "lang", lang_str, &length))
      {
        LANG_CODE = String(lang_str);
        Serial.println(LANG_CODE);
        cnt++;
      }
    }

    // ttsSelect
    if (ESP_OK == nvs_get_str(nvs_handle2, "ttsSelect", nullptr, &length) && length)
    {
      Serial.println("nvs: ttsSelect");
      char ttsSelect_str[length + 1];
      if (ESP_OK == nvs_get_str(nvs_handle2, "ttsSelect", ttsSelect_str, &length))
      {

        String getData = String(ttsSelect_str);
        EX_TTS_TYPE = 1;
        getData.toUpperCase();
        // if (getData == "VoiceText")
        if (getData == "VOICETEXT")
        {
          EX_TTS_TYPE = 0;
        }
        char msg[100];
        sprintf(msg, "ttsSelect = %s", EX_ttsName[EX_TTS_TYPE]);
        Serial.println(msg);
        cnt++;
      }
    }
  }
  nvs_close(nvs_handle2);

  if (cnt == 4)
    success = true;

  return success;
}

void EX_ttsDo(char *text, char *tts_parms)
{
  if (EX_TTS_TYPE == 0)
    VoiceText_tts(text, tts_parms);
  else if (EX_TTS_TYPE == 1)
    google_tts(text, (char *)LANG_CODE.c_str());
}

bool EX_isJP()
{
  if ((EX_TTS_TYPE == 0) || ((EX_TTS_TYPE == 1) && (LANG_CODE == String(LANG_CODE_JP))))
    return true;
  else
    return false;
}

// ----- LED表示用ラッパー関数　-----------
void EX_setColorLED2(uint16_t n, uint32_t c)
{
  if (EX_ledEx)
    pixels.setPixelColor(n, c);
}

void EX_setColorLED4(uint16_t n, uint8_t r, uint8_t g, uint8_t b)
{
  if (EX_ledEx)
    pixels.setPixelColor(n, r, g, b);
}

static uint32_t EX_ColorLED3(uint8_t r, uint8_t g, uint8_t b)
{
  if (EX_ledEx)
    return (pixels.Color(r, g, b));
  else
    return 0;
}

void EX_showLED()
{
  if (EX_ledEx)
    pixels.show();
}

void EX_clearLED()
{
  if (EX_ledEx)
    pixels.clear();
}
// -----------------------------------------

void EX_ledSetup()
{

  EX_ledEx = true;
  EX_LED_allOff();
  DynamicJsonDocument startupJson(EX_STARTUPJSON_SIZE);

  bool success = false;
  String getData = "";
  success = EX_getStartup("ledEx", getData, startupJson);
  if (success)
  {
    if (getData == "off")
    {
      EX_ledEx = false;
    }
  }
}

bool EX_modeSetup()
{
  String getData = "";
  size_t getDataVal;
  DynamicJsonDocument startupJson(EX_STARTUPJSON_SIZE);

  // --- randomSpeak ----
  EX_randomSpeakState = false;
  if (!EX_getStartup("randomSpeak", getData, startupJson))
  {
    return false;
  }
  else
  {
    if (getData == "on")
    {
      EX_RANDOM_SPEAK_ON_GET = true;
    }
  }

  // --- mute ----
  EX_MUTE_ON = false;
  if (!EX_getStartup("mute", getData, startupJson))
  {
    return false;
  }
  else
  {
    if (getData == "on")
    {
      EX_muteOn();
    }
  }

  // -- toneMode ----
  EX_TONE_MODE = 1;
  if (!EX_getStartup("toneMode", getData, startupJson))
  {
    return false;
  }
  else
  {
    if (getData != "")
    {
      getDataVal = getData.toInt();
      if ((getDataVal >= 0) && (getDataVal <= 3))
      {
        EX_TONE_MODE = getDataVal;
      }
    }
  }

  // -- ledEx ----
  EX_ledEx = true;
  if (!EX_getStartup("ledEx", getData, startupJson))
  {
    return false;
  }
  else
  {
    if (getData == "off")
    {
      EX_ledEx = false;
    }
  }

  // -- timer ----
  EX_TIMER_SEC = 180;
  if (!EX_getStartup("timer", getData, startupJson))
  {
    return false;
  }
  else
  {
    if (getData != "")
    {
      getDataVal = getData.toInt();
      EX_TIMER_SEC = getDataVal;
    }
  }
  return true;
}

bool EX_volumeSetup()
{
  String getData = "";
  size_t getDataVal;
  DynamicJsonDocument startupJson(EX_STARTUPJSON_SIZE);

  // -- startup.json から情報取得
  if (EX_getStartup("volume", getData, startupJson))
  {
    Serial.print("startup.json getData  volume = ");
    Serial.println(getData);

    if ((getData != "-1") && (getData != ""))
    {
      getDataVal = getData.toInt();

      if (getDataVal > 255)
        getDataVal = 255;
      if (getDataVal <= 0)
        getDataVal = 0;

      EX_VOLUME = getDataVal;
      Serial.print("startup.json get volume = ");
      Serial.println(getDataVal, DEC);

      M5.Speaker.setVolume(getDataVal);
      M5.Speaker.setChannelVolume(m5spk_virtual_channel, getDataVal);
      return (EX_voluemSVtoNVS(getDataVal));
    }
  }

  // NVSから情報取得
  if (EX_volumeRDfromNVS(&getDataVal))
  {
    Serial.print("NVS get volume = ");
    Serial.println(getDataVal, DEC);

    if (getDataVal > 255)
      getDataVal = 255;
    if (getDataVal <= 0)
      getDataVal = 0;
  }
  else
  { // NVSから取得できない場合
    getDataVal = 180;
  }

  EX_VOLUME = getDataVal;
  M5.Speaker.setVolume(getDataVal);
  M5.Speaker.setChannelVolume(m5spk_virtual_channel, getDataVal);
  Serial.println("EX_volumeSetup() done !");

  return (EX_voluemSVtoNVS(getDataVal));
}

bool EX_voluemSVtoNVS(size_t volume)
{ // ****** nvsにVoluemの値を保存する。 *****
  uint32_t nvs_handle;
  if (ESP_OK == nvs_open(EX_SETTING_NVS, NVS_READWRITE, &nvs_handle))
  {
    // volume = 180;
    EX_VOLUME = volume;
    nvs_set_u32(nvs_handle, "volume", volume);
    nvs_close(nvs_handle);
    M5.Speaker.setVolume(volume);
    M5.Speaker.setChannelVolume(m5spk_virtual_channel, volume);
    Serial.print("SV to NVS volume = ");
    Serial.println(volume, DEC);

    return true;
  }
  Serial.println("cannot save volume data to NVS");
  return false;
}

bool EX_volumeRDfromNVS(size_t *volume)
{ // ****** nvsからVoluemの値を呼び出す *****
  uint32_t nvs_handle1;
  if (ESP_OK == nvs_open(EX_SETTING_NVS, NVS_READONLY, &nvs_handle1))
  {
    size_t get_volume;
    nvs_get_u32(nvs_handle1, "volume", &get_volume);
    *volume = get_volume;

    Serial.print("NVS get volume = ");
    Serial.println(*volume, DEC);
    nvs_close(nvs_handle1);
    return true;
  }

  Serial.println("cannot read volume data from NVS");
  nvs_close(nvs_handle1);

  uint32_t nvs_handle2;
  if (ESP_OK == nvs_open(EX_SETTING_NVS, NVS_READWRITE, &nvs_handle2))
  {
    size_t volume2 = 180;
    nvs_set_u32(nvs_handle2, "volume", volume2);
    nvs_close(nvs_handle2);
    M5.Speaker.setVolume(volume2);
    M5.Speaker.setChannelVolume(m5spk_virtual_channel, volume2);
    Serial.println("init : volume = 180");
    EX_VOLUME = volume2;
    *volume = volume2;
  }
  nvs_close(nvs_handle2);
  return true;
}

void EX_servoSetup()
{
  EX_servoSetting();
  Servo_setup();
}

bool EX_servoSetting()
{
  // default value ------
  EX_SERVO_USE = true;
  SERVO_PIN_X = SV_PIN_X_CORE2_PA;
  SERVO_PIN_Y = SV_PIN_Y_CORE2_PA;
  EX_SERVO_PORT = "portA";
  DynamicJsonDocument startupJson(EX_STARTUPJSON_SIZE);

  // **** read startup.json(SD) file ****
  String getData1 = "";
  String getData2 = "";
  bool success1 = EX_getStartup("servo", getData1, startupJson);
  bool success2 = EX_getStartup("servoPort", getData2, startupJson);
  if (success1 && success2)
  {
    uint32_t nvs_handle1;
    // **** SAVE setting(NVS) ***
    if (ESP_OK != nvs_open(EX_SETTING_NVS, NVS_READWRITE, &nvs_handle1))
    {
      nvs_close(nvs_handle1);
      return false;
    }

    if (getData1 == "off")
      nvs_set_str(nvs_handle1, "servo", "off");
    else
      nvs_set_str(nvs_handle1, "servo", "on");

    if (getData2 == "portC")
      nvs_set_str(nvs_handle1, "servoPort", "portC");
    else
      nvs_set_str(nvs_handle1, "servoPort", "portA");

    nvs_close(nvs_handle1);
    Serial.println(" servo data save to nvs");
  }

  // *** Read setting (NVS) ****
  uint32_t nvs_handle2;
  if (ESP_OK == nvs_open(EX_SETTING_NVS, NVS_READONLY, &nvs_handle2))
  {
    // servo
    size_t length1;
    if (ESP_OK == nvs_get_str(nvs_handle2, "servo", nullptr, &length1) && length1)
    {
      char servo_str[length1 + 1];
      if (ESP_OK == nvs_get_str(nvs_handle2, "servo", servo_str, &length1))
      {
        String servoOnOff = String(servo_str);
        if (servoOnOff == "off")
          EX_SERVO_USE = false;
        else
          EX_SERVO_USE = true;
      }
    }

    // servoPort
    size_t length2;
    if (ESP_OK == nvs_get_str(nvs_handle2, "servoPort", nullptr, &length2) && length2)
    {
      char servoPort_str[length2 + 1];
      if (ESP_OK == nvs_get_str(nvs_handle2, "servoPort", servoPort_str, &length2))
      {
        String servoPort = String(servoPort_str);
        if (servoPort == "portC")
        {
          EX_SERVO_PORT = "portC";
          SERVO_PIN_X = SV_PIN_X_CORE2_PC;
          SERVO_PIN_Y = SV_PIN_Y_CORE2_PC;
        }
        else
        {
          SERVO_PIN_X = SV_PIN_X_CORE2_PA; // default: PortA
          SERVO_PIN_Y = SV_PIN_Y_CORE2_PA;
          EX_SERVO_PORT = "portA";
        }
      }
    }
  }
  nvs_close(nvs_handle2);
  Serial.println(" servo data read from nvs");

  return true;
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

  File file = SPIFFS.open(EX_CHATDOC_SPI, "r");
  if (!file)
  {
    file.close();
    String errorMsg1 = "*** Failed to open file for reading *** ";
    String errorMsg2 = "*** FATAL ERROR : cannot READ CHAT_DOC FILE !!!! ***";
    Serial.println(errorMsg1);
    Serial.println(errorMsg2);
    M5.Lcd.print(errorMsg1);
    M5.Lcd.print(errorMsg2);

    init_chat_doc(EX_json_ChatString.c_str());
    return false;
  }

  DeserializationError error = deserializeJson(CHAT_DOC, file);
  file.close();

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
      file = SPIFFS.open(EX_CHATDOC_SPI, "w");
      if (!file)
      {
        file.close();
        String errorMsg1 = "*** Failed to open file for writing *** ";
        String errorMsg2 = "*** FATAL ERROR : cannot WRITE CHAT_DOC FILE !!!! ***";
        Serial.println(errorMsg1);
        Serial.println(errorMsg2);
        M5.Lcd.print(errorMsg1);
        M5.Lcd.print(errorMsg2);
        return false;
      }
      serializeJson(CHAT_DOC, file);
      file.close();
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
    chatHistory.clear();
  }

  INIT_BUFFER = "";
  serializeJson(CHAT_DOC, INIT_BUFFER);
  Serial.println("InitBuffer = " + INIT_BUFFER);
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

  char tmp_str[40];
  sprintf(tmp_str, "Convert [%d].[%d].[%d].[%d]", iAddr[0], iAddr[1], iAddr[2], iAddr[3]);
  Serial.println(tmp_str);
  return true;
}


//-----Ver1.04 ----------------------------------------------------------
// // #define EX_TEST_UINT16
// #ifdef EX_TEST_UINT16
// void EX_test_uint16(uint16_t num)
// {
//   Serial.println(num, DEC);
// }
// #endif

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

bool EX_wifiFLRd()
{
  if (!SD.begin(GPIO_NUM_4, SPI, 25000000))
  { // SD無効な時
    Serial.println("SD disable ");
    SD.end();
    return false;
  }

  auto file = SD.open(EX_WIFI_SD, FILE_READ);
  if (!file)
  {
    SD.end();
    Serial.println("wifi.txt not open ");
    return false;
  }

  size_t sz = file.size();
  char buf[sz + 1];
  file.read((uint8_t *)buf, sz);
  buf[sz] = 0;
  file.close();
  SD.end();

  int y = 0;
  for (int x = 0; x < sz; x++)
  {
    if (buf[x] == 0x0a || buf[x] == 0x0d)
      buf[x] = 0;
    else if (!y && x > 0 && !buf[x - 1] && buf[x])
      y = x;
  }

  EX_SSID = buf;
  EX_SSID_PASSWD = &buf[y];

  Serial.print("\nSSID = ");
  Serial.println(EX_SSID);
  Serial.print("SSID_PASSWD = ");
  Serial.println(EX_SSID_PASSWD);
  if ((EX_SSID == "") || (EX_SSID_PASSWD == ""))
  {
    Serial.println("ssid or passwd is void ");
    return false;
  }

  return true;
}


bool EX_wifiTxtConnect()
{
  Serial.println("connecting wifi.txt");

  // "wifi.txt"ファイル
  if (!EX_wifiFLRd())
  {
    Serial.println(" faile to read wifi.txt");
    return false;
  }

  // "wifi.txt" 接続情報
  WiFi.begin(EX_SSID.c_str(), EX_SSID_PASSWD.c_str());

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
      Serial.println(" faile to connect  wifi.txt");
      return false;
    }
  }

  return true;
}

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
  if (EX_wifiTxtConnect())
  {
    Serial.println("\nwifi.txt wifi CONNECT");
    return true;
  }

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
    if (EX_randomSpeakState)
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
{
  // 全てのLEDを消灯
  for (int i = 0; i < NUM_LEDS; i++)
  {
    EX_setColorLED2(i, EX_ColorLED3(0, 0, 0));
  }
  EX_showLED();
  // delay(500); // 0.5秒待機
}

void EX_sysInfoDispStart(uint8_t mode_no)
{
  if (!EX_SYSINFO_DISP)
  {
    EX_muteOn();
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
  EX_muteOff();
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
  EX_SYSINFO_MSG += "\n*** Network Settings ***";
  EX_SYSINFO_MSG += "\nIP_Addr = " + EX_IP_ADDR;
  EX_SYSINFO_MSG += "\nSSID = " + EX_SSID;
  EX_SYSINFO_MSG += "\nSSID_PASSWD = " + EX_SSID_PASSWD;
  EX_SYSINFO_MSG += "\nopenAiApiKey = " + OPENAI_API_KEY;
  EX_SYSINFO_MSG += "\nvoiceTextApiKey = " + EX_VOICETEXT_API_KEY;
  EX_SYSINFO_MSG += "\nttsSelect = " + String(EX_ttsName[EX_TTS_TYPE]);
  EX_SYSINFO_MSG += "\nlang = " + LANG_CODE;
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

  if (EX_randomSpeakState)
  {
    msg = "\nrandomSpeak = on";
    EX_SYSINFO_MSG += msg;
  }
  else
  {
    msg = "\nrandomSpeak = off";
    EX_SYSINFO_MSG += msg;
  }

  uint32_t uptime = millis() / 1000;
  uint16_t up_sec = uptime % 60;
  uint16_t up_min = (uptime / 60) % 60;
  uint16_t up_hour = uptime / (60 * 60);

  sprintf(msg2, "\nuptime = %02d:%02d:%02d", up_hour, up_min, up_sec);
  EX_SYSINFO_MSG += msg2;

  sprintf(msg2, "\ntoneMode = %d", EX_TONE_MODE);
  EX_SYSINFO_MSG += msg2;

  if (EX_SERVO_USE)
  {
    sprintf(msg2, "\nservo = on");
    EX_SYSINFO_MSG += msg2;
  }
  else
  {
    sprintf(msg2, "\nservo = off");
    EX_SYSINFO_MSG += msg2;
  }

  sprintf(msg2, "\nservoPort = %s", EX_SERVO_PORT);
  EX_SYSINFO_MSG += msg2;

  if (EX_ledEx)
  {
    sprintf(msg2, "\nledEx = on");
    EX_SYSINFO_MSG += msg2;
  }
  else
  {
    sprintf(msg2, "\nledEx = off");
    EX_SYSINFO_MSG += msg2;
  }

  sprintf(msg2, "\nWK_ERR: No = %d Code = %d", EX_LAST_WK_ERROR_NO, EX_LAST_WK_ERROR_CODE);
  EX_SYSINFO_MSG += msg2;
}

void EX_sysInfo_m99_DispMake()
{
  EX_SYSINFO_MSG = "*** Test Mode  *** ";
}

void EX_randomSpeakStop2()
{
  if ((random_time != -1) || (EX_randomSpeakState == false))
  {
    return;
  }

  random_time = -1;
  EX_randomSpeakState = false;
  EX_RANDOM_SPEAK_ON_GET = false;
  EX_RANDOM_SPEAK_OFF_GET = false;
}

void EX_timerStop2()
{
  if (!EX_countdownStarted)
  {
    return;
  }

  // --- Timer を途中で停止 ------
  EX_countdownStarted = false;
  EX_TIMER_GO_GET = false;
  EX_TIMER_STOP_GET = false;
  EX_elapsedSeconds = 0;

  // 全てのLEDを消灯
  for (int i = 0; i < NUM_LEDS; i++)
  {
    EX_setColorLED2(i, EX_ColorLED3(0, 0, 0));
  }
  EX_showLED();
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
    if (!EX_randomSpeakState)
    {
      EX_timerStop2();
      EX_RANDOM_SPEAK_ON_GET = true;
      EX_RANDOM_SPEAK_OFF_GET = false;
      message += "mode=on";
    }
  }
  else if (arg_str == "off")
  {
    if (EX_randomSpeakState)
    {
      random_time = -1;
      EX_RANDOM_SPEAK_OFF_GET = true;
      EX_RANDOM_SPEAK_ON_GET = false;
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
    tmp = "独り言始めます。";
    EX_randomSpeakState = true;
  }
  else
  {
    tmp = "独り言やめます。";
    random_time = -1;
    EX_randomSpeakState = false;
  }

  // random_speak = !random_speak;
  avatar.setExpression(Expression::Happy);
  EX_ttsDo((char *)tmp.c_str(), tts_parms2);
  avatar.setExpression(Expression::Neutral);
  Serial.println("mp3 begin");

  EX_RANDOM_SPEAK_ON_GET = false;
  EX_RANDOM_SPEAK_OFF_GET = false;
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

  if (!EX_countdownStarted)
  {
    EX_randomSpeakStop2();
    EX_TIMER_STOP_GET = false;
    EX_TIMER_GO_GET = true;
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

  if (EX_countdownStarted)
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

  String message = "speakSelfIntro";
  EX_SELF_INTRO_GET = true;
  Serial.println(message);
  EX_tone(2);
  server.send(200, "text/plain", String("OK"));
}

// void EX_handle_version()
// {
//   // Version情報を送信
//   EX_tone(2);
//   String message = EX_VERSION;
//   server.send(200, "text/plain", message);
// }

void EX_timerStart()
{
  // ---- Timer 開始 ----------------
  if ((EX_TIMER_SEC < EX_TIMER_MIN) || (EX_TIMER_SEC > EX_TIMER_MAX))
  {
    Serial.println(EX_TIMER_SEC, DEC);
    return;
  }

  // Timer Start Go ----
  EX_countdownStartMillis = millis();

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
  EX_countdownStarted = true;
  EX_TIMER_GO_GET = false;
  EX_TIMER_STOP_GET = false;
}

void EX_timerStop()
{
  // --- Timer を途中で停止 ------
  EX_countdownStarted = false;
  EX_TIMER_GO_GET = false;
  EX_TIMER_STOP_GET = false;
  EX_elapsedSeconds = 0;

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
  int phase = (EX_elapsedSeconds / 5) % 2; // 往復の方向を決定
  int pos = EX_elapsedSeconds % 5;
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
  if ((EX_elapsedSeconds % 10 == 0) && (EX_elapsedSeconds < EX_TIMER_SEC))
  {
    char buffer[64];
    if (EX_elapsedSeconds < 60)
    {
      sprintf(buffer, "%d秒。", EX_elapsedSeconds);
    }
    else
    {
      int minutes = EX_elapsedSeconds / 60;
      int seconds = EX_elapsedSeconds % 60;
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
  EX_countdownStarted = false;
  EX_TIMER_GO_GET = false;
  EX_TIMER_STOP_GET = false;
  // elapsedMinutes = 0;
  EX_elapsedSeconds = 0;
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
  Serial.println(json_str);
  return true;
}

void handleRoot()
{
  server.send(200, "text/plain", "hello from m5stack!");
}

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
  String voice = server.arg("voice");
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
  if (voice != "")
  {
    parms_no = voice.toInt();
    if (parms_no < 0)
      parms_no = 0;
    if (parms_no > 4)
      parms_no = 4;
  }
  //  message = message + "\n";
  Serial.println(message);
  ////////////////////////////////////////
  // 音声の発声
  ////////////////////////////////////////
  avatar.setExpression(expressions_table[expr]);
  EX_ttsDo((char *)message.c_str(), tts_parms_table[parms_no]);
  //  avatar.setExpression(expressions_table[0]);
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
      // https.setTimeout(50000);

      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, url))
      { // HTTPS
        Serial.print("[HTTPS] POST...\n");
        // start connection and send HTTP header
        https.addHeader("Content-Type", "application/json");
        //        https.addHeader("Authorization", "Bearer YOUR_API_KEY");
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

// #define EX_DOC_SIZE 2000
// chatGPTから受取るデータのサイズを拡大した。 by NoRi 2023-05-28 
#define EX_DOC_SIZE 1024*4     

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
      // if (EX_TTS_TYPE == 0)
      // { // *** Global版以降では、次の行が削除されている。by NoRi ****
      std::replace(response.begin(), response.end(), '\n', ' ');
      // }
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
  tts_parms_no = 1;
  String text = server.arg("text");
  String voice = server.arg("voice");
  if (voice != "")
  {
    tts_parms_no = voice.toInt();
    if (tts_parms_no < 0)
      tts_parms_no = 0;
    if (tts_parms_no > 4)
      tts_parms_no = 4;
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

  // #ifdef USE_EXTEND
  //  ****  必ずrole-system-contentを１個はもつべきか？ ***
  //  --Ver105では、結論がでなかったのでこの処理はコメントにします。 --
  //   JsonArray messages = chat_doc["messages"];
  //   if (messages.size() < 2)
  //   { // role-system-contentがない場合には、初期化する。
  //     Serial.println("EX_json_ChatString init done! ");
  //     init_chat_doc(EX_json_ChatString.c_str());
  //     InitBuffer = "";
  //     serializeJson(chat_doc, InitBuffer);
  //     Role_JSON = InitBuffer;
  //     save_json();
  //   }
  // #endif

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
  init_chat_doc(Role_JSON.c_str());

  String role = CHAT_DOC["messages"][0]["role"];
  if (role == "user")
  {
    CHAT_DOC["messages"][0]["content"] = text;
  }
  String json_string;
  serializeJson(CHAT_DOC, json_string);

  response = chatGpt(json_string);
  SPEECH_TEXT = response;
}

void handle_apikey()
{
  // ファイルを読み込み、クライアントに送信する
  EX_tone(2);
  server.send(200, "text/html", APIKEY_HTML);
}

void handle_apikey_set()
{
  // POST以外は拒否
  if (server.method() != HTTP_POST)
  {
    return;
  }
  uint32_t nvs_handle;

  // openai
  String openai = server.arg("openai");
  if (openai != "")
  {
    OPENAI_API_KEY = openai;
    Serial.println(openai);

    if (ESP_OK == nvs_open(EX_APIKEY_NVS, NVS_READWRITE, &nvs_handle))
    {
      nvs_set_str(nvs_handle, "openai", openai.c_str());
    }
    nvs_close(nvs_handle);
  }

  // voicetxt
  String voicetext = server.arg("voicetext");
  if (voicetext != "")
  {
    tts_user = voicetext;
    EX_VOICETEXT_API_KEY = tts_user;
    Serial.println(voicetext);

    if (ESP_OK == nvs_open(EX_APIKEY_NVS, NVS_READWRITE, &nvs_handle))
    {
      nvs_set_str(nvs_handle, "voicetext", voicetext.c_str());
    }
    nvs_close(nvs_handle);
  }

  EX_tone(2);
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
  File file = SPIFFS.open("/data.json", "w");
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return false;
  }

  // JSONデータをシリアル化して書き込む
  serializeJson(CHAT_DOC, file);
  file.close();
  return true;
}

/**
 * アプリからテキスト(文字列)と共にRoll情報が配列でPOSTされてくることを想定してJSONを扱いやすい形に変更
 * 出力形式をJSONに変更
 */
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
    // 会話履歴をクリア
    chatHistory.clear();
  }
  INIT_BUFFER = "";
  serializeJson(CHAT_DOC, INIT_BUFFER);
  Serial.println("InitBuffer = " + INIT_BUFFER);
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
  EX_tone(2);
  server.send(200, "text/plain", String("OK"));
}

/// set M5Speaker virtual channel (0-7)
// static constexpr uint8_t m5spk_virtual_channel = 0;
static AudioOutputM5Speaker out(&M5.Speaker, m5spk_virtual_channel);
AudioGeneratorMP3 *mp3;

// for TTS00 --VoiceText(HOYA)
AudioFileSourceVoiceTextStream *file_TTS00 = nullptr;
AudioFileSourceBuffer *buff_TTS00 = nullptr;
// for TTS01 -- GoogleTTS
AudioFileSourcePROGMEM *file_TTS01 = nullptr;
const int mp3buffSize = 50 * 1024;
uint8_t mp3buff[mp3buffSize];

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

#ifdef USE_SERVO
#define START_DEGREE_VALUE_X 90
#define START_DEGREE_VALUE_Y 85 //
ServoEasing servo_x;
ServoEasing servo_y;
#endif

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

bool servo_home = false;

void servo(void *args)
{
  float gazeX, gazeY;
  DriveContext *ctx = (DriveContext *)args;
  Avatar *avatar = ctx->getAvatar();
  for (;;)
  {
#ifdef USE_SERVO
    if (EX_SERVO_USE)
    {
      if (!servo_home)
      {
        avatar->getGaze(&gazeY, &gazeX);
        servo_x.setEaseTo(START_DEGREE_VALUE_X + (int)(15.0 * gazeX));
        if (gazeY < 0)
        {
          int tmp = (int)(10.0 * gazeY);
          if (tmp > 10)
            tmp = 10;
          servo_y.setEaseTo(START_DEGREE_VALUE_Y + tmp);
        }
        else
        {
          servo_y.setEaseTo(START_DEGREE_VALUE_Y + (int)(10.0 * gazeY));
        }
      }
      else
      {
        //     avatar->setRotation(gazeX * 5);
        //     float b = avatar->getBreath();
        servo_x.setEaseTo(START_DEGREE_VALUE_X);
        //     servo_y.setEaseTo(START_DEGREE_VALUE_Y + b * 5);
        servo_y.setEaseTo(START_DEGREE_VALUE_Y);
      }
      synchronizeAllServosStartAndWaitForAllServosToStop();
#endif
    }

    delay(50);
  }
}

void Servo_setup()
{
#ifdef USE_SERVO
  if (EX_SERVO_USE)
  {
    if (servo_x.attach(SERVO_PIN_X, START_DEGREE_VALUE_X, DEFAULT_MICROSECONDS_FOR_0_DEGREE, DEFAULT_MICROSECONDS_FOR_180_DEGREE))
    {
      Serial.print("Error attaching servo x");
    }
    if (servo_y.attach(SERVO_PIN_Y, START_DEGREE_VALUE_Y, DEFAULT_MICROSECONDS_FOR_0_DEGREE, DEFAULT_MICROSECONDS_FOR_180_DEGREE))
    {
      Serial.print("Error attaching servo y");
    }
    servo_x.setEasingType(EASE_QUADRATIC_IN_OUT);
    servo_y.setEasingType(EASE_QUADRATIC_IN_OUT);
    setSpeedForAllServos(30);

    servo_x.setEaseTo(START_DEGREE_VALUE_X);
    servo_y.setEaseTo(START_DEGREE_VALUE_Y);
    synchronizeAllServosStartAndWaitForAllServosToStop();
  }
#endif
}

// ------- 完全版 NEW google_tts() ???? =======================
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
    int len = sizeof(mp3buff);
    int count = 0;

    bool data_end = false;
    while (!data_end)
    {
      if (ttsclient->available() > 0)
      {

        int bytesread = ttsclient->read(&mp3buff[i], len);
        // Serial.printf("%d Bytes Read\n",bytesread);
        i = i + bytesread;
        if (i > sizeof(mp3buff))
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
    file_TTS01 = new AudioFileSourcePROGMEM(mp3buff, i);
    mp3->begin(file_TTS01, &out);
  }
}

void VoiceText_tts(char *text, char *tts_parms)
{
  file_TTS00 = new AudioFileSourceVoiceTextStream(text, tts_parms);
  buff_TTS00 = new AudioFileSourceBuffer(file_TTS00, mp3buff, mp3buffSize);
  mp3->begin(buff_TTS00, &out);
}

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
static box_t box_servo;

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

// ---------------------------- < start of setup() > -------------------------------------
void setup()
{
  // ********** M5 config **************
  auto cfg = M5.config();
  cfg.external_spk = true; /// use external speaker (SPK HAT / ATOMIC SPK)
  // cfg.external_spk_detail.omit_atomic_spk = true; // exclude ATOMIC SPK
  // cfg.external_spk_detail.omit_spk_hat    = true; // exclude SPK HAT
  // cfg.internal_mic = true;
  M5.begin(cfg);
  // ------------------------------------

  M5.Lcd.setTextSize(2);

  // ******* SPEAKER setup **************
  { // -- custom setting
    auto spk_cfg = M5.Speaker.config();
    /// Increasing the sample_rate will improve the sound quality instead of increasing the CPU load.
    spk_cfg.sample_rate = 96000; // default:64000 (64kHz)  e.g. 48000 , 50000 , 80000 , 96000 , 100000 , 128000 , 144000 , 192000 , 200000
    spk_cfg.task_pinned_core = APP_CPU_NUM;
    M5.Speaker.config(spk_cfg);
  }
  M5.Speaker.begin();
  // ------------------------------------

  EX_servoSetup();
  EX_apiKeySetup();
  EX_volumeSetup();
  EX_ledSetup();

  // ******* wifi setup *****************
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
  server.on("/", handleRoot);
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

  // #ifdef USE_EXTEND
  // server.on("/test", EX_handle_test);
  server.on("/setting", EX_handle_setting);
  server.on("/startup", EX_handle_startup);
  server.on("/role1", EX_handle_role1);
  server.on("/role1_set", HTTP_POST, EX_handle_role1_set);
  server.on("/shutdown", EX_handle_shutdown);
  server.on("/wifiSelect", EX_handle_wifiSelect);
  server.on("/sysInfo", EX_handle_sysInfo);
  // server.on("/version", EX_handle_version);
  server.on("/timer", EX_handle_timer);
  server.on("/timerGo", EX_handle_timerGo);
  server.on("/timerStop", EX_handle_timerStop);
  server.on("/randomSpeak", EX_handle_randomSpeak);
  server.on("/speakSelfIntro", EX_handle_selfIntro);
  // #endif
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
  avatar.addTask(servo, "servo");
  avatar.setSpeechFont(&fonts::efontJA_16);
  box_servo.setupBox(80, 120, 80, 80);

  EX_modeSetup();
}
// ----------------------- < end of setup() > ----------------------------

String keywords[] = {"(Neutral)", "(Happy)", "(Sleepy)", "(Doubt)", "(Sad)", "(Angry)"};
void addPeriodBeforeKeyword(String &input, String keywords[], int numKeywords)
{
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
          if (expressionString[expressionIndx] == extractedString)
          {
            avatar.setExpression(expressions_table[expressionIndx]);
            break;
          }
          if (expressionString[expressionIndx] == "")
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

// String separator_tbl[2][7] = {{"。", "？", "！", "、", "", "　", ""}, {":", ",", ".", "?", "!", "\n", ""}};
// int search_separator(String text, int tbl)
// {
//   int i = 0;
//   int dotIndex_min = 1000;
//   int dotIndex;
//   while (separator_tbl[tbl][i] != "")
//   {
//     dotIndex = text.indexOf(separator_tbl[tbl][i++]);
//     if ((dotIndex != -1) && (dotIndex < dotIndex_min))
//       dotIndex_min = dotIndex;
//   }
//   if (dotIndex_min == 1000)
//     return -1;
//   else
//     return dotIndex_min;
// }

void loop()
{
  static int lastms = 0;
  static int lastms1 = 0;

  if (random_time >= 0 && millis() - lastms1 > random_time)
  {
    lastms1 = millis();
    random_time = 40000 + 1000 * random(30);
    if (!mp3->isRunning() && SPEECH_TEXT == "" && SPEECH_TEXT_BUFFER == "")
    {
      exec_chatGPT(random_words[random(18)]);
    }
  }

  // if (M5.BtnA.wasPressed())
  if (M5.BtnA.wasPressed() && (!mp3->isRunning()))
  {
    if (!EX_SYSINFO_DISP)
    {
      EX_tone(1);
      String tmp;
      if (!EX_randomSpeakState)
      {
        if (EX_isJP())
          tmp = "独り言始めます。";
        else
          tmp = "I'll start talking to myself.";

        lastms1 = millis();
        random_time = 40000 + 1000 * random(30);
        EX_randomSpeakState = true;
      }
      else
      {
        if (EX_isJP())
          tmp = "独り言やめます。";
        else
          tmp = "I'll stop talking to myself.";

        random_time = -1;
        EX_randomSpeakState = false;
      }
      // random_speak = !random_speak;
      avatar.setExpression(Expression::Happy);
      EX_ttsDo((char *)tmp.c_str(), tts_parms2);
      avatar.setExpression(Expression::Neutral);
      Serial.println("mp3 begin");
    }
  }

  M5.update();
#if defined(ARDUINO_M5STACK_Core2)
  auto count = M5.Touch.getCount();
  if (count)
  {
    auto t = M5.Touch.getDetail();
    if (t.wasPressed())
    {
#ifdef USE_SERVO
      if (EX_SERVO_USE)
      {
        if (box_servo.contain(t.x, t.y))
        {
          servo_home = !servo_home;
          M5.Speaker.tone(1000, 100);
        }
      }
#endif
    }
  }
#endif

#ifdef USE_EXTEND
  // --------------------------------------------------------------------
  // 独り言開始: ＜Aボタン＞と同じ機能
  if (EX_RANDOM_SPEAK_ON_GET && !EX_randomSpeakState)
  {
    EX_timerStop2();
    lastms1 = millis();
    random_time = 40000 + 1000 * random(30);
    EX_randomSpeak(true);
  }

  // 独り言終了: ＜Aボタン＞と同じ機能
  if (EX_RANDOM_SPEAK_OFF_GET && (EX_randomSpeakState))
  {
    EX_randomSpeak(false);
  }

  // タイマーが動作中の処理
  if (EX_countdownStarted)
  {
    uint32_t elapsedTimeMillis = millis() - EX_countdownStartMillis;
    uint16_t currentElapsedSeconds = elapsedTimeMillis / 1000;

    if (currentElapsedSeconds >= EX_TIMER_SEC)
    { // 指定時間が経過したら終了
      EX_timerEnd();
    }
    else if (EX_TIMER_STOP_GET)
    { // ---Timer停止---
      EX_timerStop();
    }
    else if (currentElapsedSeconds != EX_elapsedSeconds)
    { // --- Timer途中経過の処理------
      EX_elapsedSeconds = currentElapsedSeconds;
      EX_timerStarted();
    }
  }
  else
  {
    if (EX_TIMER_GO_GET)
    { // ---- Timer 開始 ----------------
      EX_randomSpeakStop2();
      EX_timerStart();
    }
  }

  // ＜Ｂボタン＞押された時の処理（タイマー機能）
  if (M5.BtnB.wasPressed())
  {
    if (!EX_SYSINFO_DISP)
    {
      EX_tone(1);

      if (!EX_countdownStarted)
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

  // ＜Ｃボタン＞（システム情報表示）
  if (M5.BtnC.wasPressed())
  {
    if (!EX_SYSINFO_DISP)
    {
      EX_sysInfoDispStart(0);
    }
    else
    {
      EX_sysInfoDispEnd();
    }
    EX_tone(1);
  }

  // 自己紹介 -- ＜Cボタン＞と同じ機能
  if (EX_SELF_INTRO_GET)
  {
    avatar.setExpression(Expression::Happy);

    char text1[] = "みなさんこんにちは、私の名前はスタックチャンです、よろしくね。";
    EX_ttsDo(text1, tts_parms2);
    avatar.setExpression(Expression::Neutral);
    Serial.println("mp3 begin");
    EX_SELF_INTRO_GET = false;
  }

#endif
  // --------------< end of USE_EXTEND > -----------------------------------------
  // if (M5.BtnC.wasPressed())
  // {
  //   M5.Speaker.tone(1000, 100);
  //   avatar.setExpression(Expression::Happy);
  //   VoiceText_tts(text1, tts_parms2);
  //   avatar.setExpression(Expression::Neutral);
  //   Serial.println("mp3 begin");
  // }

  // ********* Speech Text がある場合の処理 *************
  if (SPEECH_TEXT != "")
  {
    SPEECH_TEXT_BUFFER = SPEECH_TEXT;
    SPEECH_TEXT = "";
    addPeriodBeforeKeyword(SPEECH_TEXT_BUFFER, keywords, 6);
    Serial.println("-----------------------------");
    Serial.println(SPEECH_TEXT_BUFFER);
    //---------------------------------
    String sentence = SPEECH_TEXT_BUFFER;
    int dotIndex;
    if (EX_isJP())
    {
      // dotIndex = search_separator(speech_text_buffer, 0);
      dotIndex = SPEECH_TEXT_BUFFER.indexOf("。");
    }
    else
    {
      // dotIndex = search_separator(speech_text_buffer, 1);
      dotIndex = SPEECH_TEXT_BUFFER.indexOf(".");
    }
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

    //----------------
    getExpression(sentence, expressionIndx);
    //----------------
    if (expressionIndx < 0)
      avatar.setExpression(Expression::Happy);

    if (expressionIndx < 0)
      EX_ttsDo((char *)sentence.c_str(), tts_parms_table[tts_parms_no]);
    else
    {
      String tmp = emotion_parms[expressionIndx] + tts_parms[tts_parms_no];
      EX_ttsDo((char *)sentence.c_str(), (char *)tmp.c_str());
    }

    if (expressionIndx < 0)
      avatar.setExpression(Expression::Neutral);
  }

  // ********* 音声を出力中 の処理　************
  if (mp3->isRunning())
  {
    // if (millis()-lastms > 1000) {
    //   lastms = millis();
    //   Serial.printf("Running for %d ms...\n", lastms);
    //   Serial.flush();
    //  }

    if (!mp3->loop())
    {
      // **********************
      // ***** mp3　STOP　*****
      // **********************
      mp3->stop();
      if ((EX_TTS_TYPE == 0) && (file_TTS00 != nullptr))
      {
        delete file_TTS00;
        file_TTS00 = nullptr;
      }

      if ((EX_TTS_TYPE == 1) && (file_TTS01 != nullptr))
      {
        delete file_TTS01;
        file_TTS01 = nullptr;
      }
      Serial.println("mp3 stop");

      if (SPEECH_TEXT_BUFFER != "")
      {
        // ***************************************************************
        // ***** 次に話す sentence を speech_text_bufferから切出す処理 ****
        // ***************************************************************
        String sentence = SPEECH_TEXT_BUFFER;
        int dotIndex;
        if (EX_isJP())
        {
          // dotIndex = search_separator(speech_text_buffer, 0);
          dotIndex = SPEECH_TEXT_BUFFER.indexOf("。");
        }
        else
        {
          // dotIndex = search_separator(speech_text_buffer, 1);
          dotIndex = SPEECH_TEXT_BUFFER.indexOf(".");
        }

        if (dotIndex != -1)
        {
          if (EX_isJP())
            dotIndex += 3;
          else
            dotIndex += 2;

          sentence = SPEECH_TEXT_BUFFER.substring(0, dotIndex);
          Serial.println(sentence);
          SPEECH_TEXT_BUFFER = SPEECH_TEXT_BUFFER.substring(dotIndex);
          // ************************************************************
        }
        else
        {
          SPEECH_TEXT_BUFFER = "";
        }

        //----------------
        getExpression(sentence, expressionIndx);
        //----------------
        if (expressionIndx < 0)
          avatar.setExpression(Expression::Happy);

        if (expressionIndx < 0)
          EX_ttsDo((char *)sentence.c_str(), tts_parms_table[tts_parms_no]);
        else
        {
          String tmp = emotion_parms[expressionIndx] + tts_parms[tts_parms_no];
          EX_ttsDo((char *)sentence.c_str(), (char *)tmp.c_str());
        }

        if (expressionIndx < 0)
          avatar.setExpression(Expression::Neutral);
      }
      else
      {
        avatar.setExpression(Expression::Neutral);
        expressionIndx = -1;
      }
    }
  }
  else
  {
    server.handleClient();
  }
}
