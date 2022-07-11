#include <tuple>
#include <functional>
#include <algorithm>

namespace dp
{
	template<int Size, typename ReturnType, typename... ArgTypes>
	class memoize
	{
	public:
		using Index = int;
		using FuncType = std::function<ReturnType(memoize<Size, ReturnType, ArgTypes...>& Self, ArgTypes...)>;
		Index _NextIndex = 0;
		Index _Count = 0;

		memoize(const FuncType& Func) : _Func(Func) { static_assert(Size % 2 == 0, "Size of memoize has to be a power of 2."); }

		struct SBucket
		{
			Index _Slots[Size];
			Index _Count = 0;

			struct SEntry
			{
				Index _BucketIndex = -1;
				Index _SlotIndex = -1;
			};
		} _Buckets[Size];

		struct SCache
		{
			std::tuple<ArgTypes...> _Arguments;
			ReturnType _ReturnValue = ReturnType();
			SBucket::SEntry _BucketEntry;
		} _Cache[Size];

		ReturnType operator()(const ArgTypes... Arguments)
		{
			size_t Hash = 0;
			((Hash ^= std::hash<std::decay_t<decltype(Arguments)>>{}(Arguments)+0xeeffddcc + (Hash << 5) + (Hash >> 3)), ...);
			Index BucketIndex = Hash & (Size - 1);

			SCache* pCachedEntry = nullptr;
			SBucket& Bucket = _Buckets[BucketIndex];
			for (Index i = 0; i < Bucket._Count; i++)
			{
				SCache& CachedEntry = _Cache[Bucket._Slots[i]];
				if (std::apply([&](auto&... CachedArguments) {
					return ((CachedArguments == Arguments) && ...);
				}, CachedEntry._Arguments))
				{
					pCachedEntry = &CachedEntry;
					break;
				}
			}

			if (!pCachedEntry)
			{
				ReturnType ReturnValue = _Func(*this, Arguments...);

				pCachedEntry = &_Cache[_NextIndex];

				if (_Count == Size)
				{
					const SBucket::SEntry& BucketEntry = pCachedEntry->_BucketEntry;
					SBucket& RemoveFromBucket = _Buckets[BucketEntry._BucketIndex];

					if(BucketEntry._SlotIndex < RemoveFromBucket._Count)
						std::swap(RemoveFromBucket._Slots[BucketEntry._SlotIndex], RemoveFromBucket._Slots[RemoveFromBucket._Count - (Index)1]);

					RemoveFromBucket._Count--;
				}

				pCachedEntry->_Arguments = std::make_tuple(Arguments...);
				pCachedEntry->_ReturnValue = std::move(ReturnValue);
				pCachedEntry->_BucketEntry._BucketIndex = BucketIndex;
				pCachedEntry->_BucketEntry._SlotIndex = Bucket._Count;

				Bucket._Slots[Bucket._Count] = _NextIndex;
				Bucket._Count++;

				_Count = std::min(++_Count, Size);
				_NextIndex = ++_NextIndex % Size;
			}

			return pCachedEntry->_ReturnValue;
		}

	private:
		FuncType _Func;
	};
}
