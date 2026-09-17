#include "Body.h"

#include <Utilities/Debug.h>
#include <FootClass.h>

#include <Effects/IEffect.h>
#include <Effects/DamageEvent.h>
#include <Ext/WarheadType/Body.h>

// ============================================================
//  Hook: TechnoClass_ReceiveDamage_Effects
//  地址: 0x701900 (TechnoClass::ReceiveDamage 入口)
//  目的:
//    1. 将 DamageEvent 分发给该单位注册的所有 IEffect
//    2. 处理 WarheadTypeExt::BerserkReduce 逻辑
//
//  BerserkReduce: 无论是否造成伤害，都会减少目标的混乱帧数，
//  如果减到0或以下，则立即清除混乱状态并停止单位。
// ============================================================

DEFINE_HOOK(0x701900, TechnoClass_ReceiveDamage_Effects, 0x6)
{
    GET(TechnoClass*, pThis, ECX);
    LEA_STACK(args_ReceiveDamage*, pArgs, 0x4);

    auto pExt = TechnoExt::ExtMap.Find(pThis);
    if (!pExt)
        return 0;

    // ── BerserkReduce 处理（在效果之前，确保无论伤害如何都执行）──
    if (pArgs->WH)
    {
        auto const pWHExt = WarheadTypeExt::ExtMap.Find(pArgs->WH);
        const int reduceBy = pWHExt->BerserkReduce.Get();

        if (reduceBy != 0 && pThis->Berzerk)
        {
            // 减少混乱时间（正值=减少，负值=延长）
            if (pThis->BerzerkDurationLeft > static_cast<DWORD>(reduceBy))
            {
                pThis->BerzerkDurationLeft -= reduceBy;
                Debug::Log("[BerserkReduce] %s berserk reduced by %d, remaining: %d\n",
                    pThis->GetTechnoType()->ID, reduceBy, pThis->BerzerkDurationLeft);
            }
            else
            {
                // 减到0以下 -> 立即清除混乱
                Debug::Log("[BerserkReduce] %s berserk cleared!\n",
                    pThis->GetTechnoType()->ID);

                pThis->Berzerk = false;
                pThis->BerzerkDurationLeft = 0;

                // 清除所有目标点、停止移动、切到警戒，防止乱跑
                pThis->SetTarget(nullptr);
                pThis->ArchiveTarget = nullptr;
                pThis->ForceMission(Mission::Guard);
                if (auto pFoot = abstract_cast<FootClass*>(pThis))
                {
                    pFoot->Locomotor->Stop_Moving();
                    pFoot->Destination = nullptr;
                    pFoot->LastDestination = nullptr;
                    pFoot->MegaDestination = nullptr;
                    pFoot->MegaTarget = nullptr;
                    pFoot->ClearNavigationList();
                    pFoot->AbortMotion();
                }
            }
        }
    }

    // ── 构造 DamageEvent 并派发给所有 IEffect ──
    DamageEvent evt(pArgs);

    for (auto& pEffect : pExt->Effects)
    {
        if (pEffect)
        {
            pEffect->OnReceiveDamage(evt);
        }
    }

    return 0;
}
