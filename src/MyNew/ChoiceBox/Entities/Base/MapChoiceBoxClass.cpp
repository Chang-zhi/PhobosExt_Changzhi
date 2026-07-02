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
#include <cstdio>

// ========== 布局常量 ==========
constexpr static const int PADDINGX = 8;                // 背景框水平内边距
constexpr static const int PADDINGY = 8;                // 背景框垂直内边距
constexpr static const int LINE_HEIGHT = 18;            // 每行文字高度
constexpr static const int BUTTON_PADDINGX = 11;        // 按钮内文字水平内边距
constexpr static const int BUTTON_PADDINGY = 4;         // 按钮内文字垂直内边距
constexpr static const int BUTTON_SPACING = 10;         // 按钮之间的间距
constexpr static const int SECTION_SPACING = 4;         // 标题/描述/按钮区块之间的间距
constexpr static const int BOTTOM_SAFE_HEIGHT = 0;      // 底部安全区域（保留）
constexpr static const int CLICK_EXPIRE_FRAMES = 5;     // 隐藏期帧数（Duration 耗尽后保留供 TEvent 检测）

// ========== 按钮布局元数据 ==========
struct MapChoiceBoxClass::BtnLayoutItem
{
	int Index;                               // 按钮索引
	std::wstring Text;                       // 解析后的宽字符串文本
	std::vector<std::wstring> TextLines;     // 按钮内换行后的各行文字
	int Width;                               // 按钮宽度（固定或自动）
	int Height;                              // 按钮高度（固定或自动撑高）
};

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

	// 验证 Type 指针是否仍在 Array 中（防止悬空指针）
	{
		bool typeValid = false;
		for (auto& tp : ChoiceBoxTypeClass::Array)
		{
			if (tp.get() == this->Type)
			{
				typeValid = true;
				break;
			}
		}
		if (!typeValid)
		{
			this->Type = nullptr;
			return;
		}
	}

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

	// ===== 测量按钮尺寸 =====
	int btnFixedW = type.Button_Width;   // >0 固定宽度（文字自动换行适配）
	int btnFixedH = type.Button_Height;  // >0 固定高度（文字超出截断）
	bool isVertical = (type.Button_Layout != 0);

	// 存储每个按钮的元数据
	std::vector<BtnLayoutItem> btnItems;
	btnItems.reserve(btnCount);

	// 刷新 maxW/useWrapping（用于后续背景框宽度计算）
	maxW = type.MaxWidth;
	useWrapping = (maxW > 0);

	for (int i = 0; i < btnCount; ++i)
	{
		BtnLayoutItem item;
		item.Index = i;

		// 获取按钮文字（CSF 解析，空时回退到 "BtnN"）
		std::wstring wtext = GetCSFText(
			(i < static_cast<int>(type.Buttons.size())) ? type.Buttons[i].Text.c_str() : "");
		if (wtext.empty())
		{
			char fallback[32];
			std::sprintf(fallback, "Btn%d", i);
			for (const char* p = fallback; *p; ++p)
				wtext += static_cast<wchar_t>(static_cast<unsigned char>(*p));
		}
		item.Text = wtext;

		// 按钮宽度：固定宽度或自动取文本宽度
		if (btnFixedW > 0)
		{
			item.Width = btnFixedW;
		}
		else
		{
			RectangleStruct dims = Drawing::GetTextDimensions(wtext.c_str(), { 0, 0 }, 0, 2, 0);
			item.Width = dims.Width + BUTTON_PADDINGX * 2;
		}

		// 按钮内文本自动换行
		int textMaxW = item.Width - BUTTON_PADDINGX * 2;
		if (textMaxW > 0)
		{
			item.TextLines = WrapText(wtext.c_str(), textMaxW);
		}
		else
		{
			item.TextLines = { wtext };
		}
		if (item.TextLines.empty())
			item.TextLines = { wtext };

		// 按钮高度：固定高度或按文本行数自动撑高
		if (btnFixedH > 0)
		{
			item.Height = btnFixedH;
		}
		else
		{
			item.Height = static_cast<int>(item.TextLines.size()) * LINE_HEIGHT
				+ BUTTON_PADDINGY * 2;
		}

		btnItems.push_back(std::move(item));
	}

	// ===== 执行按钮布局 =====
	struct BtnPos { int X, Y, W, H; };
	std::vector<BtnPos> btnPositions;
	btnPositions.reserve(btnCount);

	// 根据布局方向决定背景框宽度
	int btnContentMaxW = 0;
	int btnTotalH = 0;

	if (isVertical)
	{
		// 纵向：所有按钮居中排成一列
		int maxBtnW = 0;
		for (auto& item : btnItems)
			if (item.Width > maxBtnW) maxBtnW = item.Width;
		btnContentMaxW = maxBtnW;

		for (int i = 0; i < btnCount; ++i)
		{
			btnPositions.push_back({ 0, btnTotalH, btnItems[i].Width, btnItems[i].Height });
			btnTotalH += btnItems[i].Height;
			if (i < btnCount - 1)
				btnTotalH += BUTTON_SPACING;
		}
	}
	else
	{
		// 横向：先确定 bgWidth，再折行布局
		// 先计算单行总宽度（假设不折行），同时填充 btnPositions
		int curX = 0, curY = 0, rowH = 0;
		for (int i = 0; i < btnCount; ++i)
		{
			const auto& item = btnItems[i];
			btnPositions.push_back({ curX, curY, item.Width, item.Height });
			curX += item.Width + BUTTON_SPACING;
			if (item.Height > rowH) rowH = item.Height;
		}
		btnTotalH = rowH;
		if (curX > 0)
			btnContentMaxW = curX - BUTTON_SPACING;
	}

	// ===== 确定背景框尺寸 =====
	int maxTextWidth = 0;
	{
		int titleW = 0, descW = 0;
		// 取标题/描述/按钮的最大宽度（估算）
		if (hasTitle) titleW = titleWidth;
		if (hasDesc) descW = descWidth;
		maxTextWidth = std::max({ btnContentMaxW, titleW, descW });
	}

	int bgWidth;
	if (useWrapping)
	{
		bgWidth = maxW + PADDINGX * 2;
	}
	else
	{
		bgWidth = maxTextWidth + PADDINGX * 2;
	}

	// 横向模式下，如果按钮总宽度超过背景框，重新布局折行
	if (!isVertical && btnContentMaxW > bgWidth - PADDINGX * 2)
	{
		// 用背景框宽度重新折行
		btnPositions.clear();
		int availW = bgWidth - PADDINGX * 2;
		int curX = 0, curY = 0, rowH = 0;
		int newMaxW = 0;

		for (int i = 0; i < btnCount; ++i)
		{
			const auto& item = btnItems[i];
			int itemW = (item.Width > availW) ? availW : item.Width;

			if (curX + itemW > availW && curX > 0)
			{
				if (curX > newMaxW) newMaxW = curX;
				curX = 0;
				curY += rowH + BUTTON_SPACING;
				rowH = 0;
			}
			btnPositions.push_back({ curX, curY, itemW, item.Height });
			curX += itemW + BUTTON_SPACING;
			if (item.Height > rowH) rowH = item.Height;
		}
		if (curX > 0)
		{
			if (curX - BUTTON_SPACING > newMaxW)
				newMaxW = curX - BUTTON_SPACING;
			curY += rowH;
		}
		else
		{
			curY += rowH;
		}
		btnContentMaxW = newMaxW;
		btnTotalH = curY;
		// 若背景框是自动宽度则更新
		if (!useWrapping)
			bgWidth = std::max(btnContentMaxW, maxTextWidth) + PADDINGX * 2;
	}

	int bgHeight = PADDINGY * 2;
	if (hasTitle)
		bgHeight += titleHeight + SECTION_SPACING;
	if (hasDesc)
		bgHeight += descHeight + SECTION_SPACING;
	bgHeight += btnTotalH;

	int buttonsStartY = PADDINGY;
	if (hasTitle)
		buttonsStartY += titleHeight + SECTION_SPACING;
	if (hasDesc)
		buttonsStartY += descHeight + SECTION_SPACING;

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
		topLeft, bgWidth, topLeft.Y + buttonsStartY, btnItems);

	// ===== 绘制按钮（多行布局） =====
	for (int i = 0; i < btnCount; ++i)
	{
		const auto& pos = btnPositions[i];

		// 计算按钮在屏幕上的实际位置（横向居中）
		int btnX, btnY;
		if (isVertical)
		{
			btnX = topLeft.X + (bgWidth - pos.W) / 2;
			btnY = topLeft.Y + buttonsStartY + pos.Y;
		}
		else
		{
			btnX = topLeft.X + (bgWidth - btnContentMaxW) / 2 + pos.X;
			btnY = topLeft.Y + buttonsStartY + pos.Y;
		}

		// 底部裁剪：完全在裁剪区外则跳过
		if (btnY >= topLeft.Y + drawHeight)
			continue;

		// 按钮可见高度（裁剪超出部分）
		int btnDrawH = pos.H;
		bool btnClipped = false;
		if (btnY + btnDrawH > topLeft.Y + drawHeight)
		{
			btnDrawH = topLeft.Y + drawHeight - btnY;
			btnClipped = true;
			if (btnDrawH <= 0)
				continue;
		}

		// 检测鼠标是否悬停（部分可见按钮仍可悬停高亮）
		bool isHover = (mousePos.X >= btnX && mousePos.X <= btnX + pos.W &&
			mousePos.Y >= btnY && mousePos.Y <= btnY + pos.H);

		// 检测是否已被选中
		bool isChosen = (this->ClickedIndex == i);

		// 按钮背景色（黑色半透明，悬停时偏白）
		ColorStruct btnColor = isHover ? ColorStruct{ 255, 255, 255 } : ColorStruct{ 0, 0, 0 };
		int btnOpacity = isHover
			? std::clamp(type.BackgroundOpacity * 1 / 4, 0, 100)
			: std::clamp(type.BackgroundOpacity / 2, 0, 100);

		RectangleStruct btnRect = { btnX, btnY, pos.W, btnDrawH };
		DSurface::Composite->FillRectTrans(&btnRect, &btnColor, btnOpacity);

		// 按钮边框 + 文字颜色（悬停/选中时加亮）
		int btnHighlightColor = colorInt;
		if (isChosen)
		{
			btnHighlightColor = Drawing::RGB_To_Int(ColorStruct(255, 0, 0));
		}
		else if (isHover)
		{
			ColorStruct brightColor = {
				static_cast<BYTE>(std::min(255, type.ColorR + 100)),
				static_cast<BYTE>(std::min(255, type.ColorG + 100)),
				static_cast<BYTE>(std::min(255, type.ColorB + 100))
			};
			btnHighlightColor = Drawing::RGB_To_Int(brightColor);
		}

		// 四条边框（底部边框在裁剪时跳过）
		p1 = { btnX, btnY };
		p2 = { btnX + pos.W - 1, btnY };
		DSurface::Composite->DrawLine(&p1, &p2, btnHighlightColor);
		p1 = { btnX, btnY };
		p2 = { btnX, btnY + btnDrawH - 1 };
		DSurface::Composite->DrawLine(&p1, &p2, btnHighlightColor);
		p1 = { btnX + pos.W - 1, btnY };
		p2 = { btnX + pos.W - 1, btnY + btnDrawH - 1 };
		DSurface::Composite->DrawLine(&p1, &p2, btnHighlightColor);
		if (!btnClipped)
		{
			p1 = { btnX, btnY + pos.H - 1 };
			p2 = { btnX + pos.W - 1, btnY + pos.H - 1 };
			DSurface::Composite->DrawLine(&p1, &p2, btnHighlightColor);
		}

		// 按钮文字：在按钮内部居中，支持多行/截断（悬停时颜色随之加亮）
		const auto& item = btnItems[i];
		int textAreaH = pos.H - BUTTON_PADDINGY * 2;
		int textAreaW = pos.W - BUTTON_PADDINGX * 2;

		if (textAreaH > 0 && textAreaW > 0)
		{
			int totalTextH = static_cast<int>(item.TextLines.size()) * LINE_HEIGHT;
			int lineOffsetY = (textAreaH - totalTextH) / 2;
			if (lineOffsetY < 0) lineOffsetY = 0;

			for (size_t li = 0; li < item.TextLines.size(); ++li)
			{
				int lineY = btnY + BUTTON_PADDINGY + lineOffsetY + static_cast<int>(li) * LINE_HEIGHT;

				if (btnFixedH > 0 && lineY + LINE_HEIGHT > btnY + pos.H)
					break;

				// 当前行超出裁剪区则停止
				if (lineY + LINE_HEIGHT > topLeft.Y + drawHeight)
					break;

				RectangleStruct txtDims = Drawing::GetTextDimensions(
					item.TextLines[li].c_str(), { 0, 0 }, 0, 2, 0);

				Point2D textPos = {
					btnX + (pos.W - txtDims.Width) / 2,
					lineY
				};
				DSurface::Composite->DrawText(
					item.TextLines[li].c_str(),
					&bounds,
					&textPos,
					btnHighlightColor,
					0,
					TextPrintType::Metal12 | TextPrintType::BrightColor);
			}
		}
	}
}

void MapChoiceBoxClass::UpdateButtonRects(Point2D topLeft, int bgWidth, int buttonsStartY,
	const std::vector<BtnLayoutItem>& btnItems)
{
	this->m_buttonRects.clear();

	bool isVertical = (this->Type) ? (this->Type->Button_Layout != 0) : false;
	int btnCount = static_cast<int>(btnItems.size());

	if (isVertical)
	{
		for (int i = 0; i < btnCount; ++i)
		{
			const auto& item = btnItems[i];
			int btnX = topLeft.X + (bgWidth - item.Width) / 2;
			int btnY = buttonsStartY;
			for (int j = 0; j < i; ++j)
				btnY += btnItems[j].Height + BUTTON_SPACING;
			this->m_buttonRects.push_back({ i, { btnX, btnY, item.Width, item.Height } });
		}
	}
	else
	{
		// 横向多行：重新走一遍与 DrawAt 相同的 layout
		int availW = bgWidth - PADDINGX * 2;
		int curX = 0, curY = 0, rowH = 0;
		int maxRowW = 0;

		// 先计算 btnContentMaxW（与 DrawAt 保持一致）
		for (int i = 0; i < btnCount; ++i)
		{
			const auto& item = btnItems[i];
			int itemW = (item.Width > availW) ? availW : item.Width;

			if (curX + itemW > availW && curX > 0)
			{
				if (curX > maxRowW) maxRowW = curX;
				curX = 0;
				curY += rowH + BUTTON_SPACING;
				rowH = 0;
			}
			if (item.Height > rowH) rowH = item.Height;
			curX += itemW + BUTTON_SPACING;
		}
		if (curX > 0)
		{
			if (curX - BUTTON_SPACING > maxRowW)
				maxRowW = curX - BUTTON_SPACING;
		}
		int btnContentMaxW = maxRowW;

		// 再次遍历生成实际 Rects
		curX = 0; curY = 0; rowH = 0;
		for (int i = 0; i < btnCount; ++i)
		{
			const auto& item = btnItems[i];
			int itemW = (item.Width > availW) ? availW : item.Width;

			if (curX + itemW > availW && curX > 0)
			{
				curX = 0;
				curY += rowH + BUTTON_SPACING;
				rowH = 0;
			}
			int btnX = topLeft.X + (bgWidth - btnContentMaxW) / 2 + curX;
			int btnY = buttonsStartY + curY;
			this->m_buttonRects.push_back({ i, { btnX, btnY, itemW, item.Height } });
			curX += itemW + BUTTON_SPACING;
			if (item.Height > rowH) rowH = item.Height;
		}
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
		.Process(this->ClickedConsumed)
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
template <typename T>
static void DrawChoiceBoxList(std::vector<std::shared_ptr<T>>& boxes)
{
	// 阶段一：绘制并检测点击
	for (auto& ptr : boxes)
	{
		if (!ptr || ptr->IsExpired || !ptr->CanDraw())
			continue;

		// 隐藏期：已点击且 Duration 刚好耗尽 → 不绘制，保留对象供 TEvent 检测
		if (ptr->ClickedIndex >= 0 && ptr->RemainingFrames == 0)
			continue;

		Point2D drawPos;
		if (!ptr->GetDrawPosition(drawPos))
			continue;

		ptr->DrawAt(drawPos);

		if (ptr->CheckMouseClick())
		{
			ptr->ClickedConsumed = false;
			// 回弹模式：点击即启动隐藏期倒计时（不依赖 Duration 耗尽）
			if (ptr->Type && ptr->Type->Button_Mode == static_cast<int>(ChoiceBoxButtonMode::Bounce))
				ptr->ClickExpireCounter = CLICK_EXPIRE_FRAMES;
		}
	}

	// 阶段二：处理隐藏期倒计时（含回弹模式）
	for (auto& ptr : boxes)
	{
		if (ptr && !ptr->IsExpired && ptr->ClickExpireCounter >= 0)
		{
			if (--ptr->ClickExpireCounter <= 0)
			{
				// 检查被点击的按钮是否为回弹模式
				bool isBounce = (ptr->ClickedIndex >= 0 && ptr->Type
					&& ptr->Type->Button_Mode == static_cast<int>(ChoiceBoxButtonMode::Bounce));

				// 未消费则继续等待（给 TEvent 更多时间）
				if (!ptr->ClickedConsumed)
				{
					ptr->ClickExpireCounter = 0;
					continue;
				}

				if (isBounce)
				{
					// 回弹：重置点击状态，不清除对象
					ptr->ClickedIndex = -1;
					ptr->ClickExpireCounter = -1;
					ptr->ClickedConsumed = false;
				}
				else
				{
					ptr->IsExpired = true;
				}
			}
		}
	}

	// 阶段三：处理 Duration 自动移除
	for (auto& ptr : boxes)
	{
		if (ptr && !ptr->IsExpired && ptr->RemainingFrames >= 0)
		{
			if (--ptr->RemainingFrames <= 0)
			{
				// Duration 耗尽且已点击 → 不销毁，启动隐藏期供 TEvent 检测
				if (ptr->ClickedIndex >= 0)
				{
					ptr->RemainingFrames = 0;
					if (ptr->ClickExpireCounter < 0)
						ptr->ClickExpireCounter = CLICK_EXPIRE_FRAMES;
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
