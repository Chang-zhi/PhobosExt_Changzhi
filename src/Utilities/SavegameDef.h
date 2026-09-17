#pragma once

// include this file whenever something is to be saved.

#include "Savegame.h"
#include <optional>
#include <vector>
#include <map>
#include <bitset>
#include <memory>

#include <ArrayClasses.h>
#include <FileSystem.h>
#include <FileFormats/SHP.h>
#include <RulesClass.h>
#include <SidebarClass.h>
#include <Utilities/Constructs.h>

#include "Swizzle.h"
#include "Debug.h"

namespace Savegame
{
	template <typename T>
	concept ImplementsUpperCaseSaveLoad = requires (PhobosExtStreamWriter & stmWriter, PhobosExtStreamReader & stmReader, T & value, bool registerForChange)
	{
		value.Save(stmWriter);
		value.Load(stmReader, registerForChange);
	};

	template <typename T>
	concept ImplementsLowerCaseSaveLoad = requires (PhobosExtStreamWriter & stmWriter, PhobosExtStreamReader & stmReader, T & value, bool registerForChange)
	{
		value.save(stmWriter);
		value.load(stmReader, registerForChange);
	};

	#pragma warning(push)
	#pragma warning(disable: 4702) // MSVC isn't smart enough and yells about unreachable code

	template <typename T>
	bool ReadPhobosExtStream(PhobosExtStreamReader& stm, T& value, bool registerForChange)
	{
		if constexpr (ImplementsUpperCaseSaveLoad<T>)
			return value.Load(stm, registerForChange);

		else if constexpr (ImplementsLowerCaseSaveLoad<T>)
			return value.load(stm, registerForChange);

		PhobosExtStreamObject<T> item;
		return item.ReadFromStream(stm, value, registerForChange);
	}

	template <typename T>
	bool WritePhobosExtStream(PhobosExtStreamWriter& stm, const T& value)
	{
		if constexpr (ImplementsUpperCaseSaveLoad<T>)
			return value.Save(stm);

		if constexpr (ImplementsLowerCaseSaveLoad<T>)
			return value.save(stm);

		PhobosExtStreamObject<T> item;
		return item.WriteToStream(stm, value);
	}

	#pragma warning(pop)

	template <typename T>
	T* RestoreObject(PhobosExtStreamReader& Stm, bool RegisterForChange)
	{
		T* ptrOld = nullptr;
		if (!Stm.Load(ptrOld))
			return nullptr;

		if (ptrOld)
		{
			std::unique_ptr<T> ptrNew = ObjectFactory<T>()(Stm);

			if (Savegame::ReadPhobosExtStream(Stm, *ptrNew, RegisterForChange))
			{
				PhobosExtSwizzle::RegisterChange(ptrOld, ptrNew.get());
				return ptrNew.release();
			}
		}

		return nullptr;
	}

	template <typename T>
	bool PersistObject(PhobosExtStreamWriter& Stm, const T* pValue)
	{
		if (!Savegame::WritePhobosExtStream(Stm, pValue))
			return false;

		if (pValue)
			return Savegame::WritePhobosExtStream(Stm, *pValue);

		return true;
	}

	template <typename T>
	bool PhobosExtStreamObject<T>::ReadFromStream(PhobosExtStreamReader& Stm, T& Value, bool RegisterForChange) const
	{
		bool ret = Stm.Load(Value);

		if (RegisterForChange)
			Swizzle swizzle(Value);

		return ret;
	}

	template <typename T>
	bool PhobosExtStreamObject<T>::WriteToStream(PhobosExtStreamWriter& Stm, const T& Value) const
	{
		Stm.Save(Value);
		return true;
	}


	// specializations

	template <typename T>
	struct Savegame::PhobosExtStreamObject<VectorClass<T>>
	{
		bool ReadFromStream(PhobosExtStreamReader& Stm, VectorClass<T>& Value, bool RegisterForChange) const
		{
			Value.Clear();
			int Capacity = 0;

			if (!Stm.Load(Capacity))
				return false;

			Value.Reserve(Capacity);

			for (auto ix = 0; ix < Capacity; ++ix)
			{
				if (!Savegame::ReadPhobosExtStream(Stm, Value.Items[ix], RegisterForChange))
					return false;
			}

			return true;
		}

		bool WriteToStream(PhobosExtStreamWriter& Stm, const VectorClass<T>& Value) const
		{
			Stm.Save(Value.Capacity);

			for (auto ix = 0; ix < Value.Capacity; ++ix)
			{
				if (!Savegame::WritePhobosExtStream(Stm, Value.Items[ix]))
					return false;
			}

			return true;
		}
	};

	template <typename T>
	struct Savegame::PhobosExtStreamObject<DynamicVectorClass<T>>
	{
		bool ReadFromStream(PhobosExtStreamReader& Stm, DynamicVectorClass<T>& Value, bool RegisterForChange) const
		{
			Value.Clear();
			int Capacity = 0;

			if (!Stm.Load(Capacity))
				return false;

			Value.Reserve(Capacity);

			if (!Stm.Load(Value.Count) || !Stm.Load(Value.CapacityIncrement))
				return false;

			for (auto ix = 0; ix < Value.Count; ++ix)
			{
				if (!Savegame::ReadPhobosExtStream(Stm, Value.Items[ix], RegisterForChange))
					return false;
			}

			return true;
		}

		bool WriteToStream(PhobosExtStreamWriter& Stm, const DynamicVectorClass<T>& Value) const
		{
			Stm.Save(Value.Capacity);
			Stm.Save(Value.Count);
			Stm.Save(Value.CapacityIncrement);

			for (auto ix = 0; ix < Value.Count; ++ix)
			{
				if (!Savegame::WritePhobosExtStream(Stm, Value.Items[ix]))
					return false;
			}

			return true;
		}
	};

	template <typename T>
	struct Savegame::PhobosExtStreamObject<TypeList<T>>
	{
		bool ReadFromStream(PhobosExtStreamReader& Stm, TypeList<T>& Value, bool RegisterForChange) const
		{
			if (!Savegame::ReadPhobosExtStream<DynamicVectorClass<T>>(Stm, Value, RegisterForChange))
				return false;

			return Stm.Load(Value.unknown_18);
		}

		bool WriteToStream(PhobosExtStreamWriter& Stm, const TypeList<T>& Value) const
		{
			if (!Savegame::WritePhobosExtStream<DynamicVectorClass<T>>(Stm, Value))
				return false;

			Stm.Save(Value.unknown_18);
			return true;
		}
	};

	template <>
	struct Savegame::PhobosExtStreamObject<CounterClass>
	{
		bool ReadFromStream(PhobosExtStreamReader& Stm, CounterClass& Value, bool RegisterForChange) const
		{
			if (!Savegame::ReadPhobosExtStream<VectorClass<int>>(Stm, Value, RegisterForChange))
				return false;

			return Stm.Load(Value.Total);
		}

		bool WriteToStream(PhobosExtStreamWriter& Stm, const CounterClass& Value) const
		{
			if (!Savegame::WritePhobosExtStream<VectorClass<int>>(Stm, Value))
				return false;

			Stm.Save(Value.Total);
			return true;
		}
	};

	template <size_t Size>
	struct Savegame::PhobosExtStreamObject<std::bitset<Size>>
	{
		bool ReadFromStream(PhobosExtStreamReader& Stm, std::bitset<Size>& Value, bool RegisterForChange) const
		{
			unsigned char value = 0;
			for (auto i = 0u; i < Size; ++i)
			{
				auto pos = i % 8;

				if (pos == 0 && !Stm.Load(value))
					return false;

				Value.set(i, ((value >> pos) & 1) != 0);
			}

			return true;
		}

		bool WriteToStream(PhobosExtStreamWriter& Stm, const std::bitset<Size>& Value) const
		{
			unsigned char value = 0;
			for (auto i = 0u; i < Size; ++i)
			{
				auto pos = i % 8;

				if (Value[i])
					value |= 1 << pos;

				if (pos == 7 || i == Size - 1)
				{
					Stm.Save(value);
					value = 0;
				}
			}

			return true;
		}
	};

	template <>
	struct Savegame::PhobosExtStreamObject<std::string>
	{
		bool ReadFromStream(PhobosExtStreamReader& Stm, std::string& Value, bool RegisterForChange) const
		{
			size_t size = 0;

			if (Stm.Load(size))
			{
				std::vector<char> buffer(size);

				if (!size || Stm.Read(reinterpret_cast<byte*>(buffer.data()), size))
				{
					Value.assign(buffer.begin(), buffer.end());
					return true;
				}
			}
			return false;
		}

		bool WriteToStream(PhobosExtStreamWriter& Stm, const std::string& Value) const
		{
			Stm.Save(Value.size());
			Stm.Write(reinterpret_cast<const byte*>(Value.c_str()), Value.size());

			return true;
		}
	};

	template <typename T>
	struct Savegame::PhobosExtStreamObject<std::unique_ptr<T>>
	{
		bool ReadFromStream(PhobosExtStreamReader& Stm, std::unique_ptr<T>& Value, bool RegisterForChange) const
		{
			Value.reset(RestoreObject<T>(Stm, RegisterForChange));
			return true;
		}

		bool WriteToStream(PhobosExtStreamWriter& Stm, const std::unique_ptr<T>& Value) const
		{
			return PersistObject(Stm, Value.get());
		}
	};

	template <typename T>
	struct Savegame::PhobosExtStreamObject<std::optional<T>>
	{
		bool ReadFromStream(PhobosExtStreamReader& Stm, std::optional<T>& Value, bool RegisterForChange) const
		{
			bool hasValue = false;
			if (!Stm.Load(hasValue))
				return false;

			if (hasValue)
				return Savegame::ReadPhobosExtStream(Stm, *Value, RegisterForChange);
			else
				Value.reset();

			return true;
		}

		bool WriteToStream(PhobosExtStreamWriter& Stm, const std::optional<T>& Value) const
		{
			Stm.Save(Value.has_value());

			if (Value.has_value())
				return Savegame::WritePhobosExtStream(Stm, *Value);

			return true;
		}
	};

	template <typename T>
	struct Savegame::PhobosExtStreamObject<std::vector<T>>
	{
		bool ReadFromStream(PhobosExtStreamReader& Stm, std::vector<T>& Value, bool RegisterForChange) const
		{
			Value.clear();

			size_t Capacity = 0;

			if (!Stm.Load(Capacity))
				return false;

			Value.reserve(Capacity);

			size_t Count = 0;

			if (!Stm.Load(Count))
				return false;

			Value.resize(Count);

			for (auto ix = 0u; ix < Count; ++ix)
			{
				if (!Savegame::ReadPhobosExtStream(Stm, Value[ix], RegisterForChange))
					return false;
			}

			return true;
		}

		bool WriteToStream(PhobosExtStreamWriter& Stm, const std::vector<T>& Value) const
		{
			Stm.Save(Value.capacity());
			Stm.Save(Value.size());

			for (auto ix = 0u; ix < Value.size(); ++ix)
			{
				if (!Savegame::WritePhobosExtStream(Stm, Value[ix]))
					return false;
			}

			return true;
		}
	};

	template <typename TKey, typename TValue>
	struct Savegame::PhobosExtStreamObject<std::map<TKey, TValue>>
	{
		bool ReadFromStream(PhobosExtStreamReader& Stm, std::map<TKey, TValue>& Value, bool RegisterForChange) const
		{
			Value.clear();
			static_assert(!std::is_pointer_v<TKey> && !std::is_pointer_v<TValue>);
			static_assert(std::is_trivially_constructible_v<TKey> && std::is_trivially_constructible_v<TValue>);
			static_assert(std::is_trivially_destructible_v<TKey> && std::is_trivially_destructible_v<TValue>);
			size_t Count = 0;
			if (!Stm.Load(Count))
			{
				return false;
			}

			for (auto ix = 0u; ix < Count; ++ix)
			{
				std::pair<TKey, TValue> buffer;
				if (!Savegame::ReadPhobosExtStream(Stm, buffer, RegisterForChange))
				{
					return false;
				}
				Value.insert(buffer);
			}

			return true;
		}

		bool WriteToStream(PhobosExtStreamWriter& Stm, const std::map<TKey, TValue>& Value) const
		{
			Stm.Save(Value.size());

			for (const auto& item : Value)
			{
				if (!Savegame::WritePhobosExtStream(Stm, item))
				{
					return false;
				}
			}
			return true;
		}
	};

	template <typename TKey, typename TValue> // Why are you doing this, choom
	struct Savegame::PhobosExtStreamObject<std::map<TKey, std::vector<TValue>>>
	{
		bool ReadFromStream(PhobosExtStreamReader& Stm, std::map<TKey, std::vector<TValue>>& Value, bool RegisterForChange) const
		{
			Value.clear();

			size_t Count = 0;
			if (!Stm.Load(Count))
			{
				return false;
			}

			for (auto ix = 0u; ix < Count; ++ix)
			{
				TKey key;
				std::vector<TValue> vals;
				if (!(Savegame::ReadPhobosExtStream(Stm, key, RegisterForChange)&& Savegame::ReadPhobosExtStream(Stm, vals, RegisterForChange)))
				{
					return false;
				}
				Value[key] = vals;
			}

			return true;
		}
		bool WriteToStream(PhobosExtStreamWriter& Stm, const std::map<TKey, std::vector<TValue>>& Value) const
		{
			Stm.Save(Value.size());

			for (const auto& [key,vals] : Value)
			{
				if (!(Savegame::WritePhobosExtStream(Stm, key) && Savegame::WritePhobosExtStream(Stm,vals)))
				{
					return false;
				}
			}
			return true;
		};
	};

	template <>
	struct Savegame::PhobosExtStreamObject<SHPStruct*>
	{
		bool ReadFromStream(PhobosExtStreamReader& Stm, SHPStruct*& Value, bool RegisterForChange) const
		{
			if (Value && !Value->IsReference())
				Debug::Log("Value contains SHP file data. Possible leak.\n");

			Value = nullptr;

			bool hasValue = true;
			if (Savegame::ReadPhobosExtStream(Stm, hasValue) && hasValue)
			{
				std::string name;
				if (Savegame::ReadPhobosExtStream(Stm, name))
				{
					if (auto pSHP = FileSystem::LoadSHPFile(name.c_str()))
					{
						Value = pSHP;
						return true;
					}
				}
			}

			return !hasValue;
		}

		bool WriteToStream(PhobosExtStreamWriter& Stm, SHPStruct* const& Value) const
		{
			const char* filename = nullptr;
			if (Value)
			{
				if (auto pRef = Value->AsReference())
					filename = pRef->Filename;
				else
					Debug::Log("Cannot save SHPStruct, because it isn't a reference.\n");
			}

			if (Savegame::WritePhobosExtStream(Stm, filename != nullptr))
			{
				if (filename)
				{
					std::string file(filename);
					return Savegame::WritePhobosExtStream(Stm, file);
				}
			}

			return filename == nullptr;
		}
	};

	template <>
	struct Savegame::PhobosExtStreamObject<RocketStruct>
	{
		bool ReadFromStream(PhobosExtStreamReader& Stm, RocketStruct& Value, bool RegisterForChange) const
		{
			if (!Stm.Load(Value))
				return false;

			if (RegisterForChange)
				Swizzle swizzle(Value.Type);

			return true;
		}

		bool WriteToStream(PhobosExtStreamWriter& Stm, const RocketStruct& Value) const
		{
			Stm.Save(Value);
			return true;
		}
	};

	template <>
	struct Savegame::PhobosExtStreamObject<BuildType>
	{
		bool ReadFromStream(PhobosExtStreamReader& Stm, BuildType& Value, bool RegisterForChange) const
		{
			if (!Stm.Load(Value))
				return false;

			if (RegisterForChange)
				Swizzle swizzle(Value.CurrentFactory);

			return true;
		}

		bool WriteToStream(PhobosExtStreamWriter& Stm, const BuildType& Value) const
		{
			Stm.Save(Value);
			return true;
		}
	};

	template <>
	struct Savegame::PhobosExtStreamObject<TranslucencyLevel*>
	{
		bool ReadFromStream(PhobosExtStreamReader& Stm, TranslucencyLevel*& Value, bool RegisterForChange) const
		{
			return Value->Load(Stm, RegisterForChange);
		}

		bool WriteToStream(PhobosExtStreamWriter& Stm, TranslucencyLevel* const& Value) const
		{
			return Value->Save(Stm);
		}
	};
}
