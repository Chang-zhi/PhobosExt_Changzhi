#pragma once

#include "Stream.h"

#include <memory>
#include <type_traits>

namespace Savegame
{
	template <typename T>
	bool ReadPhobosExtStream(PhobosExtStreamReader& Stm, T& Value, bool RegisterForChange = true);

	template <typename T>
	bool WritePhobosExtStream(PhobosExtStreamWriter& Stm, const T& Value);

	template <typename T>
	T* RestoreObject(PhobosExtStreamReader& Stm, bool RegisterForChange = true);

	template <typename T>
	bool PersistObject(PhobosExtStreamWriter& Stm, const T* pValue);

	template <typename T>
	struct PhobosExtStreamObject
	{
		bool ReadFromStream(PhobosExtStreamReader& Stm, T& Value, bool RegisterForChange) const;
		bool WriteToStream(PhobosExtStreamWriter& Stm, const T& Value) const;
	};

	template <typename T>
	struct ObjectFactory
	{
		std::unique_ptr<T> operator() (PhobosExtStreamReader& Stm) const
		{
			return std::make_unique<T>();
		}
	};
}
