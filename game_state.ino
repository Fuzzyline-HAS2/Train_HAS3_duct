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

    if((String)(const char *)my["device_state"] != (String)(const char *)cur["device_state"]){
        if((String)(const char *)my["device_state"] == "github"){
            ota.check();
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

    Serial.println("Data Change");
    cur = my;
}
