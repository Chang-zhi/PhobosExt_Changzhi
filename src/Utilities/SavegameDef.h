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
	concept ImplementsUpperCaseSaveLoad = requires (ScaffoldStreamWriter & stmWriter, ScaffoldStreamReader & stmReader, T & value, bool registerForChange)
	{
		value.Save(stmWriter);
		value.Load(stmReader, registerForChange);
	};

	template <typename T>
	concept ImplementsLowerCaseSaveLoad = requires (ScaffoldStreamWriter & stmWriter, ScaffoldStreamReader & stmReader, T & value, bool registerForChange)
	{
		value.save(stmWriter);
		value.load(stmReader, registerForChange);
	};

	#pragma warning(push)
	#pragma warning(disable: 4702) // MSVC isn't smart enough and yells about unreachable code

	template <typename T>
	bool ReadScaffoldStream(ScaffoldStreamReader& stm, T& value, bool registerForChange)
	{
		if constexpr (ImplementsUpperCaseSaveLoad<T>)
			return value.Load(stm, registerForChange);

		else if constexpr (ImplementsLowerCaseSaveLoad<T>)
			return value.load(stm, registerForChange);

		ScaffoldStreamObject<T> item;
		return item.ReadFromStream(stm, value, registerForChange);
	}

	template <typename T>
	bool WriteScaffoldStream(ScaffoldStreamWriter& stm, const T& value)
	{
		if constexpr (ImplementsUpperCaseSaveLoad<T>)
			return value.Save(stm);

		if constexpr (ImplementsLowerCaseSaveLoad<T>)
			return value.save(stm);

		ScaffoldStreamObject<T> item;
		return item.WriteToStream(stm, value);
	}

	#pragma warning(pop)

	template <typename T>
	T* RestoreObject(ScaffoldStreamReader& Stm, bool RegisterForChange)
	{
		T* ptrOld = nullptr;
		if (!Stm.Load(ptrOld))
			return nullptr;

		if (ptrOld)
		{
			std::unique_ptr<T> ptrNew = ObjectFactory<T>()(Stm);

			if (Savegame::ReadScaffoldStream(Stm, *ptrNew, RegisterForChange))
			{
				ScaffoldSwizzle::RegisterChange(ptrOld, ptrNew.get());
				return ptrNew.release();
			}
		}

		return nullptr;
	}

	template <typename T>
	bool PersistObject(ScaffoldStreamWriter& Stm, const T* pValue)
	{
		if (!Savegame::WriteScaffoldStream(Stm, pValue))
			return false;

		if (pValue)
			return Savegame::WriteScaffoldStream(Stm, *pValue);

		return true;
	}

	template <typename T>
	bool ScaffoldStreamObject<T>::ReadFromStream(ScaffoldStreamReader& Stm, T& Value, bool RegisterForChange) const
	{
		bool ret = Stm.Load(Value);

		if (RegisterForChange)
			Swizzle swizzle(Value);

		return ret;
	}

	template <typename T>
	bool ScaffoldStreamObject<T>::WriteToStream(ScaffoldStreamWriter& Stm, const T& Value) const
	{
		Stm.Save(Value);
		return true;
	}


	// specializations

	template <typename T>
	struct Savegame::ScaffoldStreamObject<VectorClass<T>>
	{
		bool ReadFromStream(ScaffoldStreamReader& Stm, VectorClass<T>& Value, bool RegisterForChange) const
		{
			Value.Clear();
			int Capacity = 0;

			if (!Stm.Load(Capacity))
				return false;

			Value.Reserve(Capacity);

			for (auto ix = 0; ix < Capacity; ++ix)
			{
				if (!Savegame::ReadScaffoldStream(Stm, Value.Items[ix], RegisterForChange))
					return false;
			}

			return true;
		}

		bool WriteToStream(ScaffoldStreamWriter& Stm, const VectorClass<T>& Value) const
		{
			Stm.Save(Value.Capacity);

			for (auto ix = 0; ix < Value.Capacity; ++ix)
			{
				if (!Savegame::WriteScaffoldStream(Stm, Value.Items[ix]))
					return false;
			}

			return true;
		}
	};

	template <typename T>
	struct Savegame::ScaffoldStreamObject<DynamicVectorClass<T>>
	{
		bool ReadFromStream(ScaffoldStreamReader& Stm, DynamicVectorClass<T>& Value, bool RegisterForChange) const
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
				if (!Savegame::ReadScaffoldStream(Stm, Value.Items[ix], RegisterForChange))
					return false;
			}

			return true;
		}

		bool WriteToStream(ScaffoldStreamWriter& Stm, const DynamicVectorClass<T>& Value) const
		{
			Stm.Save(Value.Capacity);
			Stm.Save(Value.Count);
			Stm.Save(Value.CapacityIncrement);

			for (auto ix = 0; ix < Value.Count; ++ix)
			{
				if (!Savegame::WriteScaffoldStream(Stm, Value.Items[ix]))
					return false;
			}

			return true;
		}
	};

	template <typename T>
	struct Savegame::ScaffoldStreamObject<TypeList<T>>
	{
		bool ReadFromStream(ScaffoldStreamReader& Stm, TypeList<T>& Value, bool RegisterForChange) const
		{
			if (!Savegame::ReadScaffoldStream<DynamicVectorClass<T>>(Stm, Value, RegisterForChange))
				return false;

			return Stm.Load(Value.unknown_18);
		}

		bool WriteToStream(ScaffoldStreamWriter& Stm, const TypeList<T>& Value) const
		{
			if (!Savegame::WriteScaffoldStream<DynamicVectorClass<T>>(Stm, Value))
				return false;

			Stm.Save(Value.unknown_18);
			return true;
		}
	};

	template <>
	struct Savegame::ScaffoldStreamObject<CounterClass>
	{
		bool ReadFromStream(ScaffoldStreamReader& Stm, CounterClass& Value, bool RegisterForChange) const
		{
			if (!Savegame::ReadScaffoldStream<VectorClass<int>>(Stm, Value, RegisterForChange))
				return false;

			return Stm.Load(Value.Total);
		}

		bool WriteToStream(ScaffoldStreamWriter& Stm, const CounterClass& Value) const
		{
			if (!Savegame::WriteScaffoldStream<VectorClass<int>>(Stm, Value))
				return false;

			Stm.Save(Value.Total);
			return true;
		}
	};

	template <size_t Size>
	struct Savegame::ScaffoldStreamObject<std::bitset<Size>>
	{
		bool ReadFromStream(ScaffoldStreamReader& Stm, std::bitset<Size>& Value, bool RegisterForChange) const
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

		bool WriteToStream(ScaffoldStreamWriter& Stm, const std::bitset<Size>& Value) const
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
	struct Savegame::ScaffoldStreamObject<std::string>
	{
		bool ReadFromStream(ScaffoldStreamReader& Stm, std::string& Value, bool RegisterForChange) const
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

		bool WriteToStream(ScaffoldStreamWriter& Stm, const std::string& Value) const
		{
			Stm.Save(Value.size());
			Stm.Write(reinterpret_cast<const byte*>(Value.c_str()), Value.size());

			return true;
		}
	};

	template <typename T>
	struct Savegame::ScaffoldStreamObject<std::unique_ptr<T>>
	{
		bool ReadFromStream(ScaffoldStreamReader& Stm, std::unique_ptr<T>& Value, bool RegisterForChange) const
		{
			Value.reset(RestoreObject<T>(Stm, RegisterForChange));
			return true;
		}

		bool WriteToStream(ScaffoldStreamWriter& Stm, const std::unique_ptr<T>& Value) const
		{
			return PersistObject(Stm, Value.get());
		}
	};

	template <typename T>
	struct Savegame::ScaffoldStreamObject<std::optional<T>>
	{
		bool ReadFromStream(ScaffoldStreamReader& Stm, std::optional<T>& Value, bool RegisterForChange) const
		{
			bool hasValue = false;
			if (!Stm.Load(hasValue))
				return false;

			if (hasValue)
				return Savegame::ReadScaffoldStream(Stm, *Value, RegisterForChange);
			else
				Value.reset();

			return true;
		}

		bool WriteToStream(ScaffoldStreamWriter& Stm, const std::optional<T>& Value) const
		{
			Stm.Save(Value.has_value());

			if (Value.has_value())
				return Savegame::WriteScaffoldStream(Stm, *Value);

			return true;
		}
	};

	template <typename T>
	struct Savegame::ScaffoldStreamObject<std::vector<T>>
	{
		bool ReadFromStream(ScaffoldStreamReader& Stm, std::vector<T>& Value, bool RegisterForChange) const
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
				if (!Savegame::ReadScaffoldStream(Stm, Value[ix], RegisterForChange))
					return false;
			}

			return true;
		}

		bool WriteToStream(ScaffoldStreamWriter& Stm, const std::vector<T>& Value) const
		{
			Stm.Save(Value.capacity());
			Stm.Save(Value.size());

			for (auto ix = 0u; ix < Value.size(); ++ix)
			{
				if (!Savegame::WriteScaffoldStream(Stm, Value[ix]))
					return false;
			}

			return true;
		}
	};

	template <typename TKey, typename TValue>
	struct Savegame::ScaffoldStreamObject<std::map<TKey, TValue>>
	{
		bool ReadFromStream(ScaffoldStreamReader& Stm, std::map<TKey, TValue>& Value, bool RegisterForChange) const
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
				if (!Savegame::ReadScaffoldStream(Stm, buffer, RegisterForChange))
				{
					return false;
				}
				Value.insert(buffer);
			}

			return true;
		}

		bool WriteToStream(ScaffoldStreamWriter& Stm, const std::map<TKey, TValue>& Value) const
		{
			Stm.Save(Value.size());

			for (const auto& item : Value)
			{
				if (!Savegame::WriteScaffoldStream(Stm, item))
				{
					return false;
				}
			}
			return true;
		}
	};

	template <typename TKey, typename TValue> // Why are you doing this, choom
	struct Savegame::ScaffoldStreamObject<std::map<TKey, std::vector<TValue>>>
	{
		bool ReadFromStream(ScaffoldStreamReader& Stm, std::map<TKey, std::vector<TValue>>& Value, bool RegisterForChange) const
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
				if (!(Savegame::ReadScaffoldStream(Stm, key, RegisterForChange)&& Savegame::ReadScaffoldStream(Stm, vals, RegisterForChange)))
				{
					return false;
				}
				Value[key] = vals;
			}

			return true;
		}
		bool WriteToStream(ScaffoldStreamWriter& Stm, const std::map<TKey, std::vector<TValue>>& Value) const
		{
			Stm.Save(Value.size());

			for (const auto& [key,vals] : Value)
			{
				if (!(Savegame::WriteScaffoldStream(Stm, key) && Savegame::WriteScaffoldStream(Stm,vals)))
				{
					return false;
				}
			}
			return true;
		};
	};

	template <>
	struct Savegame::ScaffoldStreamObject<SHPStruct*>
	{
		bool ReadFromStream(ScaffoldStreamReader& Stm, SHPStruct*& Value, bool RegisterForChange) const
		{
			if (Value && !Value->IsReference())
				Debug::Log("Value contains SHP file data. Possible leak.\n");

			Value = nullptr;

			bool hasValue = true;
			if (Savegame::ReadScaffoldStream(Stm, hasValue) && hasValue)
			{
				std::string name;
				if (Savegame::ReadScaffoldStream(Stm, name))
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

		bool WriteToStream(ScaffoldStreamWriter& Stm, SHPStruct* const& Value) const
		{
			const char* filename = nullptr;
			if (Value)
			{
				if (auto pRef = Value->AsReference())
					filename = pRef->Filename;
				else
					Debug::Log("Cannot save SHPStruct, because it isn't a reference.\n");
			}

			if (Savegame::WriteScaffoldStream(Stm, filename != nullptr))
			{
				if (filename)
				{
					std::string file(filename);
					return Savegame::WriteScaffoldStream(Stm, file);
				}
			}

			return filename == nullptr;
		}
	};

	template <>
	struct Savegame::ScaffoldStreamObject<RocketStruct>
	{
		bool ReadFromStream(ScaffoldStreamReader& Stm, RocketStruct& Value, bool RegisterForChange) const
		{
			if (!Stm.Load(Value))
				return false;

			if (RegisterForChange)
				Swizzle swizzle(Value.Type);

			return true;
		}

		bool WriteToStream(ScaffoldStreamWriter& Stm, const RocketStruct& Value) const
		{
			Stm.Save(Value);
			return true;
		}
	};

	template <>
	struct Savegame::ScaffoldStreamObject<BuildType>
	{
		bool ReadFromStream(ScaffoldStreamReader& Stm, BuildType& Value, bool RegisterForChange) const
		{
			if (!Stm.Load(Value))
				return false;

			if (RegisterForChange)
				Swizzle swizzle(Value.CurrentFactory);

			return true;
		}

		bool WriteToStream(ScaffoldStreamWriter& Stm, const BuildType& Value) const
		{
			Stm.Save(Value);
			return true;
		}
	};

	template <>
	struct Savegame::ScaffoldStreamObject<TranslucencyLevel*>
	{
		bool ReadFromStream(ScaffoldStreamReader& Stm, TranslucencyLevel*& Value, bool RegisterForChange) const
		{
			return Value->Load(Stm, RegisterForChange);
		}

		bool WriteToStream(ScaffoldStreamWriter& Stm, TranslucencyLevel* const& Value) const
		{
			return Value->Save(Stm);
		}
	};
}
