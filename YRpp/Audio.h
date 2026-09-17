#pragma once

#include <GeneralDefinitions.h>
#include <Helpers/CompileTime.h>

class RawFileClass;
class CCFileClass;
class VocClass;

struct AudioIDXHeader {
	unsigned int Magic;
	unsigned int Version;
	unsigned int numSamples;
};

struct AudioIDXEntry { // assert (IDXHeader.version != 1);
	char Name[16];
	int Offset;
	int Size;
	unsigned int SampleRate;
	unsigned int Flags;
	unsigned int ChunkSize;

	bool operator < (const AudioIDXEntry& rhs) const {
		return _strcmpi(this->Name, rhs.Name) < 0;
	}
};

struct AudioSampleData {
	AudioSampleData() :
		Data(0),
		Format(0),
		SampleRate(0),
		NumChannels(0),
		BytesPerSample(0),
		ByteRate(0),
		BlockAlign(0),
		Flags(0)
	{ }

	unsigned int Data;
	unsigned int Format;
	unsigned int SampleRate;
	unsigned int NumChannels;
	unsigned int BytesPerSample;
	unsigned int ByteRate;
	unsigned int BlockAlign;
	unsigned int Flags;
};

class AudioIDXData {
public:
	DEFINE_REFERENCE(AudioIDXData*, Instance, 0x87E294u)

	static AudioIDXData* __fastcall Create(const char* pFilename, const char* pPath)
		{ JMP_STD(0x4011C0); };

	AudioIDXData() {
		memset(this, 0, sizeof(*this));
	}

	~AudioIDXData()
		{ JMP_THIS(0x401580); }

	void ClearCurrentSample()
		{ JMP_THIS(0x401910); }

	int __fastcall FindSampleIndex(const char* pName) const
		{ JMP_STD(0x4015C0); }

	const char* __fastcall GetSampleName(int index) const
		{ JMP_STD(0x401600); }

	int __fastcall GetSampleSize(int index) const
		{ JMP_STD(0x401620); }

	AudioSampleData* __fastcall GetSampleInformation(int index, AudioSampleData* pBuffer) const
		{ JMP_STD(0x401640); }

	AudioIDXEntry* Samples;
	int SampleCount;
	char Path[MAX_PATH];
	CCFileClass* BagFile;
	RawFileClass* ExternalFile;
	BOOL PathFound;
	RawFileClass* CurrentSampleFile;
	int CurrentSampleSize;
	DWORD unknown_120;
};

class Audio {
public:

	static bool __fastcall ReadWAVFile(RawFileClass* pFile, AudioSampleData* pAudioSample, int* pDataSize)
		{ JMP_STD(0x408610); }
};

class AudioStream {
public:
	DEFINE_REFERENCE(AudioStream*, Instance, 0xB1D4D8u)

	bool __fastcall PlayWAV(const char* pFilename, bool bUnk)
		{ JMP_STD(0x407B60); }
};

struct TauntDataStruct {
	DWORD tauntIdx : 4;
	DWORD countryIdx : 4;
};

struct AudioController
{
	void* Event; // Pointer to audio event (AudioEventTag) associated with this controller, type not implemented in YRpp as of current.
	DWORD Stamp;
	VocClass* EventType;
	AudioIDXData** AudioIndex;
	DWORD Unused;

	AudioController() :
		Event(nullptr),
		Stamp(0),
		EventType(nullptr),
		AudioIndex(&AudioIDXData::Instance),
		Unused(0)
	{ }

	~AudioController()
		{ JMP_THIS(0x405C00); }

	// Stops the audio event associated with this controller instantly.
	void Stop()
		{ JMP_THIS(0x405D40); }

	// Ends the audio event associated with this controller, allowing it to fade out.
	void End()
		{ JMP_THIS(0x405FD0); }

	// Stops the looping of audio event associated with this controller instantly.
	void StopLooping()
		{ JMP_THIS(0x405E80); }

	// Ends the looping of audio event associated with this controller, allowing it to fade out.
	void EndLooping()
		{ JMP_THIS(0x406060); }

	void SetEvent(void* event, VocClass* eventType)
		{ JMP_THIS(0x4060F0); }

	void* GetEvent()
		{ JMP_THIS(0x406130); }

	VocClass* GetEventType()
		{ JMP_THIS(0x406170); }

	// Not actually AudioController function, adjusts volume of a given audio event.
	static void __fastcall AdjustAudioEventVolume(void* event, unsigned int volume)
		{ JMP_STD(0x4061D0); }

	// Not actually AudioController function, adjusts panning of a given audio event.
	static void __fastcall AdjustAudioEventPanning(void* event, unsigned int pan)
		{ JMP_THIS(0x406270); }

	// Not actually AudioController function, gets event type (VocClass) of given audio event.
	static VocClass* __fastcall GetEventType(void* event)
		{ JMP_STD(0x406310); }
};
