/**
 * @file Hooks.MapTextBoxClass.cpp
 * @brief 文本框系统的游戏钩子入口
 *
 * 通过 hook 游戏主循环的战术视图绘制函数，
 * 每帧自动调用 MapTextBoxClass::DrawAll() 渲染所有活跃文本框。
 */

#include "Base/MapTextBoxClass.h"

#include <Syringe.h>
#include <Helpers/Macro.h>

/**
 * @brief Hook: TacticalClass::Draw 渲染流程
 *
 * 地址: 0x6D4684（在 Renderer->Draw 阶段）
 * 在原函数绘制完战术视图后插入文本框绘制。
 * 在黑幕上绘制
 */
DEFINE_HOOK(0x6D4684, TacticalClass_Draw_MapTextBoxClass, 0x6)
{
	MapTextBoxClass::DrawAll();
	return 0;
}
