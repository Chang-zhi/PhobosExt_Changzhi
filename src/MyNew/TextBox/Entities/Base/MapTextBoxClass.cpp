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

constexpr static const int PADDINGX = 6;
constexpr static const int PADDINGY = 4;
constexpr static const int LINE_HEIGHT = 18;
constexpr static const int BOTTOM_SAFE_HEIGHT = 0;

std::vector<std::shared_ptr<MapTextBoxClass>> MapTextBoxClass::Array;

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

MapTextBoxClass::~MapTextBoxClass() = default;

MapTextBoxClass::MapTextBoxClass(const char* csfLabel,
							 int maxWidth, int opacityPercent,
							 int colorR, int colorG, int colorB)
	: CurrentLabel(csfLabel ? csfLabel : "")
	, MaxLineWidth(maxWidth > 0 ? maxWidth : 250)
	, BackgroundOpacity(std::clamp(opacityPercent, 0, 100))
	, ColorR(colorR)
	, ColorG(colorG)
	, ColorB(colorB)
	, m_cache(std::make_unique<Cache>())
{}

void MapTextBoxClass::UpdateLayout()
{
	if (!m_cache)
		m_cache = std::make_unique<Cache>();

	m_cache->CachedTextPtr = StringTable::TryFetchString(this->CurrentLabel.c_str());

	if (!m_cache->CachedTextPtr || wcslen(m_cache->CachedTextPtr) == 0)
	{
		m_cache->CachedLines.clear();
		m_cache->CachedBgWidth = 0;
		m_cache->CachedBgHeight = 0;
		m_cache->IsLayoutDirty = false;
		return;
	}

	m_cache->CachedLines = WrapText(m_cache->CachedTextPtr, this->MaxLineWidth);
	if (m_cache->CachedLines.empty())
	{
		m_cache->CachedBgWidth = 0;
		m_cache->CachedBgHeight = 0;
		m_cache->IsLayoutDirty = false;
		return;
	}

	int maxLineW = 0;
	for (const std::wstring& line : m_cache->CachedLines)
	{
		RectangleStruct dims = Drawing::GetTextDimensions(line.c_str(), { 0, 0 }, 0, 2, 0);
		if (dims.Width > maxLineW)
			maxLineW = dims.Width;
	}

	m_cache->CachedBgWidth = maxLineW + (PADDINGX * 2);
	m_cache->CachedBgHeight = (static_cast<int>(m_cache->CachedLines.size()) * LINE_HEIGHT) + (PADDINGY * 2);
	m_cache->IsLayoutDirty = false;
}

void MapTextBoxClass::ResetCache()
{
	m_cache = std::make_unique<Cache>();
	m_cache->IsLayoutDirty = true;
}

bool MapTextBoxClass::CanDraw() const
{
	return true;
}

void MapTextBoxClass::DrawAt(Point2D centerPos)
{
	if (!DSurface::Composite || !TacticalClass::Instance)
		return;

	if (!m_cache)
		m_cache = std::make_unique<Cache>();
	if (m_cache->IsLayoutDirty)
		UpdateLayout();

	if (m_cache->CachedLines.empty() || m_cache->CachedBgWidth <= 0 || m_cache->CachedBgHeight <= 0)
		return;

	int bgWidth = m_cache->CachedBgWidth;
	int bgHeight = m_cache->CachedBgHeight;

	Point2D topLeft = {
		centerPos.X - (bgWidth / 2),
		centerPos.Y - (bgHeight / 2)
	};

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

	RectangleStruct fullBgRect = { topLeft.X, topLeft.Y, bgWidth, bgHeight };
	Point2D mousePos = WWMouseClass::Instance->XY1;
	if (mousePos.X >= fullBgRect.X &&
		mousePos.X <= fullBgRect.X + fullBgRect.Width &&
		mousePos.Y >= fullBgRect.Y &&
		mousePos.Y <= fullBgRect.Y + fullBgRect.Height)
	{
		return;
	}

	ColorStruct bgColor = { 0, 0, 0 };
	DSurface::Composite->FillRectTrans(&bgRect, &bgColor, this->BackgroundOpacity);

	int colorInt = Drawing::RGB_To_Int(ColorStruct(
		static_cast<unsigned char>(this->ColorR),
		static_cast<unsigned char>(this->ColorG),
		static_cast<unsigned char>(this->ColorB)));
	Point2D p1, p2;

	p1 = { topLeft.X, topLeft.Y };
	p2 = { topLeft.X + bgWidth - 1, topLeft.Y };
	DSurface::Composite->DrawLine(&p1, &p2, colorInt);

	p1 = { topLeft.X, topLeft.Y };
	p2 = { topLeft.X, topLeft.Y + drawHeight - 1 };
	DSurface::Composite->DrawLine(&p1, &p2, colorInt);

	p1 = { topLeft.X + bgWidth - 1, topLeft.Y };
	p2 = { topLeft.X + bgWidth - 1, topLeft.Y + drawHeight - 1 };
	DSurface::Composite->DrawLine(&p1, &p2, colorInt);

	if (!isClipped)
	{
		p1 = { topLeft.X, topLeft.Y + bgHeight - 1 };
		p2 = { topLeft.X + bgWidth - 1, topLeft.Y + bgHeight - 1 };
		DSurface::Composite->DrawLine(&p1, &p2, colorInt);
	}

	RectangleStruct bounds = DSurface::ViewBounds;
	int currentY = topLeft.Y + PADDINGY;
	for (const std::wstring& line : m_cache->CachedLines)
	{
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

template <typename T>
bool MapTextBoxClass::Serialize(T& Stm)
{
	return Stm
		.Process(this->CurrentLabel)
		.Process(this->MaxLineWidth)
		.Process(this->BackgroundOpacity)
		.Process(this->ColorR)
		.Process(this->ColorG)
		.Process(this->ColorB)
		.Success();
}

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



void MapTextBoxClass::DrawAll()
{
	for (auto& pLabel : Array)
	{
		if (!pLabel)
			continue;
		if (!pLabel->CanDraw())
			continue;

		Point2D pos {};
		if (!pLabel->GetDrawPosition(pos))
			continue;

		pLabel->DrawAt(pos);
	}

	// ÿ֡������������λ�ı�ǩ
	TechnoTextBoxClass::CleanupDeadLabels();
}

void MapTextBoxClass::ClearAll()
{
	Array.clear();
}

bool MapTextBoxClass::SaveGlobals(PhobosStreamWriter& Stm)
{
	Stm.Save(Array.size());
	for (auto const& item : Array)
	{
		Stm.Save(item.get());
		PhobosFixedString<64> marker(item->GetTypeMarker());
		Stm.Save(marker);
		item->Save(Stm);
	}
	return true;
}

bool MapTextBoxClass::LoadGlobals(PhobosStreamReader& Stm)
{
	// ����ǰ���������ʵ������
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
		// TechnoTextBoxClass �������л���������ָ�
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
