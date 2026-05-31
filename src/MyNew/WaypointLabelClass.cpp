// ======================================================================
//						自定义路径点文本显示
//						类: WaypointLabelClass
//						作者: Chang_zhi
// ======================================================================

#include "WaypointLabelClass.h"

#include <StringTable.h>
#include <Surface.h>
#include <Drawing.h>
#include <TacticalClass.h>
#include <ScenarioClass.h>
#include <CellClass.h>
#include <RulesClass.h>
#include <SessionClass.h>
#include <WWMouseClass.h>

#include <Ext/Rules/Body.h>

#include <Utilities/Debug.h>
#include <Utilities/Stream.h>

#include <vector>
#include <algorithm>
#include <memory>

// ===== 常量定义 =====
constexpr static const int PADDINGX = 6;          // 文字左右内边距
constexpr static const int PADDINGY = 4;          // 文字上下内边距
constexpr static const int LINE_HEIGHT = 18;      // 每行文字高度（像素）
// bugfix: 会错误绘制到下UI上, 添加裁剪功能
// 默认底部安全区域为0, 如果需要可以调整该值
constexpr static const int BOTTOM_SAFE_HEIGHT = 0; // 底部安全区域

// 全局数组实例化
std::vector<std::unique_ptr<WaypointLabelClass>> WaypointLabelClass::Array;

// ===== 内部缓存结构体定义 =====
struct WaypointLabelClass::Cache
{
	bool IsLayoutDirty { true };                 // 布局是否需要重新计算
	std::vector<std::wstring> CachedLines;       // 分行后的文本（宽字符）
	int CachedBgWidth { 0 };                     // 背景宽度（像素）
	int CachedBgHeight { 0 };                    // 背景高度（像素）
	const wchar_t* CachedTextPtr { nullptr };    // 从 CSF 获取的文本指针（不拥有文本, 只是保存指针）
};

// ===== 辅助函数 =====

// 将颜色枚举转换为 ColorStruct
static ColorStruct GetColorStructByEnum(const WaypointLabel::Color& intColor)
{
	switch (intColor)
	{
	case WaypointLabel::Color::gold:		return ColorStruct(255, 215, 0);
	case WaypointLabel::Color::white:		return ColorStruct(255, 255, 255);
	case WaypointLabel::Color::red:			return ColorStruct(255, 0, 0);
	case WaypointLabel::Color::blue:		return ColorStruct(0, 0, 255);
	case WaypointLabel::Color::green:		return ColorStruct(0, 128, 0);
	case WaypointLabel::Color::yellow:		return ColorStruct(255, 255, 0);
	case WaypointLabel::Color::purple:		return ColorStruct(128, 0, 128);
	case WaypointLabel::Color::pink:		return ColorStruct(255, 192, 203);
	case WaypointLabel::Color::lightblue:	return ColorStruct(173, 216, 230);
	default:								return ColorStruct(255, 215, 0); // 默认金色
	}
}

// 文本换行：根据最大宽度（像素）将宽字符串拆分为多行
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

		// 处理显式换行符
		if (ch == L'\n' || ch == L'\r')
		{
			if (!currentLine.empty())
			{
				lines.push_back(currentLine);
				currentLine.clear();
			}
			if (ch == L'\r' && i + 1 < wStr.length() && wStr[i + 1] == L'\n')
				++i; // 跳过 \r\n 中的 \n
			continue;
		}

		// 忽略不可见控制字符（制表符除外）
		if (ch < 0x20 && ch != L'\t')
			continue;

		// 尝试加入当前字符
		std::wstring testLine = currentLine + ch;
		RectangleStruct dims = Drawing::GetTextDimensions(testLine.c_str(), { 0,0 }, 0, 2, 0);

		if (dims.Width > maxWidth)
		{
			if (currentLine.empty())
			{
				// 单个字符就超过宽度，强制加入（通常不会发生）
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

// ===== 构造函数 =====
WaypointLabelClass::WaypointLabelClass(int wpIndex, const char* csfLabel,
									   int maxWidth, int opacityPercent,
									   WaypointLabel::Color color)
	: WaypointIndex(wpIndex)
	, CurrentLabel(csfLabel ? csfLabel : "")
	, MaxLineWidth(maxWidth > 0 ? maxWidth : 250)
	, BackgroundOpacity(std::clamp(opacityPercent, 0, 100))
	, Color(color)
	, m_cache(std::make_unique<Cache>())   // 创建缓存对象
{}

// ===== 触发动作接口实现 =====

void WaypointLabelClass::SetLabel(int wpIndex, const char* csfLabel,
								  int maxWidth, int opacityPercent,
								  WaypointLabel::Color color)
{
	if (wpIndex < 0) return;
	if (!RulesClass::Instance) return; // 安全检测

	// 查找是否已存在该路径点的标签
	auto it = std::find_if(Array.begin(), Array.end(),
		[wpIndex](const std::unique_ptr<WaypointLabelClass>& pLabel) {
			return pLabel->WaypointIndex == wpIndex;	});

	// 如果文本为空，则删除该标签
	if (!(csfLabel && csfLabel[0]))
	{
		if (it != Array.end())
			Array.erase(it);
		return;
	}

	int actualOpacity = std::clamp(opacityPercent, 0, 100);
	int actualMaxWidth = (maxWidth > 0 && maxWidth <= 1000) ? maxWidth : 250;

	if (it != Array.end())
	{
		// 更新已有标签
		auto& pLabel = *it;
		pLabel->CurrentLabel = csfLabel;
		pLabel->MaxLineWidth = actualMaxWidth;
		pLabel->BackgroundOpacity = actualOpacity;
		pLabel->Color = color;
		pLabel->UpdateLayout();   // 立即更新布局
	}
	else
	{
		// 创建新标签
		auto newLabel = std::make_unique<WaypointLabelClass>(
			wpIndex, csfLabel, actualMaxWidth, actualOpacity, color);
		newLabel->UpdateLayout();
		Array.push_back(std::move(newLabel));
	}
}

void WaypointLabelClass::ClearLabel(int wpIndex)
{
	auto it = std::find_if(Array.begin(), Array.end(),
		[wpIndex](const std::unique_ptr<WaypointLabelClass>& pLabel) {
			return pLabel->WaypointIndex == wpIndex;	});
	if (it != Array.end())
		Array.erase(it);
}

void WaypointLabelClass::ClearAll()
{
	Array.clear();
}

// ===== 布局更新 =====
void WaypointLabelClass::UpdateLayout()
{
	if (!m_cache) m_cache = std::make_unique<Cache>();

	// 获取实际显示的文本
	m_cache->CachedTextPtr = StringTable::TryFetchString(this->CurrentLabel.c_str());

	// 无效文本时清空缓存
	if (!m_cache->CachedTextPtr || wcslen(m_cache->CachedTextPtr) == 0)
	{
		m_cache->CachedLines.clear();
		m_cache->CachedBgWidth = 0;
		m_cache->CachedBgHeight = 0;
		m_cache->IsLayoutDirty = false;
		return;
	}

	// 执行换行计算
	m_cache->CachedLines = WrapText(m_cache->CachedTextPtr, this->MaxLineWidth);
	if (m_cache->CachedLines.empty())
	{
		m_cache->CachedBgWidth = 0;
		m_cache->CachedBgHeight = 0;
		m_cache->IsLayoutDirty = false;
		return;
	}

	// 计算最大行宽和总高度
	int maxLineW = 0;
	for (const auto& line : m_cache->CachedLines)
	{
		RectangleStruct dims = Drawing::GetTextDimensions(line.c_str(), { 0,0 }, 0, 2, 0);
		if (dims.Width > maxLineW)
			maxLineW = dims.Width;
	}

	m_cache->CachedBgWidth = maxLineW + (PADDINGX * 2);
	m_cache->CachedBgHeight = (static_cast<int>(m_cache->CachedLines.size()) * LINE_HEIGHT) + (PADDINGY * 2);
	m_cache->IsLayoutDirty = false;
}

// ===== 绘制 =====
void WaypointLabelClass::Draw()
{
	// ----- 安全检测 -----
	if (this->WaypointIndex < 0) return;
	if (!ScenarioClass::Instance->IsDefinedWaypoint(this->WaypointIndex)) return;
	if (!TacticalClass::Instance || !DSurface::Composite ||
		!RulesClass::Instance || !ScenarioClass::Instance) return;

	// 确保缓存存在且布局不是脏的
	if (!m_cache) m_cache = std::make_unique<Cache>();
	if (m_cache->IsLayoutDirty) UpdateLayout();

	// 缓存无效则跳过绘制
	if (m_cache->CachedLines.empty() || m_cache->CachedBgWidth <= 0 || m_cache->CachedBgHeight <= 0)
		return;

	// ----- 获取路径点屏幕坐标 -----
	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(this->WaypointIndex);
	if (cell.X < 0 && cell.Y < 0) return;

	CoordStruct coords = CellClass::Cell2Coord(cell);
	Point2D screenPos;
	if (!TacticalClass::Instance->CoordsToClient(&coords, &screenPos))
		return;

	int bgWidth = m_cache->CachedBgWidth;
	int bgHeight = m_cache->CachedBgHeight;

	// 计算标签左上角位置（居中，位于路径点上方）
	Point2D topLeft = {
		screenPos.X - (bgWidth / 2),
		screenPos.Y - bgHeight - 10
	};

	int viewHeight = DSurface::ViewBounds.Height;
	int clipBottomY = viewHeight - BOTTOM_SAFE_HEIGHT;

	// 完全超出底部安全区域则跳过
	if (topLeft.Y >= clipBottomY)
		return;

	// 处理底部裁剪
	int drawHeight = bgHeight;
	bool isClipped = false;
	if (topLeft.Y + bgHeight > clipBottomY)
	{
		drawHeight = clipBottomY - topLeft.Y;
		isClipped = true;
		if (drawHeight < LINE_HEIGHT)
			return; // 剩余高度不足以显示一行文字
	}

	// 超出屏幕左右边界则跳过
	if (topLeft.X + bgWidth < 0 || topLeft.Y + drawHeight < 0)
		return;

	RectangleStruct bgRect = { topLeft.X, topLeft.Y, bgWidth, drawHeight };

	// 鼠标悬停检测（完整矩形）
	Point2D mousePos = WWMouseClass::Instance->XY1;
	RectangleStruct fullBgRect = { topLeft.X, topLeft.Y, bgWidth, bgHeight };
	if (mousePos.X >= fullBgRect.X &&
		mousePos.X <= fullBgRect.X + fullBgRect.Width &&
		mousePos.Y >= fullBgRect.Y &&
		mousePos.Y <= fullBgRect.Y + fullBgRect.Height)
	{
		return; // 鼠标悬浮时隐藏标签
	}

	// ----- 绘制半透明背景 -----
	ColorStruct bgColor = { 0, 0, 0 };
	DSurface::Composite->FillRectTrans(&bgRect, &bgColor, this->BackgroundOpacity);

	// ----- 绘制边框（注意裁剪）-----
	int colorInt = Drawing::RGB_To_Int(GetColorStructByEnum(this->Color));
	Point2D p1, p2;

	// 上边框（不裁剪）
	p1 = { topLeft.X, topLeft.Y };
	p2 = { topLeft.X + bgWidth - 1, topLeft.Y };
	DSurface::Composite->DrawLine(&p1, &p2, colorInt);

	// 左边框（裁剪）
	p1 = { topLeft.X, topLeft.Y };
	p2 = { topLeft.X, topLeft.Y + drawHeight - 1 };
	DSurface::Composite->DrawLine(&p1, &p2, colorInt);

	// 右边框（裁剪）
	p1 = { topLeft.X + bgWidth - 1, topLeft.Y };
	p2 = { topLeft.X + bgWidth - 1, topLeft.Y + drawHeight - 1 };
	DSurface::Composite->DrawLine(&p1, &p2, colorInt);

	// 下边框（仅当未裁剪时绘制）
	if (!isClipped)
	{
		p1 = { topLeft.X, topLeft.Y + bgHeight - 1 };
		p2 = { topLeft.X + bgWidth - 1, topLeft.Y + bgHeight - 1 };
		DSurface::Composite->DrawLine(&p1, &p2, colorInt);
	}

	// ----- 绘制文字 -----
	RectangleStruct bounds = DSurface::ViewBounds;
	int currentY = topLeft.Y + PADDINGY;
	for (const std::wstring& line : m_cache->CachedLines)
	{
		if (currentY + LINE_HEIGHT > topLeft.Y + drawHeight + PADDINGY)
			break; // 超出裁剪区域

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
}

void WaypointLabelClass::DrawAll()
{
	for (const std::unique_ptr<WaypointLabelClass>& pLabel : Array)
	{
		if (!pLabel) continue;

		// 如果路径点被迷雾遮挡则跳过绘制
		if(RulesExt::Global()->ShowWaypointLabelInShroud)
		{
			CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(pLabel->WaypointIndex);
			char isShrouded = TacticalClass::Instance->GetOcclusion(cell, false);
			if (static_cast<int>(isShrouded) == -2)  // -1是可见, -2是遮挡
			{
				Debug::Log("[WaypointLabelClass]: Index \"%d\", isShrouded is \"%d\".\n", pLabel->WaypointIndex, isShrouded);
				continue;
			}
		}

		pLabel->Draw();
	}
}

// ===== 序列化实现 =====
template <typename T>
bool WaypointLabelClass::Serialize(T& Stm)
{
	// 只序列化持久化成员，忽略 m_cache
	return Stm
		.Process(this->WaypointIndex)
		.Process(this->CurrentLabel)
		.Process(this->MaxLineWidth)
		.Process(this->BackgroundOpacity)
		.Process(this->Color)
		.Success();
}

bool WaypointLabelClass::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	return Serialize(Stm);
}

bool WaypointLabelClass::Save(PhobosStreamWriter& Stm) const
{
	return const_cast<WaypointLabelClass*>(this)->Serialize(Stm);
}

bool WaypointLabelClass::SaveGlobals(PhobosStreamWriter& Stm)
{
	return Stm.Process(Array).Success();
}

bool WaypointLabelClass::LoadGlobals(PhobosStreamReader& Stm)
{
	Array.clear();
	bool result = Stm.Process(Array).Success();

	if (result)
	{
		// 读档后为每个标签重建缓存，并标记为脏
		for (auto& pLabel : Array)
		{
			if (pLabel)
			{
				pLabel->m_cache = std::make_unique<Cache>();
				pLabel->m_cache->IsLayoutDirty = true;  // 下次绘制时自动重新计算布局
			}
		}
	}
	return result;
}
