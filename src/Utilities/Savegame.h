#pragma once

#include "Stream.h"

#include <memory>
#include <type_traits>

namespace Savegame
{
	template <typename T>
	bool ReadScaffoldStream(ScaffoldStreamReader& Stm, T& Value, bool RegisterForChange = true);

	template <typename T>
	bool WriteScaffoldStream(ScaffoldStreamWriter& Stm, const T& Value);

	template <typename T>
	T* RestoreObject(ScaffoldStreamReader& Stm, bool RegisterForChange = true);

	template <typename T>
	bool PersistObject(ScaffoldStreamWriter& Stm, const T* pValue);

	template <typename T>
	struct ScaffoldStreamObject
	{
		bool ReadFromStream(ScaffoldStreamReader& Stm, T& Value, bool RegisterForChange) const;
		bool WriteToStream(ScaffoldStreamWriter& Stm, const T& Value) const;
	};

	template <typename T>
	struct ObjectFactory
	{
		std::unique_ptr<T> operator() (ScaffoldStreamReader& Stm) const
		{
			return std::make_unique<T>();
		}
	};
}
