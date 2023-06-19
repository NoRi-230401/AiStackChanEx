// ---------------------------< AiStackChanEx.h >------------------------------------
#ifndef AI_STACKCHAN_EX_H
#define AI_STACKCHAN_EX_H
// ---------------------------
#include <sys/socket.h> //アドレスドメイン
#include <sys/types.h> //ソケットタイプ
#include <arpa/inet.h> //バイトオーダの変換に利用

#include <Adafruit_NeoPixel.h>
#define PIN 25                                                 // GPIO25でLEDを使用する
#define NUM_LEDS 10                                            // LEDの数を指定する
Adafruit_NeoPixel pixels(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800); // 800kHzでNeoPixelを駆動 おまじない行

#define EX_TIMER_INIT 180          // タイマー初期値：３分
#define EX_TIMER_MIN 30            // 最小タイマー設定値：３０秒
#define EX_TIMER_MAX (60 * 60 - 1) // 最大タイマー設定値：６０分未満 (59分59秒)
#define EX_WK_CNT_MAX 10           // 「わかりません」が何回連続すると初期化するか
#define EX_SHUTDOWN_MIN_TM 3

// ---- servo define 
#define SV_CENTER_X 90
#define SV_CENTER_Y 90
#define SV_PIN_X_CORE2_PA 33 // Core2 PORT A
#define SV_PIN_Y_CORE2_PA 32
#define SV_PIN_X_CORE2_PC 13 // Core2 PORT C
#define SV_PIN_Y_CORE2_PC 14

#define SV_MD_MOVING 0
#define SV_MD_HOME 1
#define SV_MD_ADJUST 2
#define SV_MD_STOP 3
#define SV_MD_CENTER 4
#define SV_MD_POINT 5
#define SV_MD_DELTA 6
#define SV_MD_SWING 7
#define SV_MD_NONE 8

#define SV_REQ_MSG_CLS 0
#define SV_REQ_SPEAK 1
#define SV_REQ_MSG 2
#define SV_REQ_SPEAK_MSG 3
#define SV_REQ_MD_ADJUST 9


// #define SV_SWING_MD_X 0
// #define SV_SWING_MD_Y 1
// #define SV_SWING_MD_XY 2

#define SV_SWING_AXIS_X 0
#define SV_SWING_AXIS_Y 1
#define SV_SWING_AXIS_XY 2


#define SV_X_MIN 0
#define SV_X_MAX 180
#define SV_Y_MIN 50
#define SV_Y_MAX 100

// --- v111



// --- v110
void sv_setEaseToX(int x);
void sv_setEaseToY(int y);
void sv_setEaseToXY(int x, int y);
void sv_easeToX(int x);
void sv_easeToY(int y);
void sv_easeToXY(int x, int y);

void EX_servo(void *args);
void EX_handle_servo();
void EX_handle_setting();
void EX_report_batt_level();
void EX_RandomSpeakOperation();
void EX_ServoOperation();
void EX_SpeechText1st();
void EX_SpeechTextNext();

// --- v109
bool EX_StartSetting();

// --- v108
void EX_handleRoot();

// --- v107
bool EX_wifiSelctFLRd(DynamicJsonDocument& wifiJson);
bool EX_wifiSelctFLSv(DynamicJsonDocument& wifiJson);
bool EX_initWifiJosn(DynamicJsonDocument& wifiJson);
bool EX_wifiSelectConnect();
void handle_exWifi();
bool EX_apiKeyStartupJson1();
bool EX_apiKeyStartupJson2();
void handle_exStartup();
// bool EX_startupFLRd(DynamicJsonDocument &startupJson);
bool EX_startupFLSv(DynamicJsonDocument &startupJson);
bool EX_setStartup(String item, String data,DynamicJsonDocument& startupJson);
bool EX_getStartup(String item, String &data,DynamicJsonDocument& startupJson);
bool EX_setGetStrToStartSetting(const char *item, DynamicJsonDocument &startupJson);

// --- v106
void EX_handle_sysInfo();
// void EX_handle_setting();
bool EX_apiKeySetup();
bool EX_apiKeyTxt();
bool EX_apiKeyFmNVS();
bool EX_isJP();
void EX_setColorLED2(uint16_t n, uint32_t c);
void EX_setColorLED4(uint16_t n, uint8_t r, uint8_t g, uint8_t b);
static uint32_t EX_ColorLED3(uint8_t r, uint8_t g, uint8_t b);
void EX_showLED();
void EX_clearLED();
void EX_ledSetup();
bool EX_modeSetup();
bool EX_volumeSetup();
bool EX_voluemSVtoNVS(size_t volume);
bool EX_volumeRDfromNVS(size_t &volume);
void EX_servoSetup();
bool EX_servoSetting();
// bool EX_startupFLRd();
// bool EX_startupFLSv();
// bool EX_setGetStrToStartSetting(const char *item);
void google_tts(char *text, char *lang); // New 

// --- v105
void EX_errStop(const char *msg);
void EX_errReboot(const char *msg);
bool EX_chatDocInit();
void EX_handle_test();
void EX_handle_role1();
void EX_handle_role1_set();
bool EX_strIPtoIntArray(String strIPaddr, int *iAddr);
bool EX_wifiSelectConnect();
void handle_exWifi();

// --- v104
// void EX_test_uint16(uint16_t num);
void EX_toneOn();
void EX_tone(int mode);
void EX_handle_shutdown();
void EX_wifiTxtConfig();
bool EX_wifiFLRd();
bool EX_wifiTxtConnect();
bool EX_wifiNoSetupFileConnect();
bool EX_wifiSmartConfigConnect();
bool EX_wifiConnect();
bool EX_sysInfoGet(String txArg, String& txData);

// --- v103
void EX_LED_allOff();
void EX_sysInfoDisp();
void EX_sysInfo_m00_DispMake();
void EX_sysInfo_m01_DispMake();
uint8_t EX_getBatteryLevel();
void EX_sysInfoDispMake();
void EX_muteOn();
void EX_muteOff();

// --- v102
void EX_handle_randomSpeak();
void EX_randomSpeak(bool mode);

// --- v101
void EX_handle_timer();
void EX_handle_timerStop();
void EX_handle_timerGo();
void EX_handle_selfIntro();
// void EX_handle_version();
void EX_timerStart();
void EX_timerStop();
void EX_timerStarted();
void EX_timerEnd();

//-----------------------------------------------------------------------------------
bool init_chat_doc(const char *data);
// void handleRoot();
void handleNotFound();
void handle_speech();
String https_post_json(const char *url, const char *json_string, const char *root_ca);
String chatGpt(String json_string);
void handle_chat();
void handle_apikey();
void handle_apikey_set();
void handle_role();
bool save_json();
void handle_role_set();
// void handle_role_set2();
// void handle_role_set1();
// void handle_setting();
void handle_face();

void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string);
void StatusCallback(void *cbData, int code, const char *string);
void lipSync(void *args);
void EX_servo(void *args);
void Servo_setup();
void EX_ttsDo(char *text, char *tts_parms);
void addPeriodBeforeKeyword(String &input, String keywords[], int numKeywords);
void getExpression(String &sentence, int &expressionIndx);
//------------------------------------------------------------------------------------

void setup();
void loop();

// ---- end of < AI_STACKCHAN_EX_H > ------------------------------------------------
#endif
