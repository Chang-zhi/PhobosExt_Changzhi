#pragma once

#include "DamageEvent.h"

/*
 * IEffect - 效果抽象接口
 *
 * 所有自定义效果都应继承此类并实现虚函数。
 * 效果通过 TechnoExt::ExtData 附加到单位上，
 * 在单位受到伤害时自动被调用。
 *
 * 使用示例:
 *   class MyEffect : public IEffect { ... };
 *   auto pExt = TechnoExt::ExtMap.Find(pTechno);
 *   pExt->Effects.push_back(std::make_unique<MyEffect>());
 */
class IEffect
{
public:
    virtual ~IEffect() = default;

    // ── 单位受到伤害时调用 ──────────────────────────────
    // 在 Verses 计算和扣血之前触发。
    // 返回 true 表示"已处理此事件并修改了伤害"。
    // 注意: 修改 damageEvent.Damage 指向的值会影响最终扣血量。
    virtual bool OnReceiveDamage(DamageEvent& damageEvent) { return false; }

    // ── 每帧更新 ────────────────────────────────────────
    // 在 TechnoClass::AI 中被调用。
    virtual void OnUpdate() { }

    // ── 效果是否仍存活 ──────────────────────────────────
    // 返回 false 时，框架会自动从 Effects 列表中移除该效果。
    virtual bool IsAlive() const { return true; }
};
