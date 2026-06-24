#include "Train_HAS3_duct.h"

/**
 * @brief 덕트 사용시 동작
 */
void DuctTag(String tag_player)
{
    if (duct_available)
    {
        tag_player_name = tag_player;
        use_duct_num++;
        CooltimeCalculation();
        DuctOpen();
        TagPlayerSend();
    }
    else
    {
        CooltimeMp3();
    }
}

void DuctGhost(String tag_ghost)
{
    ghost_name = tag_ghost;
    Mp3PlayLargeFolder(1, 2);
    duct_open_bool = true;
    switch_available = false;
    duct_available = false;
    pixels_line.lightColor(line_blue);
    pixels_switch.lightColor(blue);
    pixels_round.lightColor(blue);
    digitalWrite(RELAY_PIN, HIGH);
    duct_close_timer_id = duct_close_timer.setTimeout(4000, DuctClose);
}

void DuctOpen(bool switch_push)
{
    if (duct_available || switch_push)
    {
        if (cooltime_timer.isEnabled(cooltime_timer_id))
        {
            cooltime_timer.deleteTimer(cooltime_timer_id);
        }
        Mp3PlayLargeFolder(1, 2);
        duct_open_bool = true;
        switch_available = false;
        duct_available = false;
        pixels_line.lightColor(line_red);
        pixels_switch.lightColor(red);
        pixels_round.lightColor(red);
        digitalWrite(RELAY_PIN, HIGH);
        duct_close_timer_id = duct_close_timer.setTimeout(4000, DuctClose);
    }
}

void DuctClose()
{
    digitalWrite(RELAY_PIN, LOW);
    duct_open_bool = false;
    pixels_line.lightColor(line_red);
    pixels_switch.lightColor(yellow);
    switch_available = true;
    current_time = 0;
    cool_time_neo_bool = true;
    if (!cooltime_timer.isEnabled(cooltime_timer_id))
    {
        cooltime_timer_id = cooltime_timer.setInterval(1000, CooltimeTimerFunc);
    }
    has2wifi.Send((String)(const char *)my["device_name"], "device_state", "lock");
}

/**
 * @brief 덕트 사용횟수에 따른 쿨타임 계산 함수
 */
void CooltimeCalculation()
{
    switch (use_duct_num)
    {
    case 1:
    case 2:
        cooltime = cooltime_set + cooltime_add * 0;
        break;
    case 3:
    case 4:
        cooltime = cooltime_set + cooltime_add * 1;
        break;
    case 5:
    case 6:
        cooltime = cooltime_set + cooltime_add * 2;
        break;
    case 7:
    case 8:
        cooltime = cooltime_set + cooltime_add * 3;
        break;
    case 9:
    case 10:
        cooltime = cooltime_set + cooltime_add * 4;
        break;
    default:
        break;
    }
}

void TagPlayerSend()
{
    has2wifi.Send((String)(const char *)my["device_name"], "tag_player", tag_player_name);
}

void DuctKill()
{
    // 가장 최근 태그한 플레이어 정보를 DB에서 가져옴
    String kill_player = (String)(const char *)my["tag_player"];

    has2wifi.Receive(kill_player);

    // 대상 검증 없이 술래가 태그하면 무조건 킬 처리
    duct_kill_bool = true;
    Serial.println("Duct Kill!");
    if ((String)(const char *)shift_machine["selected_language"] == "EN")
    {
        Mp3PlayLargeFolder(4, 3);
    }
    cool_time_neo_bool = false;
    pixels_line.lightColor(purple);
    pixels_round.lightColor(purple);
    delay(500);
    pixels_line.clear(); pixels_line.show();
    pixels_round.clear(); pixels_round.show();
    delay(500);
    pixels_line.lightColor(purple);
    pixels_round.lightColor(purple);
    delay(500);
    pixels_line.clear(); pixels_line.show();
    pixels_round.clear(); pixels_round.show();
    delay(500);
    if (duct_available)
    {
        pixels_line.lightColor(line_yellow);
        pixels_round.lightColor(yellow);
        pixels_switch.lightColor(yellow);
    }
    else
    {
        pixels_round.lightColor(red);
        pixels_line.lightColor(line_red);
    }

    has2wifi.Send(kill_player, "life_chip", "-1");
    if ((int)tag["life_chip"] > 1)
    {
        has2wifi.Send(kill_player, "role", "revival");
    }
    else if ((int)tag["life_chip"] == 1)
    {
        has2wifi.Send(kill_player, "role", "ghost");
    }

    has2wifi.Send(tagger_name, "taken_chip", "+1");
    has2wifi.Send(tagger_name, "exp", "+130");

    cool_time_neo_bool = true;
}
