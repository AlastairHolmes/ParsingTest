#pragma once

namespace utilities
{
	template <typename Type>
	struct identity
	{
		using type = Type;
	};

	template <typename RuleType, typename IteratorType, typename SentinelType, typename RootType>
	using parse_result_t = decltype(std::declval<RuleType>().parse(
		std::declval<IteratorType>(),
		std::declval<SentinelType>(),
		std::declval<RootType>()
	));

	template <typename RuleType, typename IteratorType, typename SentinelType, typename RootType>
	using parse_attribute_t = decltype(
		std::declval<utilities::parse_result_t<
			RuleType,
			IteratorType,
			SentinelType,
			RootType
		>>().value().second
	);
}