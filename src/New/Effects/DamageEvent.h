#pragma once

#include <WarheadTypeClass.h>
#include <TechnoClass.h>
#include <HouseClass.h>
#include <SpecificStructures.h>

/*
 * DamageEvent - 封装一次伤害事件的所有信息
 * 对应原版 ObjectClass::ReceiveDamage 的栈参数
 * 参考 YRpp/SpecificStructures.h 中的 args_ReceiveDamage
 */
struct DamageEvent
{
    int* Damage;                  // 伤害值指针（可修改）
    int DistanceToEpicenter;      // 距爆炸中心距离
    WarheadTypeClass* WH;         // 弹头
    TechnoClass* Attacker;        // 攻击者
    bool IgnoreDefenses;          // 是否无视防御
    bool PreventPassengerEscape;  // 是否阻止乘客逃生
    HouseClass* AttackingHouse;   // 攻击方所属国家

    // 从 ReceiveDamage 栈参数构造
    explicit DamageEvent(struct args_ReceiveDamage* args)
        : Damage(args->Damage)
        , DistanceToEpicenter(args->DistanceToEpicenter)
        , WH(args->WH)
        , Attacker(args->Attacker)
        , IgnoreDefenses(args->IgnoreDefenses)
        , PreventPassengerEscape(args->PreventsPassengerEscape)
        , AttackingHouse(args->SourceHouse)
    { }
};
