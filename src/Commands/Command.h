#pragma once

#include <YRPP.h>
#include <CommandClass.h>
#include <StringTable.h>

class AresCommandClass : public CommandClass
{
protected:
	/**
		Adds a notification to the message list when debug key commands are disabled.

		\return true, if debug commands are deactivated, false otherwise.
		\author AlexB
		\date 2012-06-21
	*/
	bool CheckDebugDeactivated() const {
		//if (!Ares::GlobalControls::DebugKeysEnabled) {
		//	if (const wchar_t* text = StringTable::LoadStringA("TXT_COMMAND_DISABLED")) {
		//		wchar_t msg[0x100] = L"\0";
		//		wsprintfW(msg, text, this->GetUIName());
		//		MessageListClass::Instance->PrintMessage(msg);
		//	}
		//	return true;
		//}
		//return false;
		return true;
	}
};

// will the templates ever stop? :D
template <typename T>
void MakeCommand() {
	T* command = GameCreate<T>();
	CommandClass::Array.AddItem(command);
};
