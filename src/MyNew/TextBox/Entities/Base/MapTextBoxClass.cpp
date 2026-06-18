#include "MapTextBoxClass.h"

#include <MyNew/TextBox/Entities/Derived/TechnoTextBoxClass.h>
#include "../Derived/WaypointTextBoxClass.h"

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

// ========== 文本框布局常量 ==========
constexpr static const int PADDINGX = 6;          // 文字与边框之间的水平内边距
constexpr static const int PADDINGY = 4;          // 文字与边框之间的垂直内边距
constexpr static const int LINE_HEIGHT = 18;      // 每行文字的高度（行间距）
constexpr static const int BOTTOM_SAFE_HEIGHT = 0; // 底部安全区域（保留像素，防止被UI遮挡）

// 全局数组：存储所有活跃的文本框实例（基类指针，统一管理生命周期）
std::vector<std::shared_ptr<MapTextBoxClass>> MapTextBoxClass::Array;

/**
 * @brief 对宽字符串进行自动换行处理
 *
 * 遍历文本中的每个字符，逐字累加并测量宽度。
 * 当累加后的宽度超过最大宽度 maxWidth 时，将当前行存入结果并开始新行。
 * 遇到 \n 或 \r\n 时强制换行，控制字符（除 \t 外）被过滤掉。
 *
 * @param text     输入的宽字符串
 * @param maxWidth 单行最大像素宽度
 * @return          换行后的字符串数组
 */
static std::vector<std::wstring> WrapText(const wchar_t* text, int maxWidth)
{
	if (!text || wcslen(text) == 0 || maxWidth <= 0)
		return {};

	std::vector<std::wstring> lines;      // 最终换行结果
	std::wstring wStr(text);              // 源文本副本
	std::wstring currentLine;             // 当前正在累积的行

	for (size_t i = 0; i < wStr.length(); ++i)
	{
		wchar_t ch = wStr[i];

		// 处理显式换行符 \n 或 \r\n
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

		// 过滤掉控制字符（保留制表符 \t）
		if (ch < 0x20 && ch != L'\t')
			continue;

		// 试探性：将当前字符拼接到行尾，测量总宽度
		std::wstring testLine = currentLine + ch;
		RectangleStruct dims = Drawing::GetTextDimensions(testLine.c_str(), { 0, 0 }, 0, 2, 0);

		if (dims.Width > maxWidth)
		{
			// 超过最大宽度，触发换行
			if (currentLine.empty())
			{
				// 单个字符就已经超宽，强制放入该行
				lines.push_back(testLine);
			}
			else
			{
				// 将当前行存入结果，用当前字符另起新行
				lines.push_back(currentLine);
				currentLine = ch;
			}
		}
		else
		{
			// 未超宽，继续累加
			currentLine = testLine;
		}
	}

	// 处理最后剩余的一行
	if (!currentLine.empty())
		lines.push_back(currentLine);

	return lines;
}

// ========== 构造 / 析构 ==========

MapTextBoxClass::~MapTextBoxClass() = default;

/**
 * @brief 构造函数
 *
 * @param csfLabel       CSF 标签名（用于从 StringTable 中查找本地化文本）
 * @param maxWidth       单行最大像素宽度（<=0 时默认 250）
 * @param opacityPercent 背景不透明度（0-100，自动 clamp）
 * @param colorR         颜色 R 分量
 * @param colorG         颜色 G 分量
 * @param colorB         颜色 B 分量
 */
MapTextBoxClass::MapTextBoxClass(const char* csfLabel,
							 int maxWidth, int opacityPercent,
							 int colorR, int colorG, int colorB)
	: CurrentLabel(csfLabel ? csfLabel : "")
	, MaxLineWidth(maxWidth > 0 ? maxWidth : 250)
	, BackgroundOpacity(std::clamp(opacityPercent, 0, 100))
	, ColorR(colorR)
	, ColorG(colorG)
	, ColorB(colorB)
	, m_cache(std::make_unique<Cache>()) // 缓存对象，延迟到首次绘制时初始化
{}

// ========== 布局更新 ==========

/**
 * @brief 更新文本框的布局缓存
 *
 * 根据 CurrentLabel 从 CSF 中查找本地化文本，若找不到则直接使用 ANSI 转换。
 * 调用 WrapText 进行自动换行，并计算出背景框的宽高缓存到 m_cache 中。
 * 布局完成后将 IsLayoutDirty 置为 false。
 */
void MapTextBoxClass::UpdateLayout()
{
	if (!m_cache)
		m_cache = std::make_unique<Cache>();

	// 优先从 CSF 文件（RA2 的字符串表）中获取本地化文本
	const wchar_t* textPtr = StringTable::TryFetchString(this->CurrentLabel.c_str());

	// Fallback: CSF 标签不存在时，直接将 ANSI 标签名逐字节转成宽字符
	std::wstring fallbackText;
	if (!textPtr || wcslen(textPtr) == 0)
	{
		if (this->CurrentLabel.empty())
		{
			m_cache->CachedLines.clear();
			m_cache->CachedBgWidth = 0;
			m_cache->CachedBgHeight = 0;
			m_cache->IsLayoutDirty = false;
			return;
		}

		// 逐字节转换（标签名通常是 ASCII，不需要 MultiByteToWideChar 的开销）
		fallbackText.reserve(this->CurrentLabel.length());
		for (char ch : this->CurrentLabel)
			fallbackText += static_cast<wchar_t>(static_cast<unsigned char>(ch));
		textPtr = fallbackText.c_str();
	}

	// 再次检查文本是否有效
	if (!textPtr || wcslen(textPtr) == 0)
	{
		m_cache->CachedLines.clear();
		m_cache->CachedBgWidth = 0;
		m_cache->CachedBgHeight = 0;
		m_cache->IsLayoutDirty = false;
		return;
	}

	// 执行自动换行
	m_cache->CachedLines = WrapText(textPtr, this->MaxLineWidth);
	if (m_cache->CachedLines.empty())
	{
		m_cache->CachedBgWidth = 0;
		m_cache->CachedBgHeight = 0;
		m_cache->IsLayoutDirty = false;
		return;
	}

	// 计算最长行的像素宽度
	int maxLineW = 0;
	for (const std::wstring& line : m_cache->CachedLines)
	{
		RectangleStruct dims = Drawing::GetTextDimensions(line.c_str(), { 0, 0 }, 0, 2, 0);
		if (dims.Width > maxLineW)
			maxLineW = dims.Width;
	}

	// 背景框尺寸 = 文字区域 + 内边距
	m_cache->CachedBgWidth = maxLineW + (PADDINGX * 2);
	m_cache->CachedBgHeight = (static_cast<int>(m_cache->CachedLines.size()) * LINE_HEIGHT) + (PADDINGY * 2);
	m_cache->IsLayoutDirty = false;
}

// ========== 缓存管理 ==========

/**
 * @brief 重置布局缓存，强制下次绘制时重新计算布局
 *
 * 通常在标签内容或样式发生变化时调用。
 */
void MapTextBoxClass::ResetCache()
{
	m_cache = std::make_unique<Cache>();
	m_cache->IsLayoutDirty = true;
}

/**
 * @brief 判断该文本框当前是否允许绘制
 *
 * 基类始终返回 true，派生类（如 TechnoTextBoxClass）可重写此方法
 * 以实现在特定条件下隐藏文本框（如单位进入载具时）。
 */
bool MapTextBoxClass::CanDraw() const
{
	return true;
}

// ========== 绘制 ==========

/**
 * @brief 在指定屏幕位置绘制文本框
 *
 * 绘制流程：
 * 1. 检查 Canvas 有效性，若布局脏则重新更新
 * 2. 计算背景框的左上角坐标（以 centerPos 为中心居中）
 * 3. 进行底部裁剪，防止超出屏幕下边界（被 UI 栏遮挡）
 * 4. 鼠标悬停检测——若鼠标位于文本框区域内则跳过绘制（避免遮挡操作）
 * 5. 绘制半透明黑色背景
 * 6. 按 RGB 颜色绘制边框（四条边，若底部被裁剪则跳过底边）
 * 7. 逐行绘制文字
 *
 * @param centerPos 文本框在屏幕上的中心点坐标
 */
void MapTextBoxClass::DrawAt(Point2D centerPos)
{
	// 检查渲染表面和战术视图是否可用
	if (!DSurface::Composite || !TacticalClass::Instance)
		return;

	// 初始化缓存并进行延迟布局更新
	if (!m_cache)
		m_cache = std::make_unique<Cache>();
	if (m_cache->IsLayoutDirty)
		UpdateLayout();

	// 无有效内容则不绘制
	if (m_cache->CachedLines.empty() || m_cache->CachedBgWidth <= 0 || m_cache->CachedBgHeight <= 0)
		return;

	int bgWidth = m_cache->CachedBgWidth;
	int bgHeight = m_cache->CachedBgHeight;

	// 背景框左上角（居中定位）
	Point2D topLeft = {
		centerPos.X - (bgWidth / 2),
		centerPos.Y - (bgHeight / 2)
	};

	// ===== 底部裁剪 =====
	// 文本框不应超出屏幕底部（避免被界面栏遮挡）
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
			return; // 裁剪后连一行都放不下，直接跳过
	}

	// 完全在屏幕左侧或上方，跳过
	if (topLeft.X + bgWidth < 0 || topLeft.Y + drawHeight < 0)
		return;

	RectangleStruct bgRect = { topLeft.X, topLeft.Y, bgWidth, drawHeight };

	// ===== 鼠标悬停检测 =====
	// 鼠标移到文本框上时暂停绘制，让玩家能点选被遮挡的单位
	RectangleStruct fullBgRect = { topLeft.X, topLeft.Y, bgWidth, bgHeight };
	Point2D mousePos = WWMouseClass::Instance->XY1;
	if (mousePos.X >= fullBgRect.X &&
		mousePos.X <= fullBgRect.X + fullBgRect.Width &&
		mousePos.Y >= fullBgRect.Y &&
		mousePos.Y <= fullBgRect.Y + fullBgRect.Height)
	{
		return;
	}

	// ===== 绘制半透明黑色背景 =====
	ColorStruct bgColor = { 0, 0, 0 };
	DSurface::Composite->FillRectTrans(&bgRect, &bgColor, this->BackgroundOpacity);

	// 将 RGB 分量转为游戏引擎所需的颜色整数值
	int colorInt = Drawing::RGB_To_Int(ColorStruct(
		static_cast<unsigned char>(this->ColorR),
		static_cast<unsigned char>(this->ColorG),
		static_cast<unsigned char>(this->ColorB)));
	Point2D p1, p2;

	// ===== 绘制边框（上、左、右、下） =====
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

	// 底边（被裁剪时跳过，避免画出界）
	if (!isClipped)
	{
		p1 = { topLeft.X, topLeft.Y + bgHeight - 1 };
		p2 = { topLeft.X + bgWidth - 1, topLeft.Y + bgHeight - 1 };
		DSurface::Composite->DrawLine(&p1, &p2, colorInt);
	}

	// ===== 逐行绘制文字 =====
	RectangleStruct bounds = DSurface::ViewBounds;
	int currentY = topLeft.Y + PADDINGY;
	for (const std::wstring& line : m_cache->CachedLines)
	{
		// 当前行超出裁剪区域则停止
		if (currentY + LINE_HEIGHT > topLeft.Y + drawHeight + PADDINGY)
			break;

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

// ========== 序列化（存档/读档） ==========

/**
 * @brief 序列化核心模板
 *
 * 将所有需要持久化的成员变量传递给 Phobos 流框架。
 * 派生类应调用此基类方法后再处理自己的额外字段。
 */
template <typename T>
bool MapTextBoxClass::Serialize(T& Stm)
{
	return Stm
		.Process(this->CurrentLabel)        // CSF 标签名
		.Process(this->MaxLineWidth)        // 最大行宽
		.Process(this->BackgroundOpacity)   // 背景不透明度
		.Process(this->ColorR)              // 颜色 R
		.Process(this->ColorG)              // 颜色 G
		.Process(this->ColorB)              // 颜色 B
		.Process(this->RemainingFrames)     // 剩余显示帧数
		.Success();
}

/**
 * @brief 清空所有文本框实例
 *
 * 依次清空路径点文本框数组、单位文本框数组和基类数组。
 */
void MapTextBoxClass::Clear()
{
	WaypointTextBoxClass::Array.clear();
	TechnoTextBoxClass::Array.clear();
	Array.clear();
}

bool MapTextBoxClass::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	return Serialize(Stm);
}

bool MapTextBoxClass::Save(PhobosStreamWriter& Stm) const
{
	return const_cast<MapTextBoxClass*>(this)->Serialize(Stm);
}



// ========== 全局绘制入口 ==========

/**
 * @brief 绘制所有活跃的文本框
 *
 * 在主循环的每帧渲染中被调用，执行以下操作：
 * 1. 遍历所有文本框，进行倒计时（RemainingFrames）递减，过期移除
 * 2. 调用每个文本框的 CanDraw() 和 GetDrawPosition() 判断是否可绘
 * 3. 调用 DrawAt() 渲染文本框
 * 4. 移除过期实例（同时从基类数组和派生类数组中移除）
 * 5. 调用 CleanupDeadLabels() 清理已销毁单位的残留标签
 */
void MapTextBoxClass::DrawAll()
{
	std::vector<std::shared_ptr<MapTextBoxClass>> expired; // 收集本帧过期的实例

	for (auto& pLabel : Array)
	{
		if (!pLabel)
			continue;

		// ===== 倒计时递减 =====
		// RemainingFrames >= 0 表示有时限，每帧减1，减到0时标记为过期
		if (pLabel->RemainingFrames >= 0)
		{
			if (--pLabel->RemainingFrames <= 0)
				expired.push_back(pLabel);
		}

		// ===== 检查是否允许绘制 =====
		if (!pLabel->CanDraw())
			continue;

		// ===== 获取绘制位置 =====
		Point2D pos {};
		if (!pLabel->GetDrawPosition(pos))
			continue;

		// ===== 执行绘制 =====
		pLabel->DrawAt(pos);
	}

	// ===== 移除过期实例 =====
	// 注意：必须同时从基类 Array 和对应的派生类 Array 中移除
	for (auto& pExpired : expired)
	{
		auto const marker = pExpired->GetTypeMarker();

		// 从派生类数组中移除
		if (std::strcmp(marker, "WaypointTextBoxClass") == 0)
		{
			auto* pRaw = static_cast<WaypointTextBoxClass*>(pExpired.get());
			auto& derived = WaypointTextBoxClass::Array;
			auto it = std::find_if(derived.begin(), derived.end(),
				[pRaw](const auto& sp) { return sp.get() == pRaw; });
			if (it != derived.end())
				derived.erase(it);
		}
		else if (std::strcmp(marker, "TechnoTextBoxClass") == 0)
		{
			auto* pRaw = static_cast<TechnoTextBoxClass*>(pExpired.get());
			auto& derived = TechnoTextBoxClass::Array;
			auto it = std::find_if(derived.begin(), derived.end(),
				[pRaw](const auto& sp) { return sp.get() == pRaw; });
			if (it != derived.end())
				derived.erase(it);
		}

		// 从基类数组中移除
		auto baseIt = std::find(Array.begin(), Array.end(), pExpired);
		if (baseIt != Array.end())
			Array.erase(baseIt);
	}

	// 每帧清理已摧毁单位的残留标签
	TechnoTextBoxClass::CleanupDeadLabels();
}

/**
 * @brief 清空所有文本框（用于场景重置等）
 */
void MapTextBoxClass::ClearAll()
{
	Array.clear();
}

// ========== 全局存档/读档 ==========

/**
 * @brief 保存所有文本框到存档流
 *
 * 遍历基类 Array，对每个实例保存：
 * - 旧指针（用于读档时的指针映射重建）
 * - 类型标记字符串（用于反序列化时确定派生类类型）
 * - 实例自身的序列化数据
 */
bool MapTextBoxClass::SaveGlobals(PhobosStreamWriter& Stm)
{
	Stm.Save(Array.size());
	for (auto const& item : Array)
	{
		Stm.Save(item.get());                             // 保存旧指针地址
		PhobosFixedString<64> marker(item->GetTypeMarker()); // 保存类型标记
		Stm.Save(marker);
		item->Save(Stm);                                  // 保存实例数据
	}
	return true;
}

/**
 * @brief 从存档流加载所有文本框
 *
 * 清除当前所有实例后，按存档顺序逐一重建：
 * 1. 读取旧指针（占位）
 * 2. 读取类型标记，确定派生类类型
 * 3. 创建对应派生类实例并调用 Load
 * 4. 同时加入派生类 Array 和基类 Array
 */
bool MapTextBoxClass::LoadGlobals(PhobosStreamReader& Stm)
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

		if (std::strcmp(typeMarker.data(), "WaypointTextBoxClass") == 0)
		{
			auto newObj = std::make_shared<WaypointTextBoxClass>();
			if (!newObj->Load(Stm, false))
				return false;
			WaypointTextBoxClass::Array.push_back(newObj);
			Array.push_back(std::move(newObj));
		}
		else if (std::strcmp(typeMarker.data(), "TechnoTextBoxClass") == 0)
		{
			auto newObj = std::make_shared<TechnoTextBoxClass>();
			if (!newObj->Load(Stm, false))
				return false;
			TechnoTextBoxClass::Array.push_back(newObj);
			Array.push_back(std::move(newObj));
		}
		else
		{
			Debug::Log("[MapTextBoxClass] Warning: unknown type marker \"%s\"!\n",
				typeMarker.data());
			return false;
		}
	}

	return true;
}
