#pragma once

#include <Helpers/CompileTime.h>

class Powerups {
	public:
	// all these actually point to arrays with 0x13 items, see ePowerup for their numbering
	/**
	 * e.g. Powerups::Weights[pow_Unit] is the weight of the free unit crate
	 */

	// the name of the effect, for INI reading purposes
	DEFINE_ARRAY_REFERENCE(const char* const, [19u], Effects, 0x7E523Cu)

	// the weight of the effect
	DEFINE_ARRAY_REFERENCE(int, [19u], Weights, 0x81DA8Cu)

	// the effect-specific argument
	DEFINE_ARRAY_REFERENCE(double, [19u], Arguments, 0x89EC28u)

	// can this crate appear on water?
	DEFINE_ARRAY_REFERENCE(bool, [19u], Naval, 0x89ECC0u)

	// index into AnimTypeClass::Array
	DEFINE_ARRAY_REFERENCE(int, [19u], Anims, 0x81DAD8u)
};
