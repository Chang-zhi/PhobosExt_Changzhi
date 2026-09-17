#pragma once

#include <Phobos.h>
#include <GeneralStructures.h>
#include <Utilities/SavegameDef.h>

#include <string>
#include <vector>
#include <memory>

class PhobosStreamWriter;
class PhobosStreamReader;

class MapTextBoxClass
{
public:
	std::string CurrentLabel;
	int MaxLineWidth { 250 };
	int BackgroundOpacity { 75 };
	int ColorR { 255 };
	int ColorG { 215 };
	int ColorB { 0 };

	MapTextBoxClass(const MapTextBoxClass&) = delete;
	MapTextBoxClass& operator=(const MapTextBoxClass&) = delete;

	virtual ~MapTextBoxClass();

	void DrawAt(Point2D centerPos);
	void UpdateLayout();
	void ResetCache();

	virtual bool CanDraw() const;
	virtual bool GetDrawPosition(Point2D& outPos) const = 0;
	virtual const char* GetTypeMarker() const = 0;

	static std::vector<std::shared_ptr<MapTextBoxClass>> Array;

	static void DrawAll();
	static void ClearAll();
	static void Clear();
	static bool SaveGlobals(PhobosStreamWriter& Stm);
	static bool LoadGlobals(PhobosStreamReader& Stm);

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
	virtual bool Save(PhobosStreamWriter& Stm) const;

protected:
	MapTextBoxClass() = default;
	MapTextBoxClass(const char* csfLabel,
				  int maxWidth = 250, int opacityPercent = 75,
				  int colorR = 255, int colorG = 215, int colorB = 0);

	int VerticalOffset { 0 };

	// Remaining display frames (-1 = manual control, 0 = expired)
	int RemainingFrames { -1 };

	template <typename T>
	bool Serialize(T& Stm);

private:
	struct Cache
	{
		bool IsLayoutDirty { true };
		std::vector<std::wstring> CachedLines;
		int CachedBgWidth { 0 };
		int CachedBgHeight { 0 };
	};
	std::unique_ptr<Cache> m_cache;
};
