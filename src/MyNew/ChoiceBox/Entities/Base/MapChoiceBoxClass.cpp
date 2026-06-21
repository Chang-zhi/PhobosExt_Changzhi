#include "MapChoiceBoxClass.h"

#include <MyNew/ChoiceBox/Types/ChoiceBoxTypeClass.h>
#include <MyNew/ChoiceBox/Entities/Derived/WaypointChoiceBoxClass.h>
#include <MyNew/ChoiceBox/Entities/Derived/ScreenChoiceBoxClass.h>

#include <StringTable.h>
#include <Surface.h>
#include <Drawing.h>
#include <TacticalClass.h>
#include <WWMouseClass.h>

#include <Utilities/Stream.h>
#include <Utilities/Debug.h>

#include <cstring>
#include <algorithm>
#include <memory>

#include <Windows.h>

// ========== 布局常量 ==========
constexpr static const int PADDINGX = 8;          // 水平内边距
constexpr static const int PADDINGY = 8;          // 垂直内边距
constexpr static const int LINE_HEIGHT = 18;       // 每行文字高度
constexpr static const int BUTTON_PADDINGX = 11;   // 按钮文字水平内边距
constexpr static const int BUTTON_PADDINGY = 4;    // 按钮文字垂直内边距
constexpr static const int BUTTON_SPACING = 10;     // 按钮之间的间距
constexpr static const int SECTION_SPACING = 4;    // 不同区块（标题/描述/按钮）之间的间距
constexpr static const int BOTTOM_SAFE_HEIGHT = 0; // 底部安全区域

// 全局数组
std::vector<std::shared_ptr<MapChoiceBoxClass>> MapChoiceBoxClass::Array;

// ========== 构造 / 析构 ==========

MapChoiceBoxClass::~MapChoiceBoxClass() = default;

MapChoiceBoxClass::MapChoiceBoxClass(int id, const char* label, const ChoiceBoxTypeClass* pType)
	: ID(id)
	, Label(label ? label : "")
	, Type(pType)
{
	if (pType)
	{
		// 记录类型索引，用于序列化后重建指针
		for (size_t i = 0; i < ChoiceBoxTypeClass::Array.size(); ++i)
		{
			if (ChoiceBoxTypeClass::Array[i].get() == pType)
			{
				this->TypeIndex = static_cast<int>(i);
				break;
			}
		}

		if (pType->Duration >= 0)
			this->RemainingFrames = pType->Duration;
	}
}

// ========== 获取 CSF 文本工具函数 ==========

/**
 * @brief 从 CSF 标签名获取本地化文本
 *
 * 与 TextBox 相同的策略：先用 StringTable::TryFetchString 查找 CSF 翻译，
 * 若找不到（CSF 标签不存在或为空），则将标签名本身作为显示文本。
 *
 * @param csfLabel CSF 标签名（如 "MSG:MyText"）
 * @return 宽字符串文本
 */
static std::wstring GetCSFText(const char* csfLabel)
{
	if (!csfLabel || csfLabel[0] == '\0')
		return L"";

	const wchar_t* textPtr = StringTable::TryFetchString(csfLabel);

	if (textPtr && wcslen(textPtr) > 0)
		return textPtr;

	// Fallback: 直接输出 CSF 标签原文（如 "MSG:Attack"）
	std::wstring fallback;
	fallback.reserve(std::strlen(csfLabel));
	for (const char* p = csfLabel; *p; ++p)
		fallback += static_cast<wchar_t>(static_cast<unsigned char>(*p));
	return fallback;
}

// ========== 自动换行（与 TextBox 相同算法） ==========

/**
 * @brief 对宽字符串进行自动换行处理
 *
 * 逐字累加测量宽度，超过 maxWidth 时换行。
 * \n 或 \r\n 强制换行，控制字符（除 \t 外）被过滤。
 *
 * @param text     输入的宽字符串
 * @param maxWidth 单行最大像素宽度
 * @return          换行后的字符串数组
 */
static std::vector<std::wstring> WrapText(const wchar_t* text, int maxWidth)
{
	if (!text || wcslen(text) == 0 || maxWidth <= 0)
		return {};

	std::vector<std::wstring> lines;
	std::wstring wStr(text);
	std::wstring currentLine;

	for (size_t i = 0; i < wStr.length(); ++i)
	{
		wchar_t ch = wStr[i];

		if (ch == L'\n' || ch == L'\r')
		{
			if (!currentLine.empty())
			{
				lines.push_back(currentLine);
				currentLine.clear();
			}
			if (ch == L'\r' && i + 1 < wStr.length() && wStr[i + 1] == L'\n')
				++i;
			continue;
		}

		if (ch < 0x20 && ch != L'\t')
			continue;

		std::wstring testLine = currentLine + ch;
		RectangleStruct dims = Drawing::GetTextDimensions(testLine.c_str(), { 0, 0 }, 0, 2, 0);

		if (dims.Width > maxWidth)
		{
			if (currentLine.empty())
			{
				lines.push_back(testLine);
			}
			else
			{
				lines.push_back(currentLine);
				currentLine = ch;
			}
		}
		else
		{
			currentLine = testLine;
		}
	}

	if (!currentLine.empty())
		lines.push_back(currentLine);

	return lines;
}

// ========== 交互 ==========

void MapChoiceBoxClass::ResetChoice()
{
	this->ClickedIndex = -1;
}

/**
 * @brief 检测鼠标左键点击
 *
 * 使用 Windows GetAsyncKeyState 检测左键按下状态。
 * 遍历缓存的按钮区域，检查鼠标位置是否落在某个按钮上。
 * 通过静态变量跟踪上一帧的左键状态，检测"按下"事件而非"按住"状态。
 */
bool MapChoiceBoxClass::CheckMouseClick()
{
	if (this->ClickedIndex >= 0)
		return false; // 已经选过了，不再响应

	// 用静态变量跟踪左键的上一帧状态，检测按下瞬间
	static bool s_prevLeftDown = false;
	bool currentLeftDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	bool justPressed = currentLeftDown && !s_prevLeftDown;
	s_prevLeftDown = currentLeftDown;

	if (!justPressed)
		return false;

	const auto pMouse = WWMouseClass::Instance;
	if (!pMouse)
		return false;

	const Point2D& mousePos = pMouse->XY1;

	for (const auto& btn : this->m_buttonRects)
	{
		if (mousePos.X >= btn.Rect.X &&
			mousePos.X <= btn.Rect.X + btn.Rect.Width &&
			mousePos.Y >= btn.Rect.Y &&
			mousePos.Y <= btn.Rect.Y + btn.Rect.Height)
		{
			this->ClickedIndex = btn.Index;
			Debug::Log(L"点击了, 点击的索引是\"%d\"\n", btn.Index);
			return true;
		}
	}

	return false;
}

// ========== 绘制 ==========

void MapChoiceBoxClass::DrawAt(Point2D centerPos)
{
	if (!DSurface::Composite || !TacticalClass::Instance)
		return;

	if (!this->Type)
		return;

	const auto& type = *this->Type;

	// 计算各区块内容
	const auto& csfTitle = type.Title.Get();
	const auto& csfDesc = type.Description.Get();
	bool hasTitle = !csfTitle.empty();
	bool hasDesc = !csfDesc.empty();
	std::wstring titleText = hasTitle ? std::wstring(csfTitle) : std::wstring();
	std::wstring descText = hasDesc ? std::wstring(csfDesc) : std::wstring();
	int btnCount = type.Button_Count;

	if (btnCount <= 0)
		return;

	// 测量文本尺寸（支持自动换行）
	int maxW = type.MaxWidth;
	bool useWrapping = (maxW > 0);

	std::vector<std::wstring> titleLines, descLines;
	int titleWidth = 0, titleHeight = 0;
	if (hasTitle)
	{
		if (useWrapping)
		{
			titleLines = WrapText(titleText.c_str(), maxW);
			if (titleLines.empty()) { hasTitle = false; titleText.clear(); }
			else
			{
				titleWidth = maxW;
				titleHeight = static_cast<int>(titleLines.size()) * LINE_HEIGHT;
			}
		}
		else
		{
			titleLines = { titleText };
			RectangleStruct dims = Drawing::GetTextDimensions(titleText.c_str(), { 0, 0 }, 0, 2, 0);
			titleWidth = dims.Width;
			titleHeight = LINE_HEIGHT;
		}
	}

	int descWidth = 0, descHeight = 0;
	if (hasDesc)
	{
		if (useWrapping)
		{
			descLines = WrapText(descText.c_str(), maxW);
			if (descLines.empty()) { hasDesc = false; descText.clear(); }
			else
			{
				descWidth = maxW;
				descHeight = static_cast<int>(descLines.size()) * LINE_HEIGHT;
			}
		}
		else
		{
			descLines = { descText };
			RectangleStruct dims = Drawing::GetTextDimensions(descText.c_str(), { 0, 0 }, 0, 2, 0);
			descWidth = dims.Width;
			descHeight = LINE_HEIGHT;
		}
	}

	// 计算按钮尺寸
	int btnTextMaxWidth = 0;
	std::vector<std::wstring> btnWTexts;
	for (int i = 0; i < btnCount; ++i)
	{
		std::wstring wtext = GetCSFText(
			(i < static_cast<int>(type.ButtonTexts.size())) ? type.ButtonTexts[i].c_str() : "");
		if (wtext.empty())
		{
			char fallback[32];
			std::sprintf(fallback, "Btn%d", i);
			for (const char* p = fallback; *p; ++p)
				wtext += static_cast<wchar_t>(static_cast<unsigned char>(*p));
		}
		btnWTexts.push_back(wtext);

		RectangleStruct dims = Drawing::GetTextDimensions(wtext.c_str(), { 0, 0 }, 0, 2, 0);
		if (dims.Width > btnTextMaxWidth)
			btnTextMaxWidth = dims.Width;
	}

	int btnWidth = btnTextMaxWidth + (BUTTON_PADDINGX * 2);
	int btnHeight = LINE_HEIGHT + (BUTTON_PADDINGY * 2);

	// 判断布局方向
	bool isVertical = (type.Button_Layout != 0);

	// 计算整体背景框宽度
	int btnContentW = isVertical ? btnWidth : ((btnWidth * btnCount) + (BUTTON_SPACING * (btnCount - 1)));
	int maxTextWidth = std::max({ btnContentW, titleWidth, descWidth });
	int bgWidth;
	if (useWrapping)
	{
		// 固定宽度模式：背景框 = MaxWidth + 内边距
		bgWidth = maxW + (PADDINGX * 2);
	}
	else
	{
		// 自动宽度模式：取最宽内容
		bgWidth = maxTextWidth + (PADDINGX * 2);
	}
	int bgHeight = PADDINGY * 2;

	if (hasTitle)
		bgHeight += titleHeight + SECTION_SPACING;
	if (hasDesc)
		bgHeight += descHeight + SECTION_SPACING;

	int buttonsStartY = PADDINGY;
	if (hasTitle)
		buttonsStartY += titleHeight + SECTION_SPACING;
	if (hasDesc)
		buttonsStartY += descHeight + SECTION_SPACING;

	// 按钮区域高度
	if (isVertical)
		bgHeight += (btnHeight * btnCount) + (BUTTON_SPACING * (btnCount - 1));
	else
		bgHeight += btnHeight;

	// 背景框左上角（居中定位）
	Point2D topLeft = {
		centerPos.X - (bgWidth / 2),
		centerPos.Y - (bgHeight / 2)
	};

	// ===== 屏幕钳制（仅 ScreenChoiceBoxClass） =====
	if (this->ClampToScreen())
	{
		int viewW = DSurface::ViewBounds.Width;
		int viewH = DSurface::ViewBounds.Height;
		constexpr int CLAMP_MARGIN = 4;

		if (topLeft.X < CLAMP_MARGIN)
			topLeft.X = CLAMP_MARGIN;
		if (topLeft.Y < CLAMP_MARGIN)
			topLeft.Y = CLAMP_MARGIN;
		if (topLeft.X + bgWidth > viewW - CLAMP_MARGIN)
			topLeft.X = viewW - CLAMP_MARGIN - bgWidth;
		if (topLeft.Y + bgHeight > viewH - CLAMP_MARGIN)
			topLeft.Y = viewH - CLAMP_MARGIN - bgHeight;
	}

	// ===== 底部裁剪 =====
	int viewHeight = DSurface::ViewBounds.Height;
	int clipBottomY = viewHeight - BOTTOM_SAFE_HEIGHT;

	if (topLeft.Y >= clipBottomY)
		return;

	int drawHeight = bgHeight;
	bool isClipped = false;
	if (topLeft.Y + bgHeight > clipBottomY)
	{
		drawHeight = clipBottomY - topLeft.Y;
		isClipped = true;
		if (drawHeight < LINE_HEIGHT)
			return;
	}

	if (topLeft.X + bgWidth < 0 || topLeft.Y + drawHeight < 0)
		return;

	RectangleStruct bgRect = { topLeft.X, topLeft.Y, bgWidth, drawHeight };

	// ===== 获取鼠标位置（用于按钮悬停高亮） =====
	Point2D mousePos = WWMouseClass::Instance->XY1;

	// ===== 绘制半透明黑色背景 =====
	ColorStruct bgColor = { 0, 0, 0 };
	DSurface::Composite->FillRectTrans(&bgRect, &bgColor, type.BackgroundOpacity);

	// 颜色整数值
	int colorInt = Drawing::RGB_To_Int(ColorStruct(
		static_cast<BYTE>(type.ColorR),
		static_cast<BYTE>(type.ColorG),
		static_cast<BYTE>(type.ColorB)));

	Point2D p1, p2;

	// ===== 绘制边框 =====
	// 上边
	p1 = { topLeft.X, topLeft.Y };
	p2 = { topLeft.X + bgWidth - 1, topLeft.Y };
	DSurface::Composite->DrawLine(&p1, &p2, colorInt);

	// 左边
	p1 = { topLeft.X, topLeft.Y };
	p2 = { topLeft.X, topLeft.Y + drawHeight - 1 };
	DSurface::Composite->DrawLine(&p1, &p2, colorInt);

	// 右边
	p1 = { topLeft.X + bgWidth - 1, topLeft.Y };
	p2 = { topLeft.X + bgWidth - 1, topLeft.Y + drawHeight - 1 };
	DSurface::Composite->DrawLine(&p1, &p2, colorInt);

	// 底边（裁剪时跳过）
	if (!isClipped)
	{
		p1 = { topLeft.X, topLeft.Y + bgHeight - 1 };
		p2 = { topLeft.X + bgWidth - 1, topLeft.Y + bgHeight - 1 };
		DSurface::Composite->DrawLine(&p1, &p2, colorInt);
	}

	RectangleStruct bounds = DSurface::ViewBounds;

	// ===== 绘制标题（支持多行，可选居中） =====
	int currentY = topLeft.Y + PADDINGY;
	if (hasTitle)
	{
		for (const std::wstring& line : titleLines)
		{
			int textX = topLeft.X + PADDINGX;
			if (type.Title_Center)
			{
				RectangleStruct lineDims = Drawing::GetTextDimensions(line.c_str(), { 0, 0 }, 0, 2, 0);
				int lineW = (lineDims.Width > 0) ? lineDims.Width : titleWidth;
				textX = topLeft.X + (bgWidth / 2) - (lineW / 2);
			}
			Point2D textPos = { textX, currentY };
			DSurface::Composite->DrawText(
				line.c_str(),
				&bounds,
				&textPos,
				colorInt,
				0,
				TextPrintType::Metal12 | TextPrintType::BrightColor);
			currentY += LINE_HEIGHT;
		}
		currentY += SECTION_SPACING;
	}

	// ===== 绘制描述（支持多行） =====
	if (hasDesc)
	{
		for (const auto& line : descLines)
		{
			Point2D textPos = { topLeft.X + PADDINGX, currentY };
			DSurface::Composite->DrawText(
				line.c_str(),
				&bounds,
				&textPos,
				colorInt,
				0,
				TextPrintType::Metal12 | TextPrintType::BrightColor);
			currentY += LINE_HEIGHT;
		}
		currentY += SECTION_SPACING;
	}

	// ===== 更新按钮区域缓存 =====
	const_cast<MapChoiceBoxClass*>(this)->UpdateButtonRects(
		topLeft, bgWidth, topLeft.Y + buttonsStartY, btnHeight);

	// ===== 绘制按钮 =====
	int btnStartX = isVertical
		? topLeft.X + (bgWidth - btnWidth) / 2
		: topLeft.X + (bgWidth - btnContentW) / 2;

	for (int i = 0; i < btnCount; ++i)
	{
		int btnX = isVertical ? btnStartX : (btnStartX + (i * (btnWidth + BUTTON_SPACING)));
		int btnY = isVertical
			? (topLeft.Y + buttonsStartY + (i * (btnHeight + BUTTON_SPACING)))
			: (topLeft.Y + buttonsStartY);

		// 检测鼠标是否悬停在此按钮上
		bool isHover = (mousePos.X >= btnX && mousePos.X <= btnX + btnWidth &&
			mousePos.Y >= btnY && mousePos.Y <= btnY + btnHeight);

		// 检测是否已被选中
		bool isChosen = (this->ClickedIndex == i);

		// 按钮背景色（黑色，不透明度与 BackgroundOpacity 联动）
		ColorStruct btnColor = { 0, 0, 0 };
		int btnOpacity = isHover
			? std::clamp(type.BackgroundOpacity * 3 / 4, 0, 100)
			: std::clamp(type.BackgroundOpacity / 2, 0, 100);

		RectangleStruct btnRect = { btnX, btnY, btnWidth, btnHeight };
		DSurface::Composite->FillRectTrans(&btnRect, &btnColor, btnOpacity);

		// 按钮边框（选中或悬停时加亮）
		int btnBorderColor = colorInt;
		if (isChosen)
		{
			// 选中时用红色边框
			btnBorderColor = Drawing::RGB_To_Int(ColorStruct(255, 0, 0));
		}
		else if (isHover)
		{
			// 悬停时加亮（大幅提亮使变化更明显）
			ColorStruct brightColor = {
				static_cast<BYTE>(std::min(255, type.ColorR + 100)),
				static_cast<BYTE>(std::min(255, type.ColorG + 100)),
				static_cast<BYTE>(std::min(255, type.ColorB + 100))
			};
			btnBorderColor = Drawing::RGB_To_Int(brightColor);
		}

		// 上边
		p1 = { btnX, btnY };
		p2 = { btnX + btnWidth - 1, btnY };
		DSurface::Composite->DrawLine(&p1, &p2, btnBorderColor);

		// 左边
		p1 = { btnX, btnY };
		p2 = { btnX, btnY + btnHeight - 1 };
		DSurface::Composite->DrawLine(&p1, &p2, btnBorderColor);

		// 右边
		p1 = { btnX + btnWidth - 1, btnY };
		p2 = { btnX + btnWidth - 1, btnY + btnHeight - 1 };
		DSurface::Composite->DrawLine(&p1, &p2, btnBorderColor);

		// 底边
		p1 = { btnX, btnY + btnHeight - 1 };
		p2 = { btnX + btnWidth - 1, btnY + btnHeight - 1 };
		DSurface::Composite->DrawLine(&p1, &p2, btnBorderColor);

		// 按钮文字（居中）
		RectangleStruct txtDims = Drawing::GetTextDimensions(btnWTexts[i].c_str(), { 0, 0 }, 0, 2, 0);
		Point2D textPos = {
			btnX + (btnWidth - txtDims.Width) / 2,
			btnY + (btnHeight - LINE_HEIGHT) / 2
		};
		DSurface::Composite->DrawText(
			btnWTexts[i].c_str(),
			&bounds,
			&textPos,
			colorInt,
			0,
			TextPrintType::Metal12 | TextPrintType::BrightColor);
	}
}

void MapChoiceBoxClass::UpdateButtonRects(Point2D topLeft, int bgWidth, int buttonsStartY, int buttonHeight)
{
	if (!this->Type)
		return;

	const auto& type = *this->Type;
	int btnCount = type.Button_Count;
	if (btnCount <= 0)
		return;

	// 计算按钮宽度（与 DrawAt 中一致的算法）
	int btnTextMaxWidth = 0;
	for (int i = 0; i < btnCount; ++i)
	{
		std::wstring wtext = GetCSFText(
			(i < static_cast<int>(type.ButtonTexts.size())) ? type.ButtonTexts[i].c_str() : "");
		if (wtext.empty())
			wtext = L"Btn";

		RectangleStruct dims = Drawing::GetTextDimensions(wtext.c_str(), { 0, 0 }, 0, 2, 0);
		if (dims.Width > btnTextMaxWidth)
			btnTextMaxWidth = dims.Width;
	}
	int btnWidth = btnTextMaxWidth + (BUTTON_PADDINGX * 2);

	bool isVertical = (type.Button_Layout != 0);

	int btnStartX;
	if (isVertical)
	{
		btnStartX = topLeft.X + (bgWidth - btnWidth) / 2;
	}
	else
	{
		int contentWidth = (btnWidth * btnCount) + (BUTTON_SPACING * (btnCount - 1));
		btnStartX = topLeft.X + (bgWidth - contentWidth) / 2;
	}

	this->m_buttonRects.clear();
	for (int i = 0; i < btnCount; ++i)
	{
		int btnX = isVertical ? btnStartX : (btnStartX + (i * (btnWidth + BUTTON_SPACING)));
		int btnY = isVertical
			? (buttonsStartY + (i * (buttonHeight + BUTTON_SPACING)))
			: buttonsStartY;
		this->m_buttonRects.push_back({
			i,
			{ btnX, btnY, btnWidth, buttonHeight }
		});
	}
}

// ========== 序列化 ==========

template <typename T>
bool MapChoiceBoxClass::Serialize(T& Stm)
{
	return Stm
		.Process(this->ID)
		.Process(this->Label)
		.Process(this->TypeIndex)
		.Process(this->ClickedIndex)
		.Process(this->RemainingFrames)
		.Process(this->ClickExpireCounter)
		.Process(this->IsExpired)
		.Success();
}

bool MapChoiceBoxClass::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	if (!Serialize(Stm))
		return false;

	// 从 TypeIndex 重建 Type 指针
	if (this->TypeIndex >= 0
		&& static_cast<size_t>(this->TypeIndex) < ChoiceBoxTypeClass::Array.size())
	{
		this->Type = ChoiceBoxTypeClass::Array[this->TypeIndex].get();
	}
	else
	{
		this->Type = nullptr;
	}

	return true;
}

bool MapChoiceBoxClass::Save(PhobosStreamWriter& Stm) const
{
	return const_cast<MapChoiceBoxClass*>(this)->Serialize(Stm);
}

// ========== 全局管理 ==========

void MapChoiceBoxClass::Clear()
{
	WaypointChoiceBoxClass::Array.clear();
	ScreenChoiceBoxClass::Array.clear();
	Array.clear();
}

void MapChoiceBoxClass::ClearAll()
{
	Clear();
}

MapChoiceBoxClass* MapChoiceBoxClass::FindByID(int id)
{
	if (id < 0)
		return nullptr;

	for (auto& pBox : Array)
	{
		if (pBox && pBox->ID == id)
			return pBox.get();
	}

	return nullptr;
}

// ========== 内部绘制逻辑（共享给 DrawAll/DrawWaypoint/DrawScreen） ==========

/**
 * @brief 绘制一组选择框并处理点击和过期
 * @tparam T 派生类类型（WaypointChoiceBoxClass 或 ScreenChoiceBoxClass）
 * @param boxes 要绘制和更新的实例列表
 */
template <typename T>
static void DrawChoiceBoxList(std::vector<std::shared_ptr<T>>& boxes)
{
	// 绘制并检测点击
	bool anyClicked = false;
	for (auto& ptr : boxes)
	{
		if (!ptr || ptr->IsExpired || !ptr->CanDraw())
			continue;

		// 如果已点击且 Duration 已耗尽，不绘制但仍保留对象（给 TEvent 检测窗口）
		if (ptr->ClickedIndex >= 0 && ptr->RemainingFrames <= 0)
			continue;

		Point2D drawPos;
		if (!ptr->GetDrawPosition(drawPos))
			continue;

		ptr->DrawAt(drawPos);

		if (ptr->CheckMouseClick())
		{
			anyClicked = true;
			// 点击后强制保留 5 帧（给 TEvent 检测窗口）
			ptr->ClickExpireCounter = 5;
		}
	}

	// 处理点击后消失倒计时
	for (auto& ptr : boxes)
	{
		if (ptr && !ptr->IsExpired && ptr->ClickExpireCounter >= 0)
		{
			if (--ptr->ClickExpireCounter <= 0)
				ptr->IsExpired = true;
		}
	}

	// 处理 Duration 自动移除
	for (auto& ptr : boxes)
	{
		if (ptr && !ptr->IsExpired && ptr->RemainingFrames >= 0)
		{
			if (--ptr->RemainingFrames <= 0)
			{
				// 如果已点击过，不立即过期（由 ClickExpireCounter 控制清除）
				if (ptr->ClickedIndex >= 0)
				{
					ptr->RemainingFrames = 0; // 保持为 0，绘制时会跳过
				}
				else
				{
					ptr->IsExpired = true;
					ptr->ClickedIndex = -2;
				}
			}
		}
	}
}

void MapChoiceBoxClass::DrawAll()
{
	DrawWaypoint();
	DrawScreen();
}

void MapChoiceBoxClass::DrawWaypoint()
{
	DrawChoiceBoxList(WaypointChoiceBoxClass::Array);
}

void MapChoiceBoxClass::DrawScreen()
{
	DrawChoiceBoxList(ScreenChoiceBoxClass::Array);
}

bool MapChoiceBoxClass::CanDraw() const
{
	return true;
}

bool MapChoiceBoxClass::ClampToScreen() const
{
	return false;
}

// ========== 全局存档/读档（遵循 TextBox 序列化模式） ==========

bool MapChoiceBoxClass::SaveGlobals(PhobosStreamWriter& Stm)
{
	Stm.Save(Array.size());
	for (auto const& item : Array)
	{
		Stm.Save(item.get());                              // 保存旧指针地址（占位）
		PhobosFixedString<64> marker(item->GetTypeMarker());// 保存类型标记
		Stm.Save(marker);
		item->Save(Stm);                                   // 保存实例数据
	}
	return true;
}

bool MapChoiceBoxClass::LoadGlobals(PhobosStreamReader& Stm)
{
	// 清除当前所有已存在的实例
	Clear();

	size_t Count = 0;
	if (!Stm.Load(Count))
		return false;

	Array.reserve(Count);
	for (size_t i = 0; i < Count; ++i)
	{
		void* oldPtr = nullptr;
		PhobosFixedString<64> typeMarker;

		if (!Stm.Load(oldPtr) || !Stm.Load(typeMarker))
			return false;

		if (std::strcmp(typeMarker.data(), "WaypointChoiceBoxClass") == 0)
		{
			auto newObj = std::make_shared<WaypointChoiceBoxClass>();
			if (!newObj->Load(Stm, false))
				return false;
			WaypointChoiceBoxClass::Array.push_back(newObj);
			Array.push_back(std::move(newObj));
		}
		else if (std::strcmp(typeMarker.data(), "ScreenChoiceBoxClass") == 0)
		{
			auto newObj = std::make_shared<ScreenChoiceBoxClass>();
			if (!newObj->Load(Stm, false))
				return false;
			ScreenChoiceBoxClass::Array.push_back(newObj);
			Array.push_back(std::move(newObj));
		}
		else
		{
			Debug::Log("[MapChoiceBoxClass] Warning: unknown type marker \"%s\"!\n",
				typeMarker.data());
			return false;
		}
	}

	return true;
}
