#ifndef SET_VISITORS_H
#define SET_VISITORS_H

#include "AnyNode.h"

namespace te {

struct BadVisitError:
	public std::logic_error
{
	BadVisitError():
		std::logic_error("Visitation of AnyValue instance failed with provided types.")
	{
		
	}
};

template <
	class T,
	class ... U,
	class Visitor,
	class Hash,
	class Compare,
	class ResultType_ = std::common_type_t<
		std::invoke_result_t<Visitor, T>, 
		std::invoke_result_t<Visitor, U> ...
	>
>
decltype(auto) visit(Visitor&& visitor, const AnyValue<Hash, Compare>& value)
{
	if(const T* p = try_as<T>(value); static_cast<bool>(p))
		return std::forward<Visitor>(visitor)(*p);
	else if constexpr((sizeof...(U)) > 0u)
		return visit<U...>(std::forward<Visitor>(visitor), value);
	else
		throw BadVisitError();
}


} /* namespace te */

#endif /* SET_VISITORS_H */
