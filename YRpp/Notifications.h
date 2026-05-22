#pragma once

#include <ArrayClasses.h>

class AbstractClass;

// encapsulates a bunch of vectors that are used for announcing invalid pointers.
// if an AbstractClass is contained in an list it will be notified through
// PointerExpired whenever an object of that type expires.
class PointerExpiredNotification {
public:
	DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidObject, 0xB0F720u) // Object class hierarchy
	DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidType, 0xB0F670u) // AbstractType class hierarchy
	DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidAnim, 0xB0F5B8u) // AnimClass
	DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidHouse, 0xB0F6C8u) // HouseClass
	DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidTag, 0xB0F618u) // TagClass
	DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidTrigger, 0xB0F708u) // TriggerClass
	DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidFactory, 0xB0F640u) // FactoryClass
	DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidWaypoint, 0xB0F5F0u) // WaypointClass
	DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidTeam, 0xB0F5D8u) // TeamClass
	DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidNeuron, 0xB0F6F0u) // NeuronClass
	DEFINE_REFERENCE(PointerExpiredNotification, NotifyInvalidActionOrEvent, 0xB0F658u) // ActionClass and EventClass

	inline bool Add(AbstractClass* object) {
		// add only if doesn't exist
		return this->Array.AddUnique(object);
	}

	inline bool Remove(AbstractClass* object) {
		return this->Array.Remove(object);
	}

	DynamicVectorClass<AbstractClass*> Array;
};
