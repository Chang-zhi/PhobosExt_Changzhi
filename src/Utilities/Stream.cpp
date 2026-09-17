#include "Stream.h"
#include "Debug.h"

#include <SwizzleManagerClass.h>

#include <Objidl.h>

PhobosExtByteStream::PhobosExtByteStream(size_t Reserve) : Data(), CurrentOffset(0)
{
	this->Data.reserve(Reserve);
}

PhobosExtByteStream::~PhobosExtByteStream() = default;

bool PhobosExtByteStream::ReadFromStream(IStream* pStm, const size_t Length)
{
	auto size = this->Data.size();
	this->Data.resize(size + Length);
	auto pv = reinterpret_cast<void*>(this->Data.data());

	ULONG out = 0;
	auto success = pStm->Read(pv, Length, &out);
	bool result(SUCCEEDED(success) && out == Length);

	if (!result)
		this->Data.resize(size);

	return result;
}

bool PhobosExtByteStream::WriteToStream(IStream* pStm) const
{
	const size_t Length(this->Data.size());
	auto pcv = reinterpret_cast<const void*>(this->Data.data());

	ULONG out = 0;
	auto success = pStm->Write(pcv, Length, &out);

	return SUCCEEDED(success) && out == Length;
}

bool PhobosExtByteStream::Read(data_t* Value, size_t Size)
{
	bool ret = false;

	if (this->Data.size() >= this->CurrentOffset + Size)
	{
		auto Position = &this->Data[this->CurrentOffset];
		std::memcpy(Value, Position, Size);
		ret = true;
	}

	this->CurrentOffset += Size;
	return ret;
}

void PhobosExtByteStream::Write(const data_t* Value, size_t Size)
{
	this->Data.insert(this->Data.end(), Value, Value + Size);
}

size_t PhobosExtByteStream::ReadBlockFromStream(IStream* pStm)
{
	ULONG out = 0;
	size_t Length = 0;

	if (SUCCEEDED(pStm->Read(&Length, sizeof(Length), &out)))
	{
		if (this->ReadFromStream(pStm, Length))
			return Length;
	}

	return 0;
}

bool PhobosExtByteStream::WriteBlockToStream(IStream* pStm) const
{
	ULONG out = 0;
	const size_t Length = this->Data.size();

	if (SUCCEEDED(pStm->Write(&Length, sizeof(Length), &out)))
		return this->WriteToStream(pStm);

	return false;
}

bool PhobosExtStreamReader::RegisterChange(void* newPtr)
{
	static_assert(sizeof(long) == sizeof(void*), "long and void* need to be of same size.");

	long oldPtr = 0;
	if (this->Load(oldPtr))
	{
		if (SUCCEEDED(SwizzleManagerClass::Instance.Here_I_Am(oldPtr, newPtr)))
			return true;

		this->EmitSwizzleWarning(oldPtr, newPtr, stream_debugging_t());
	}

	return false;
}

void PhobosExtStreamReader::EmitExpectEndOfBlockWarning(std::true_type) const
{
	Debug::Log("PhobosExtStreamReader - Read %X bytes instead of %X!\n",
		this->stream->Offset(), this->stream->Size());
}

void PhobosExtStreamReader::EmitLoadWarning(size_t size, std::true_type) const
{
	Debug::Log("PhobosExtStreamReader - Could not read data of length %u at %X of %X.\n",
		size, this->stream->Offset() - size, this->stream->Size());
}

void PhobosExtStreamReader::EmitExpectWarning(unsigned int found, unsigned int expect, std::true_type) const
{
	Debug::Log("PhobosExtStreamReader - Found %X, expected %X\n", found, expect);
}

void PhobosExtStreamReader::EmitSwizzleWarning(long id, void* pointer, std::true_type) const
{
	Debug::Log("PhobosExtStreamReader - Could not register change from %X to %p\n", id, pointer);
}
