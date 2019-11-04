#pragma once

namespace utilities
{
	template <typename ElementType, typename RootType>
	class any_rule
	{
	public:
		static_assert(!std::is_reference_v<RootType>);

		template <typename... RuleAttributeTypes>
		using attribute_template = ElementType;

		template <typename IteratorType, typename SentinelType>
		auto parse(const IteratorType& p_iterator, const SentinelType& p_sentinel, const RootType&) const
		{
			return p_sentinel == p_iterator ? std::nullopt : std::make_optional(std::make_pair(std::next(p_iterator), *p_iterator));
		}
	};

	template <typename ElementType>
	struct any_rule_definition : rule_definition
	{
		template <typename RootType>
		using rule_template = utilities::any_rule<ElementType, RootType>;

		template <typename RootType>
		constexpr auto create()
		{
			return utilities::any_rule<ElementType, RootType>();
		}
	};
}

template <typename ElementType>
constexpr auto any()
{
	return utilities::any_rule_definition<ElementType>();
};