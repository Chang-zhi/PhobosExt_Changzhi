// ======================================================================
//					Custom Waypoint Label Display
//					Class: WaypointLabelClass
//					Author: Chang_zhi
// ======================================================================

#pragma once

#include <Phobos.h>
#include <Utilities/SavegameDef.h>

#include <string>
#include <vector>
#include <memory>

// front declare
class PhobosStreamWriter;
class PhobosStreamReader;

namespace WaypointLabel
{ 
    enum Color : unsigned int {
        gold = 0x0,         // RGB(255,215,0)
        white = 0x1,        // RGB(255,255,255)
		red = 0x2,          // RGB(255,0,0)
		blue = 0x3,         // RGB(0,0,255)
		green = 0x4,        // RGB(0,128,0)
		yellow = 0x5,       // RGB(255,255,0)
		purple = 0x6,       // RGB(128,0,128)
		pink = 0x7,         // RGB(255,192,203)
		lightblue = 0x8,    // RGB(173, 216, 230)
        Count
    };
}

class WaypointLabelClass
{
public:
    static std::vector<std::unique_ptr<WaypointLabelClass>> Array;

    int WaypointIndex { -1 };  
    std::string CurrentLabel; 
    int MaxLineWidth { 250 };
    int BackgroundOpacity { 75 };
    WaypointLabel::Color Color { WaypointLabel::Color::gold };

	// Constructor
    WaypointLabelClass() = default;
    WaypointLabelClass(int wpIndex, const char* csfLabel, int maxWidth = 250, int opacityPercent = 75, WaypointLabel::Color color = WaypointLabel::Color::gold);

    void Draw();
	static void DrawAll();
    void UpdateLayout();

	// Interfaces for Actions
    static void SetLabel(int wpIndex, const char* csfLabel, int maxWidth = 250, int opacityPercent = 75, WaypointLabel::Color color = WaypointLabel::Color::gold);
    static void ClearLabel(int wpIndex);
    static void ClearAll();

	// Interfaces for Phobos.Ext.cpp
    bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
    bool Save(PhobosStreamWriter& Stm) const;

    static bool SaveGlobals(PhobosStreamWriter& Stm);
    static bool LoadGlobals(PhobosStreamReader& Stm);

private:
	// Interfaces for Phobos.Ext.cpp
    template <typename T>
    bool Serialize(T& Stm);

    // Cached values to improve performance
    bool IsLayoutDirty{ true };               
    std::vector<std::wstring> CachedLines;    
    int CachedBgWidth{ 0 };                   
    int CachedBgHeight{ 0 };                  
    const wchar_t* CachedTextPtr{ nullptr };  
};
