#pragma once

class FPSCounter
{
public:
	//!< The number of frames processed in the last second.
	DEFINE_REFERENCE(unsigned int, CurrentFrameRate, 0xABCD44u)

	//!< The total number of frames elapsed.
	DEFINE_REFERENCE(unsigned int, TotalFramesElapsed, 0xABCD48u)

	//!< The time it took to process TotalFramesElapsed frames.
	DEFINE_REFERENCE(unsigned int, TotalTimeElapsed, 0xABCD4Cu)

	//!< Whether the current fps is considered too low.
	DEFINE_REFERENCE(bool, ReducedEffects, 0xABCD50u)

	//!< The average frame rate for all frames processed.
	static inline double GetAverageFrameRate()
	{
		if(TotalTimeElapsed) {
			return static_cast<double>(TotalFramesElapsed)
				/ static_cast<double>(TotalTimeElapsed);
		}

		return 0.0;
	}
};

class Detail {
public:
	//!< What is considered the minimum acceptable FPS.
	DEFINE_REFERENCE(unsigned int, MinFrameRate, 0x829FF4u)

	//!< The zone that needs to be left to change
	DEFINE_REFERENCE(unsigned int, BufferZoneWidth, 0x829FF8u)

	//!< The minimum frame rate considering the buffer zone.
	static inline unsigned int GetMinFrameRate()
		{ JMP_STD(0x55AF60); }

	//!< Whether effects should be reduced.
	static inline bool ReduceEffects()
	{
		return FPSCounter::CurrentFrameRate < GetMinFrameRate();
	}
};
