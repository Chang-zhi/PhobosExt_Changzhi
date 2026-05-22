#pragma once

#include <GeneralDefinitions.h>
#include <ArrayClasses.h>
#include <IndexClass.h>

#include <Helpers/CompileTime.h>

class NOVTABLE CommandClass
{
public:
	// static
	DEFINE_REFERENCE(DynamicVectorClass<CommandClass*>, Array, 0x87F658u)
	DEFINE_REFERENCE((IndexClass<unsigned short, CommandClass*>), Hotkeys, 0x87F680u)

	//CommandClass
	virtual ~CommandClass() = default;
	virtual const char* GetName() const = 0;
	virtual const wchar_t* GetUIName() const = 0;
	virtual const wchar_t* GetUICategory() const = 0;
	virtual const wchar_t* GetUIDescription() const = 0;

	virtual bool PreventCombinationOverride(WWKey eInput) const // Do we need to check extra value like SHIFT?
		{ return false; }										// If this value is true, the game won't process
																// Combination keys written here
																// e.g. To ignore SHIFT + this key
																// return eInput & WWKey::Shift;

	virtual bool ExtraTriggerCondition(WWKey eInput) const // Only with this key set to true will the game call the Execute
		{ return !(eInput & WWKey::Release); }

	virtual bool CheckLoop55E020(WWKey eInput) const // Stupid loop, I don't know what's it used for
		{ return false; }

	virtual void Execute(WWKey eInput) const = 0;
};
