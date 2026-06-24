#include "Train_HAS3_duct.h"

void UpdateBrightness()
{
    int serverBrightness = my["brightness"].as<int>();
    if (serverBrightness <= 0 || serverBrightness > 100) {
        colorBrightness = DEFAULT_COLOR_BRIGHTNESS;
        lineBrightness  = DEFAULT_LINE_BRIGHTNESS;
    } else {
        colorBrightness = map(serverBrightness, 1, 100, 1, 255);
        lineBrightness  = map(serverBrightness, 1, 100, 1, 255);
    }
    pixels_line.setBrightness(lineBrightness);
    pixels_round.setBrightness(colorBrightness);
    pixels_switch.setBrightness(colorBrightness);
}

/**
 * @brief 현재 game_state에 맞게 네오픽셀 색상을 재적용
 */
void ApplyCurrentNeopixel()
{
    if (game_state == setting) {
        pixels_line.lightColor(line_white);
        pixels_round.lightColor(white);
        pixels_switch.lightColor(white);
    } else if (game_state == ready) {
        pixels_line.lightColor(line_red);
        pixels_round.lightColor(red);
        pixels_switch.lightColor(red);
    } else if (game_state == activate) {
        if (duct_available) {
            pixels_line.lightColor(line_yellow);
            pixels_round.lightColor(yellow);
            pixels_switch.lightColor(yellow);
        } else {
            pixels_line.lightColor(line_red);
            pixels_round.lightColor(red);
        }
    }
}

/**
 * @brief DB gamestate가 setting 일 때 한번 동작하는 코드
 */
void SettingFunc()
{
    UpdateBrightness();
    game_state = setting;

    use_duct_num = 0;
    duct_available = true;
    duct_open_bool = false;
    current_time = 0;
    cooltime = 0;
    cooltime_timer.deleteTimer(cooltime_timer_id);

    pixels_line.lightColor(line_white);
    pixels_round.lightColor(white);
    pixels_switch.lightColor(white);
}

/**
 * @brief DB gamestate가 ready 일 때 한번 동작하는 코드
 */
void ReadyFunc()
{
    UpdateBrightness();
    game_state = ready;

    use_duct_num = 0;
    duct_available = true;
    duct_open_bool = false;
    current_time = 0;
    cooltime = 0;
    cooltime_timer.deleteTimer(cooltime_timer_id);

    pixels_line.lightColor(line_red);
    pixels_round.lightColor(red);
    pixels_switch.lightColor(red);
}

/**
 * @brief DB gamestate가 activate 일 때 반복 동작하는 코드
 */
void ActivateFunc()
{
    RfidLoop();
    if(!digitalRead(SW_PIN) && switch_available) { DuctOpen(true); }
    if(!digitalRead(EMCHECK_PIN)){ duct_kill_bool = false; }
}

/**
 * @brief DB gamestate가 activate 일 때 한번 동작하는 코드
 */
void ActivateRunOnce()
{
    UpdateBrightness();
    game_state = activate;
    duct_open_bool = false;

    // 쿨타임과 쿨타임 증가량을 DB에서 읽어 사용할 수 있음
    cooltime_set = (int)my["cool_time"];
    cooltime_add = (int)my["cool_time_add"];

    pixels_line.lightColor(line_yellow);
    pixels_round.lightColor(yellow);
    pixels_switch.lightColor(yellow);
}

/**
 * @brief 주기적으로 DB를 읽어옴
 */
void DataChange()
{
    static StaticJsonDocument<1000> cur;

    // device_state 분기는 항상 처리 (tagger 동결 중에도 back 수신을 감지해야 함)
    if((String)(const char *)my["device_state"] != (String)(const char *)cur["device_state"]){
        if((String)(const char *)my["device_state"] == "github"){
            ota.check();
        }
        else if((String)(const char *)my["device_state"] == "tagger"){
            EnterTaggerMode();
        }
        else if((String)(const char *)my["device_state"] == "back"){
            ExitTaggerMode();
        }
    }

    // 아래 분기들은 tagger 동결 중에는 무시 (game_state/cool_time 보존)
    if(!tagger_mode){
        if ((String)(const char *)my["game_state"] != (String)(const char *)cur["game_state"]){
            if ((String)(const char *)my["game_state"] == "setting"){
                SettingFunc();
            }
            else if ((String)(const char *)my["game_state"] == "ready"){
                ReadyFunc();
            }
            else if ((String)(const char *)my["game_state"] == "activate"){
                ActivateRunOnce();
            }
        }

        if((String)(const char *)my["manage_state"] != (String)(const char *)cur["manage_state"]){
            if((String)(const char *)my["manage_state"] == "mo"){
                DuctOpen();
            }
        }

        if((String)(const char *)my["brightness"] != (String)(const char *)cur["brightness"]){
            UpdateBrightness();
            ApplyCurrentNeopixel();
        }
    }

    Serial.println("Data Change");
    cur = my;
}

/**
 * @brief "이로운 효과" 진입 - tagger 수신 시 현 상태와 무관하게 덕트를 동결
 *        (전체 보라색, RFID 비활성, 쿨타임 일시정지). game_state/cool_time은 손대지 않아 보존됨.
 */
void EnterTaggerMode()
{
    if (tagger_mode) return;   // 재진입 방지 (tagger -> activate -> tagger 등)
    tagger_mode = true;        // RfidLoop / CooltimeTimerFunc 자동 정지
    pixels_line.lightColor(line_purple);
    pixels_round.lightColor(purple);
    pixels_switch.lightColor(purple);
    tagger_timer_id = tagger_timer.setTimeout(10000, AutoExitTaggerMode);  // 훈련소: 10초 뒤 자동 back
    Serial.println("Enter Tagger Mode");
}

/**
 * @brief "이로운 효과" 해제 - back 수신 시 동결을 풀고 원래 game_state 색상으로 복귀.
 *        값들은 동결 중 변하지 않았으므로 별도 복원 없이 재페인트만 수행 (서버 전송 없음).
 */
void ExitTaggerMode()
{
    if (!tagger_mode) return;  // tagger 상태일 때만 동작
    tagger_mode = false;       // RFID / 쿨타임 게이팅 해제 (멈췄던 지점부터 재개)
    if (tagger_timer.isEnabled(tagger_timer_id))
    {
        tagger_timer.deleteTimer(tagger_timer_id);  // 서버 back이 10초 전에 먼저 온 경우 중복 실행 방지
    }
    ApplyCurrentNeopixel();    // 복원된 game_state / duct_available 기준 색 복원
    Serial.println("Exit Tagger Mode");
}

/**
 * @brief 훈련소 전용 - 10초 타임아웃으로 tagger 모드를 자동 해제할 때,
 *        서버에 device_state "activate"를 통보한 뒤 동결 해제 (서버 back 수신 경로와 구분).
 */
void AutoExitTaggerMode()
{
    if (!tagger_mode) return;  // 이미 해제됐으면 무시
    has2wifi.Send((String)(const char *)my["device_name"], "device_state", "activate");
    ExitTaggerMode();
}
