// ======================================================================
//					Custom Waypoint Label Display
//					Class: WaypointLabelClass
//					Author: Chang_zhi
// ======================================================================

#include <MyNew/WaypointLabelClass.h>

#include <StringTable.h>
#include <Surface.h>
#include <Drawing.h>
#include <TacticalClass.h>
#include <ScenarioClass.h> 
#include <CellClass.h>
#include <RulesClass.h>
#include <SessionClass.h>
#include <WWMouseClass.h>

#include <Utilities/Debug.h>
#include <Utilities/Stream.h> 

#include <vector>
#include <algorithm>

// Constants pixels count
constexpr static const int PADDINGX = 6;
constexpr static const int PADDINGY = 4;
constexpr static const int LINE_HEIGHT = 18;
// Debug: Bottom safe area to prevent drawing on bottom UI
constexpr static const int BOTTOM_SAFE_HEIGHT = 0;

std::vector<std::unique_ptr<WaypointLabelClass>> WaypointLabelClass::Array;

// Constructor
WaypointLabelClass::WaypointLabelClass(int wpIndex, const char* csfLabel, int maxWidth, int opacityPercent, WaypointLabel::Color color)
    : WaypointIndex(wpIndex)
    , CurrentLabel(csfLabel ? csfLabel : "")
    , MaxLineWidth(maxWidth > 0 ? maxWidth : 250)
    , BackgroundOpacity(std::clamp(opacityPercent, 0, 100))
    , Color(color)
{}

// First Interface: Action 550
void WaypointLabelClass::SetLabel(int wpIndex, const char* csfLabel, int maxWidth, int opacityPercent, WaypointLabel::Color color)
{
    if (wpIndex < 0) return;
    if (!RulesClass::Instance) return;  // Safety check, but may be unnecessary

	// get iterator
    auto it = std::find_if(Array.begin(), Array.end(),
        [wpIndex](const std::unique_ptr<WaypointLabelClass>& pLabel) -> bool {
            return (pLabel->WaypointIndex) == wpIndex;
        });

    if (!(csfLabel && csfLabel[0])) // If csf is empty, delete it
    {
        Array.erase(it);
        return;
    }

	// Safety check, but may be unnecessary
    int actualOpacity = std::clamp(opacityPercent, 0, 100);
	int actualMaxWidth = (maxWidth > 0 || maxWidth <= 1000) ? maxWidth : 250;

    if (it != Array.end()) // Exist
    {
        auto& pLabel = *it;
        pLabel->CurrentLabel = csfLabel;
        pLabel->MaxLineWidth = actualMaxWidth;
        pLabel->BackgroundOpacity = actualOpacity; 
        pLabel->Color = color;
        
        // Inmidately update layout
        // pLabel->IsLayoutDirty = true; 
        pLabel->UpdateLayout();
    }
	else // it == Array.end()
    {
        auto newLabel = std::make_unique<WaypointLabelClass>(wpIndex, csfLabel, actualMaxWidth, actualOpacity, color);
		// Inmidately update layout
        newLabel->UpdateLayout();
        Array.push_back(std::move(newLabel));
    }
}

// No.2 Interface: Action 551
void WaypointLabelClass::ClearLabel(int wpIndex)
{
    auto it = std::find_if(Array.begin(), Array.end(), 
        [wpIndex](const std::unique_ptr<WaypointLabelClass>& pLabel) -> bool {
            return pLabel->WaypointIndex == wpIndex;
        });

    if (it != Array.end())
    {
        Array.erase(it);
    }
}

// No.3 Interface: Action 552
void WaypointLabelClass::ClearAll()
{
    Array.clear();
}

// Helper function: convert enum to ColorStruct
static ColorStruct GetColorStructByEnum(const WaypointLabel::Color& intColor) {
    switch (intColor)
    {
        case WaypointLabel::Color::gold:
            return ColorStruct(255, 215, 0);
        case WaypointLabel::Color::white:
            return ColorStruct(255, 255, 255);
        case WaypointLabel::Color::red:
            return ColorStruct(255, 0, 0);
        case WaypointLabel::Color::blue:
            return ColorStruct(0, 0, 255);
        case WaypointLabel::Color::green:
            return ColorStruct(0, 128, 0);
        case WaypointLabel::Color::yellow:
            return ColorStruct(255, 255, 0);
        case WaypointLabel::Color::purple:
            return ColorStruct(128, 0, 128);
        case WaypointLabel::Color::pink:
            return ColorStruct(255, 192, 203);
        case WaypointLabel::Color::lightblue:
            return ColorStruct(173, 216, 230);
        default: 
            return ColorStruct(255, 215, 0);
    }
}

// Helper: wrap text by max width (pixels), return vector of each lines
static std::vector<std::wstring> WrapText(const wchar_t* text, int maxWidth)
{
	if (!text || wcslen(text) == 0 || maxWidth <= 0) return {};

    std::vector<std::wstring> lines; // Result
    std::wstring wStr(text);		 // Original text
    std::wstring currentLine;

    // Iterate over each character
    for (size_t i = 0; i < wStr.length(); ++i)
    {
		// current character
        wchar_t ch = wStr[i];

		// 1. Handle explicit line breaks (\n or \r\n)
        if (ch == L'\n' || ch == L'\r')
        {
			// Save current line if it has content
            if (!currentLine.empty())
            {
                lines.push_back(currentLine);
                currentLine.clear();
            }
			// Skip the following \n if this is \r\n
            if (ch == L'\r' && i + 1 < wStr.length() && wStr[i + 1] == L'\n')
            {
                ++i;
            }
            continue;
        }

		// Ignore invisible control characters (except '\t') to prevent layout issues
        if (ch < 0x20 && ch != L'\t')
        {
            continue;
        }

		// 3. Try appending current character to the current line
		std::wstring testLine = currentLine + ch;   // Test line with current character
		RectangleStruct dims = Drawing::GetTextDimensions(testLine.c_str(), { 0,0 }, 0, 2, 0);

		// If width exceeds max limit after adding
		if (dims.Width > maxWidth)
		{
			// If current line is empty, force add the character (should only happen if max width is extremely small)
			if (currentLine.empty())
			{
				lines.push_back(testLine);
			}
			else
			{
				// Save the current completed line
				lines.push_back(currentLine);
				// Start new line with current character
				currentLine = ch;
			}
		}
		else
		{
			// Within width limit, append to current line
			currentLine = testLine;
		}
    }

	// 4. Process the final line
    if (!currentLine.empty())
    {
        lines.push_back(currentLine);
    }

    return lines;
}

// Lazy load to improve performance
void WaypointLabelClass::UpdateLayout()
{
	// 1. Get and cache text pointer
	this->CachedTextPtr = StringTable::TryFetchString(this->CurrentLabel.c_str());

	// Clear cache if text is invalid or empty
	if (!this->CachedTextPtr || wcslen(this->CachedTextPtr) == 0)
	{
		this->CachedLines.clear();
		this->CachedBgWidth = 0;
		this->CachedBgHeight = 0;
		this->IsLayoutDirty = false;
		return;
	}

	// 2. Perform text wrapping
	this->CachedLines = WrapText(this->CachedTextPtr, this->MaxLineWidth);

	if (this->CachedLines.empty())
	{
		this->CachedBgWidth = 0;
		this->CachedBgHeight = 0;
		this->IsLayoutDirty = false;
		return;
	}

	// 3. Calculate maximum line width and total height
	int maxLineW = 0;
	for (const auto& line : this->CachedLines)
	{
		RectangleStruct dims = Drawing::GetTextDimensions(line.c_str(), { 0,0 }, 0, 2, 0);
		if (dims.Width > maxLineW) maxLineW = dims.Width;
	}

	this->CachedBgWidth = maxLineW + (PADDINGX * 2);
	this->CachedBgHeight = (static_cast<int>(this->CachedLines.size()) * LINE_HEIGHT) + (PADDINGY * 2);

	// 4. Mark layout as up to date
	this->IsLayoutDirty = false;
}

void WaypointLabelClass::Draw()
{

	// ===== 1. Safety checks =====
	if (this->WaypointIndex < 0)
		return;
	if (!ScenarioClass::Instance->IsDefinedWaypoint(this->WaypointIndex))
		return;
	// I am not familiar with memory addresses, so I add this check to avoid wrong hook positions
	if (!TacticalClass::Instance || !DSurface::Composite || !RulesClass::Instance || !ScenarioClass::Instance)
		return;

	// ===== 1.5 Update layout cache if dirty =====
	if (this->IsLayoutDirty)
	{
		this->UpdateLayout();
	}

	// Skip drawing if cache is invalid
	if (this->CachedLines.empty() || this->CachedBgWidth <= 0 || this->CachedBgHeight <= 0)
		return;

	// ===== 2. Get waypoint position and validate =====
	CellStruct cell = ScenarioClass::Instance->GetWaypointCoords(this->WaypointIndex);
	if (cell.X < 0 && cell.Y < 0)
		return;

	CoordStruct coords = CellClass::Cell2Coord(cell);
	Point2D screenPos;
	if (!TacticalClass::Instance->CoordsToClient(&coords, &screenPos))
		return;

	int bgWidth = this->CachedBgWidth;
	int bgHeight = this->CachedBgHeight;

	// Calculate label top-left position
	Point2D topLeft = {
		screenPos.X - (bgWidth / 2),
		screenPos.Y - bgHeight - 10
	};

	int viewWidth = DSurface::ViewBounds.Width; // may be not useful
	int viewHeight = DSurface::ViewBounds.Height;

	// Calculate clipping boundary (prevent drawing over UI)
	int clipBottomY = viewHeight - BOTTOM_SAFE_HEIGHT;

	// Skip if label is entirely below safe area
	if (topLeft.Y >= clipBottomY)
	{
		return;
	}

	// Calculate draw height with clipping
	int drawHeight = bgHeight;
	bool isClipped = false;

	// Clip if label exceeds bottom safe area
	if (topLeft.Y + bgHeight > clipBottomY)
	{
		drawHeight = clipBottomY - topLeft.Y;
		isClipped = true;

		// Skip if clipped height is too small to render text
		if (drawHeight < LINE_HEIGHT)
		{
			return;
		}
	}

	// Sanity check: prevent drawing labels too far from waypoint
	// Dynamic snapping code was removed, so this check is unnecessary
	// 
	//int labelCenterX = topLeft.X + bgWidth / 2;
	//int labelCenterY = topLeft.Y + drawHeight / 2;
	//int distX = std::abs(labelCenterX - screenPos.X);
	//int distY = std::abs(labelCenterY - screenPos.Y);

	//if (distX > bgWidth * 2 || distY > bgHeight * 2)
	//{
	//	return;
	//}

	// Skip if label is outside viewport
	if (topLeft.X + bgWidth < 0 || topLeft.Y + drawHeight < 0)
	{
		return;
	}

	RectangleStruct bgRect = { topLeft.X, topLeft.Y, bgWidth, drawHeight };

	// Mouse hover check (use full unclipped bounds)
	Point2D mousePos = WWMouseClass::Instance->XY1;
	RectangleStruct fullBgRect = { topLeft.X, topLeft.Y, bgWidth, bgHeight };
	if (mousePos.X >= fullBgRect.X &&
		mousePos.X <= fullBgRect.X + fullBgRect.Width &&
		mousePos.Y >= fullBgRect.Y &&
		mousePos.Y <= fullBgRect.Y + fullBgRect.Height)
	{
		return;
	}

	// ===== 4. Draw semi-transparent background =====
	ColorStruct bgColor = { 0, 0, 0 };
	DSurface::Composite->FillRectTrans(&bgRect, &bgColor, this->BackgroundOpacity);

	// ===== 5. Draw border with clipping support =====
	int ColorInt = Drawing::RGB_To_Int(GetColorStructByEnum(this->Color));
	Point2D p1, p2;

	// draw each border line individually	_:(´□`」 ∠):_
	// Top border
	p1 = { topLeft.X, topLeft.Y };
	p2 = { topLeft.X + bgWidth - 1, topLeft.Y };
	DSurface::Composite->DrawLine(&p1, &p2, ColorInt);

	// Left border (clipped)
	p1 = { topLeft.X, topLeft.Y };
	p2 = { topLeft.X, topLeft.Y + drawHeight - 1 };
	DSurface::Composite->DrawLine(&p1, &p2, ColorInt);

	// Right border (clipped)
	p1 = { topLeft.X + bgWidth - 1, topLeft.Y };
	p2 = { topLeft.X + bgWidth - 1, topLeft.Y + drawHeight - 1 };
	DSurface::Composite->DrawLine(&p1, &p2, ColorInt);

	// Bottom border (only if not clipped)
	if (!isClipped)
	{
		p1 = { topLeft.X, topLeft.Y + bgHeight - 1 };
		p2 = { topLeft.X + bgWidth - 1, topLeft.Y + bgHeight - 1 };
		DSurface::Composite->DrawLine(&p1, &p2, ColorInt);
	}

	// ===== 6. Draw text lines with clipping =====
	RectangleStruct bounds = DSurface::ViewBounds;
	int currentY = topLeft.Y + PADDINGY;

	for (const auto& line : this->CachedLines)
	{
		// Stop drawing if line exceeds clipped area
		if (currentY + LINE_HEIGHT > topLeft.Y + drawHeight + PADDINGY)
		{
			break;
		}

		Point2D textPos = {
			topLeft.X + PADDINGX,
			currentY
		};

		DSurface::Composite->DrawText(
			line.c_str(),
			&bounds,
			&textPos,
			ColorInt,
			0,
			TextPrintType::Metal12 | TextPrintType::BrightColor
		);

		currentY += LINE_HEIGHT;
	}
}

void WaypointLabelClass::DrawAll()
{
	for (const auto& pLabel : WaypointLabelClass::Array)
	{
		if (pLabel) pLabel->Draw();
	}
	return;
}

// ============================================================
//             Interfaces for Phobos.Ext.cpp
// ============================================================

template <typename T>
bool WaypointLabelClass::Serialize(T& Stm)
{
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
        for (auto& pLabel : Array)
        {
            if (pLabel)
            {
                pLabel->IsLayoutDirty = true;
                pLabel->CachedTextPtr = nullptr;
                pLabel->CachedLines.clear();
            }
        }
    }

    return result;
}
