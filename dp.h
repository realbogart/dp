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

		memoize(const FuncType& Func) 
			: _Func(Func) 
		{ 
			static_assert(Size % 2 == 0, "Size of memoize has to be a power of 2."); 

			_WasteBucket._Count = Size;

			for (Index i = 0; i < Size; i++)
			{
				SCache& Cache = _Cache[i];
				Cache._SlotIndex = 0;
				Cache._pBucket = &_WasteBucket;
			}
		}

		ReturnType operator()(const ArgTypes&... Arguments)
		{
			size_t Hash = 0;
			std::apply([&](auto&... Hashers) {
				((Hash ^= Hashers(Arguments) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2)), ...);
				}, _Hashers);

			SBucket& Bucket = _Buckets[Hash & (Size - 1)];
			for (Index i = 0; i < Bucket._Count; i++)
			{
				SCache& CachedEntry = _Cache[Bucket._Slots[i]];
				if (std::apply([&](auto&... CachedArguments) {
					return ((CachedArguments == Arguments) && ...);
				}, CachedEntry._Arguments))
				{
					return CachedEntry._ReturnValue;
				}
			}

			ReturnType ReturnValue = _Func(*this, Arguments...);
			SCache& CachedEntry = _Cache[_NextIndex];
			SBucket& RemoveFromBucket = *CachedEntry._pBucket;
			std::swap(RemoveFromBucket._Slots[CachedEntry._SlotIndex], RemoveFromBucket._Slots[--RemoveFromBucket._Count]);
			std::apply([&](auto&... CachedArguments) {
				((CachedArguments = Arguments), ...);
				}, CachedEntry._Arguments);
			CachedEntry._ReturnValue = std::move(ReturnValue);
			CachedEntry._pBucket = &Bucket;
			CachedEntry._SlotIndex = Bucket._Count;
			Bucket._Slots[Bucket._Count++] = _NextIndex;
			_NextIndex = ++_NextIndex & (Size - 1);

			return CachedEntry._ReturnValue;
		}

	private:
		struct SBucket
		{
			Index _Slots[Size];
			Index _Count = 0;
		} _Buckets[Size];

		SBucket _WasteBucket;

		struct SCache
		{
			std::tuple<ArgTypes...> _Arguments;
			ReturnType _ReturnValue = ReturnType();
			SBucket* _pBucket;
			Index _SlotIndex;
		} _Cache[Size];

		std::tuple<std::hash<std::decay_t<ArgTypes>>...> _Hashers;
		FuncType _Func;
		Index _NextIndex = 0;
	};
}
