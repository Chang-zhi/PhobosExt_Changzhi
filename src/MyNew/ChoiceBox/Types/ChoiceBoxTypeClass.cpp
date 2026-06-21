#include "ChoiceBoxTypeClass.h"

#include <Phobos.h>
#include <CCINIClass.h>

#include <Utilities/INIParser.h>
#include <Utilities/Stream.h>
#include <Utilities/TemplateDef.h>

#include <cstdio>

/**
 * @brief 指定 ChoiceBoxType 在 INI 中的主段名
 *
 * Enumerable 模板通过此函数确定该类型从哪个 INI 段中读取。
 * 对应 rulesmd.ini 中的 [ChoiceBoxTypes] 段。
 */
template<>
const char* Enumerable<ChoiceBoxTypeClass>::GetMainSection()
{
	return "ChoiceBoxTypes";
}

// ========== INI 加载 ==========

/**
 * @brief 从 INI 文件中加载选择框类型配置
 *
 * 从以类型名为段名的节中读取以下字段：
 *   - Title               : 标题文本（CSF 标签）
 *   - Description         : 描述文本（CSF 标签）
 *   - Button.Count        : 按钮数量
 *   - ButtonText1~N       : 各按钮文字
 *   - BackgroundOpacity   : 背景不透明度
 *   - Duration            : 自动移除帧数
 *   - Color=R,G,B         : 文本和边框颜色
 */
void ChoiceBoxTypeClass::LoadFromINI(CCINIClass* pINI)
{
	const char* section = this->Name;

	if (!pINI->GetSection(section))
		return;

	INI_EX exINI(pINI);

	this->Title.Read(exINI, section, "Title");
	this->Title_Center.Read(exINI, section, "Title.Center");
	this->Description.Read(exINI, section, "Description");
	this->MaxWidth.Read(exINI, section, "MaxWidth");
	if (this->MaxWidth <= 0)
		this->MaxWidth = 250;
	this->BackgroundOpacity.Read(exINI, section, "BackgroundOpacity");
	this->Duration.Read(exINI, section, "Duration");

	// Button.Count - INI 用点，代码变量用下划线
	this->Button_Count.Read(exINI, section, "Button.Count");

	// Button.Layout - 0=横向, 1=纵向
	this->Button_Layout.Read(exINI, section, "Button.Layout");

	// DisappearDelay - 点击后消失延迟帧数
	this->DisappearDelay.Read(exINI, section, "DisappearDelay");

	// 逐个读取 Button.Text1 ~ Button.TextN
	this->ButtonTexts.clear();
	for (int i = 1; i <= this->Button_Count; ++i)
	{
		char key[32];
		std::sprintf(key, "Button.Text%d", i);

		if (pINI->ReadString(section, key, "", Phobos::readBuffer))
		{
			this->ButtonTexts.push_back(Phobos::readBuffer);
		}
	}

	// Color 格式：Color=255,215,0  （RGB 逗号分隔）
	if (pINI->ReadString(section, "Color", "", Phobos::readBuffer))
	{
		const char* pColor = Phobos::readBuffer;
		int r = 255, g = 215, b = 0;
		if (std::sscanf(pColor, "%d,%d,%d", &r, &g, &b) >= 3)
		{
			this->ColorR = std::clamp(r, 0, 255);
			this->ColorG = std::clamp(g, 0, 255);
			this->ColorB = std::clamp(b, 0, 255);
		}
	}
}

// ========== 序列化模板 ==========

template <typename T>
void ChoiceBoxTypeClass::Serialize(T& Stm)
{
	Stm
		.Process(this->Title)
		.Process(this->Title_Center)
		.Process(this->Description)
		.Process(this->Button_Count)
		.Process(this->Button_Layout)
		.Process(this->ButtonTexts)
		.Process(this->MaxWidth)
		.Process(this->BackgroundOpacity)
		.Process(this->ColorR)
		.Process(this->ColorG)
		.Process(this->ColorB)
		.Process(this->Duration)
		.Process(this->DisappearDelay)
		;
}

void ChoiceBoxTypeClass::LoadFromStream(PhobosStreamReader& Stm)
{
	this->Serialize(Stm);
}

void ChoiceBoxTypeClass::SaveToStream(PhobosStreamWriter& Stm)
{
	this->Serialize(Stm);
}
