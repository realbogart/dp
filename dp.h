#include <tuple>
#include <functional>

namespace dp
{
	template<int Size, typename ReturnType, typename... ArgTypes>
	class memoize
	{
	public:
		using FuncType = std::function<ReturnType(memoize<Size, ReturnType, ArgTypes...>& Self, ArgTypes...)>;
		int _CurrentIndex = 0;

		memoize(const FuncType& Func) : _Func(Func) {}

		struct SCache
		{
			bool _bCalculated = false;
			ReturnType _ReturnValue = ReturnType();
			std::tuple<ArgTypes...> _Arguments;
		} _Cache[Size];

		ReturnType operator()(const ArgTypes... Arguments)
		{
			SCache NewCache;
			NewCache._Arguments = std::make_tuple(Arguments...);

			const SCache* pCachedResult = nullptr;
			for (int i = 0; i < Size; i++)
			{
				int Index = (_CurrentIndex + Size - i - 1) % Size;
				const SCache& CachedResult = _Cache[Index];
				if (CachedResult._bCalculated && (CachedResult._Arguments == NewCache._Arguments))
				{
					pCachedResult = &CachedResult;
					break;
				}
			}

			if (!pCachedResult)
			{
				NewCache._ReturnValue = _Func(*this, Arguments...);
				NewCache._bCalculated = true;

				_Cache[_CurrentIndex] = NewCache;
				_CurrentIndex = ++_CurrentIndex % Size;

				pCachedResult = &NewCache;
			}

			return pCachedResult->_ReturnValue;
		}

	private:
		FuncType _Func;
	};
}
