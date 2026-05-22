#pragma once

template<typename key_type, typename value_type>
struct HashObject
{
	key_type Key;
	value_type Value;
};

template<typename key_type, typename value_type>
struct HashTable
{
	DynamicVectorClass<HashObject<key_type,value_type>> * Buckets;
	DWORD (*BucketHashFunction)(const key_type&);
	int BucketCount;
	int BucketGrowthStep;
};

struct HashIterator
{
	int BucketIndex;
	int InBucketIndex;
	bool OutOfBuckets;
};
