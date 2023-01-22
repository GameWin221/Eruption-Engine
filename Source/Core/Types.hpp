#ifndef EN_TYPES_HPP
#define EN_TYPES_HPP

#include <memory>

namespace en 
{
	template <typename T>
	using Handle = std::shared_ptr<T>;

	template<typename T, typename ... Args>
	constexpr Handle<T> MakeHandle(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template <typename T>
	using Scope = std::unique_ptr<T>;

	template<typename T, typename ... Args>
	constexpr Scope<T> MakeScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	//using i8  = char;
	//using i16 = short;
	//using i32 = long;
	//using i64 = long long;
	//
	//using u8  = unsigned char;
	//using u16 = unsigned short;
	//using u32 = unsigned long;
	//using u64 = unsigned long long ;
	//
	//using f32 = float;
	//using f64 = double;
}

#endif