#pragma once

#include <tuple>
#include <memory>

namespace utilities
{
	struct rule_definition {};
	struct null_attribute {};

	template <typename AttributeType>
	inline constexpr bool is_null_attribute_v =
		std::is_same_v<AttributeType, utilities::null_attribute>;
}