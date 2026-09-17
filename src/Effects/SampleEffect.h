#pragma once

#include "IEffect.h"

/*
 * SampleEffect - 示例效果
 *
 * 演示如何使用 IEffect 接口。
 * 当单位受到伤害时，在伤害应用前输出调试日志。
 */
class SampleEffect : public IEffect
{
public:
    SampleEffect(const char* debugTag)
        : _debugTag(debugTag)
        , _lifetime(600)  // 存活 600 帧 (约 10 秒)
    { }

    // 单位受到伤害时调用
    virtual bool OnReceiveDamage(DamageEvent& damageEvent) override
    {
        if (!damageEvent.Damage || *damageEvent.Damage <= 0)
            return false;

        Debug::Log("[SampleEffect:%s] 受到伤害! "
            "damage=%d, warhead=%s, attacker=%08x\n",
            _debugTag.c_str(),
            *damageEvent.Damage,
            damageEvent.WH ? damageEvent.WH->ID : "null",
            (DWORD)damageEvent.Attacker);

        return false;  // 不改动伤害
    }

    // 每帧更新
    virtual void OnUpdate() override
    {
        if (_lifetime > 0)
            --_lifetime;
    }

    // 效果是否存活
    virtual bool IsAlive() const override
    {
        return _lifetime > 0;
    }

private:
    std::string _debugTag;
    int _lifetime;
};
