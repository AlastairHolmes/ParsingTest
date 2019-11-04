#pragma once

#include <parsingtest/type_traits.h>
#include <parsingtest/utilities.h>

namespace utilities
{
	template <typename... RuleDefinitionTypes>
	struct root
	{
	public:
		template <typename RuleDefinitionType>
		using rule_type = typename RuleDefinitionType::template rule_template<utilities::root<RuleDefinitionTypes...>>;

		root(RuleDefinitionTypes&& ... p_rules)
			: m_rules(p_rules.template create<root>()...)
		{}

		template <typename IteratorType, typename SentinelType>
		auto parse(const IteratorType& p_iterator, const SentinelType& p_sentinel) const
		{
			return rule<0>().parse(p_iterator, p_sentinel, *this);
		}

		template <std::size_t Index>
		const auto& rule() const
		{
			return std::get<Index>(m_rules);
		}
	private:
		std::tuple<rule_type<RuleDefinitionTypes>...> m_rules;
	};
}

template <typename... RuleDefinitionTypes>
auto make_root(RuleDefinitionTypes&& ... p_rule_definitions)
{
	return utilities::root<RuleDefinitionTypes...>(std::forward<RuleDefinitionTypes>(p_rule_definitions)...);
}

template <typename RootType, typename IteratorType, typename SentinelType>
auto parse(const RootType& p_root, const IteratorType& p_iterator, const SentinelType& p_sentinel)
{
	return p_root.parse(p_iterator, p_sentinel);
}
