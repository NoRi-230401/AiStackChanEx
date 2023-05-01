// ---------------------------< AiStackChanEx.h >------------------------------------
#ifndef AI_STACKCHAN_EX_H
#define AI_STACKCHAN_EX_H
// ---------------------------

#include <Adafruit_NeoPixel.h>
#define PIN 25                                                 // GPIO25でLEDを使用する
#define NUM_LEDS 10                                            // LEDの数を指定する
Adafruit_NeoPixel pixels(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800); // 800kHzでNeoPixelを駆動 おまじない行

#define EX_TIMER_INIT 180          // タイマー初期値：３分
#define EX_TIMER_MIN 30            // 最小タイマー設定値：３０秒
#define EX_TIMER_MAX (60 * 60 - 1) // 最大タイマー設定値：６０分未満 (59分59秒)

// --- v103 --
void EX_LED_allOff();
void EX_randomSpeakStop2();
void EX_timerStop2();
void handle_sysInfo();
void EX_sysInfoDisp();
void EX_sysInfoDispStart(uint8_t mode_no);
void EX_sysInfoDispEnd();
uint8_t EX_getBatteryLevel();
void EX_sysInfo_m00_DispMake();
void EX_sysInfo_m01_DispMake();
void handle_setting();
void EX_muteOn();
void EX_muteOff();

// --- v102 --
void handle_randomSpeak();
void EX_randomSpeak(bool mode);
void EX_timerStart();
void EX_timerStop();
void EX_timerStarted();
void EX_timerEnd();

// --- v101 --
void handle_timer();
void handle_timerGo();
void handle_timerStop();
void handle_selfIntro();
void handle_version();


//-----------------------------------------------------------------------------------
bool init_chat_doc(const char *data);

void handleRoot();
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
void handle_role_set2();
void handle_role_set1();
void handle_face();

void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string);
void StatusCallback(void *cbData, int code, const char *string);
void lipSync(void *args);
void servo(void *args);
void Servo_setup();
void VoiceText_tts(char *text, char *tts_parms);
void Wifi_setup();

void addPeriodBeforeKeyword(String &input, String keywords[], int numKeywords);
void getExpression(String &sentence, int &expressionIndx);
//------------------------------------------------------------------------------------

void setup();
void loop();

// ---- end of < AI_STACKCHAN_EX_H > ------------------------------------------------
#endif
