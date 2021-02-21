#pragma once
#include <limits>

class XOrShift
{
public:

	// manually seeding
	explicit XOrShift(unsigned long seed)
	{
		m_seed1 = seed << 3 | seed;
		m_seed2 = seed >> 7 | seed;
	}

	// Random bool
	bool RandomBool()
	{
		if (RandomIntRange(0, 100) >= 50)
			return true;

		return false;
	}

	// Generating random 32 bit integer
	int RandomInt()
	{
		int result = 0;

		// mangling...
		unsigned long temp1 = m_seed2;
		m_seed1 ^= m_seed1 << 23;
		unsigned long temp2 = m_seed1 ^ m_seed2 ^ (m_seed1 >> 17) ^ (m_seed2 >> 26);
		result = (int)(temp2 + m_seed2);

		// reassign seed
		m_seed1 = temp1;
		m_seed2 = temp2;

		return result;
	}

	// Generate random int in range ( min, max ]
	int RandomIntRange(int min, int max)
	{
		float randomFloat = RandomFloat();
		int range = max - min;
		return (int)(randomFloat * (float)range) + min;
	}

	// Generating random float from 0 to 1
	float RandomFloat()
	{
		float result = 0.f;

		// mangling...
		unsigned long temp1 = m_seed2;
		m_seed1 ^= m_seed1 << 23;
		unsigned long temp2 = m_seed1 ^ m_seed2 ^ (m_seed1 >> 17) ^ (m_seed2 >> 26);
		unsigned long temp3 = temp2 + m_seed2;

		result = 1.0f / ((float)INT_MAX + 1.0f) * (float)(0x7FFFFFFF & temp3);

		// reassign seed
		m_seed1 = temp1;
		m_seed2 = temp2;

		return result;
	}

	// Generate random float in range ( min, max ]
	float RandomFloatRange(float min, float max)
	{
		float randomFloat = RandomFloat();
		float range = max - min;
		return randomFloat * range + min;
	}

private:
	unsigned long m_seed1;
	unsigned long m_seed2;

};