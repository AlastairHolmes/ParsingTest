#pragma once

#include <optional>
#include <utility>
#include <parsingtest/utilities.h>

namespace utilities
{
	template <typename ElementType, typename RootType>
	class single_rule
	{
	public:
		static_assert(!std::is_reference_v<RootType>);
		static_assert(!std::is_reference_v<ElementType>);

		single_rule(const ElementType& p_element)
			: m_element(p_element)
		{}

		single_rule(ElementType&& p_element)
			: m_element(std::move(p_element))
		{}

		template <typename... RuleAttributeTypes>
		using attribute_template = utilities::null_attribute;

		template <typename IteratorType, typename SentinelType>
		auto parse(const IteratorType& p_iterator, const SentinelType& p_sentinel, const RootType&) const
		{
			return p_sentinel != p_iterator && m_element == *p_iterator ? std::make_optional(std::make_pair(std::next(p_iterator), utilities::null_attribute{})) : std::nullopt;
		}
	private:
		const ElementType m_element;
	};

	template <typename ElementType>
	struct single_rule_definition : utilities::rule_definition
	{
		single_rule_definition(const ElementType& p_element)
			: m_element(p_element)
		{}

		single_rule_definition(ElementType&& p_element)
			: m_element(std::move(p_element))
		{}

		template <typename RootType>
		using rule_template = utilities::single_rule<ElementType, RootType>;

		template <typename RootType>
		constexpr auto create()
		{
			return utilities::single_rule<ElementType, RootType>(std::move(m_element));
		}

		ElementType m_element;
	};
}

template <typename ElementType>
constexpr auto single(ElementType&& p_element)
{
	return utilities::single_rule_definition<ElementType>(std::forward<ElementType>(p_element));
};