#pragma once

#include "../Base/MapTextBoxClass.h"

#include <Phobos.h>
#include <Utilities/SavegameDef.h>

#include <string>
#include <vector>
#include <memory>

class PhobosStreamWriter;
class PhobosStreamReader;

class TechnoTextBoxClass final : public MapTextBoxClass
{
public:
	static std::vector<std::shared_ptr<TechnoTextBoxClass>> Array;

	TechnoClass* Target { nullptr };
	DWORD SavedTargetUID { 0 };

	TechnoTextBoxClass() = default;
	TechnoTextBoxClass(TechnoClass* pTarget, const char* csfLabel, const char* typeName);

	bool CanDraw() const override;
	bool GetDrawPosition(Point2D& outPos) const override;
	const char* GetTypeMarker() const override { return "TechnoTextBoxClass"; }

	static TechnoTextBoxClass* FindOrCreate(TechnoClass* pTarget,
		const char* csfLabel, const char* typeName);
	static TechnoTextBoxClass* Find(TechnoClass* pTarget);
	static void Remove(TechnoClass* pTarget);
	static void RemoveByType(int typeIndex);
	static void RemoveByTrigger(TriggerClass* pTrigger);
	static void RemoveByTeam(int teamIndex);
	static void ClearAll();
	static void Clear();
	static void CleanupDeadLabels();

	bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	bool Save(PhobosStreamWriter& Stm) const override;

private:
	template <typename T>
	bool Serialize(T& Stm);
};
