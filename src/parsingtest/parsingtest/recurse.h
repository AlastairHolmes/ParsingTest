#pragma once

#include <parsingtest/root.h>

namespace utilities
{
	template <typename RootType, template<typename...> typename AttributeTemplate>
	struct recursive_attribute;

	template <typename RootType, template<typename...> typename AttributeTemplate>
	struct attribute_type;

	template <typename... RuleDefinitionTypes, template<typename...> typename AttributeTemplate>
	struct attribute_type<utilities::root<RuleDefinitionTypes...>, AttributeTemplate>
	{
		using type = AttributeTemplate<
			utilities::recursive_attribute<
				utilities::root<RuleDefinitionTypes...>,
				#if defined(__clang__) ||  defined(__GNUC__) || defined(__GNUG__)
					RuleDefinitionTypes::
						template rule_template<root<RuleDefinitionTypes...>>::
							template attribute_template
				#elif defined(_MSC_VER)
					typename RuleDefinitionTypes::
						template rule_template<utilities::root<RuleDefinitionTypes...>>::
							template attribute_template
				#else
					#error "Unknown Compiler"
				#endif
			>...
		>;

		//using type = std::conditional_t<utilities::is_null_attribute_v<>>;
	};

	template <typename RootType, template<typename...> typename AttributeTemplate>
	using attribute_type_t =
		typename utilities::attribute_type<RootType, AttributeTemplate>::type;

	template <typename RootType, template<typename...> typename AttributeTemplate>
	struct recursive_attribute
	{
		recursive_attribute(utilities::attribute_type_t<RootType, AttributeTemplate>&& p_value) noexcept
			: m_attribute(std::make_unique<utilities::attribute_type_t<RootType, AttributeTemplate>>(std::move(p_value)))
		{}

		std::unique_ptr<utilities::attribute_type_t<RootType, AttributeTemplate>> m_attribute;
	};

	template <std::size_t RuleIndex, typename RootType>
	class recurse_rule
	{
	public:

		template <typename... RuleAttributeTypes>
		using attribute_template =
			std::tuple_element_t<RuleIndex, std::tuple<RuleAttributeTypes...>>;

		template <typename IteratorType, typename SentinelType>
		auto parse(const IteratorType& p_iterator, const SentinelType& p_sentinel, const RootType& p_root) const
		{
			auto optpairitattr = p_root.template rule<RuleIndex>().parse(p_iterator, p_sentinel, p_root);
			if (optpairitattr.has_value())
			{
				return std::make_optional(std::make_pair(
					std::move(optpairitattr.value().first),
					utilities::attribute_type_t<RootType, attribute_template>(std::move(optpairitattr.value().second))
				));
			}
			else
			{
				return std::optional<std::pair<IteratorType, utilities::attribute_type_t<RootType, attribute_template>>>{std::nullopt};
			}
		}

	};

	template <std::size_t Index>
	struct recurse_rule_definition : utilities::rule_definition
	{
		template <typename RootType>
		using rule_template = utilities::recurse_rule<Index, RootType>;

		template <typename RootType>
		constexpr rule_template<RootType> create()
		{
			return rule_template<RootType>();
		}
	};
}

template <std::size_t Index>
constexpr utilities::recurse_rule_definition<Index> recurse()
{
	return utilities::recurse_rule_definition<Index>();
};