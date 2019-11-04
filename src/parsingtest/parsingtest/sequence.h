#pragma once

#include <optional>
#include <utility>
#include <type_traits>
#include <parsingtest/utilities.h>
#include <parsingtest/type_traits.h>
#include <parsingtest/recurse.h>

namespace utilities
{
	template <typename RuleType>
	struct is_tuple : std::false_type {};

	template <typename... Types>
	struct is_tuple<std::tuple<Types...>> : std::true_type {};

	template <typename Type>
	inline constexpr bool is_tuple_v = utilities::is_tuple<Type>::value;

	template <typename Tuple1, typename Tuple2>
	struct tuple_cat {};

	template <typename... Types1, typename... Types2>
	struct tuple_cat<std::tuple<Types1...>, std::tuple<Types2...>>
	{
		using type = std::tuple<Types1..., Types2...>;
	};

	template <typename Tuple1, typename Tuple2>
	using tuple_cat_t = typename utilities::tuple_cat<Tuple1, Tuple2>::type;

	template <typename HeadRuleType, typename TailRuleType, typename RootType>
	class sequence_rule;

	template <typename RuleType>
	struct is_sequence_rule :
		std::false_type {};

	template <typename HeadRuleType, typename TailRuleType, typename RootType>
	struct is_sequence_rule<utilities::sequence_rule<HeadRuleType, TailRuleType, RootType>> :
		std::true_type {};

	template <typename RuleType>
	inline constexpr bool is_sequence_rule_v =
		utilities::is_sequence_rule<RuleType>::value;

	template <typename HeadRuleType, typename TailRuleType, typename RootType>
	class sequence_rule
	{
	private:

		static_assert(
			!std::is_reference_v<HeadRuleType> && !std::is_const_v<HeadRuleType> &&
			!std::is_reference_v<TailRuleType> && !std::is_const_v<TailRuleType> &&
			!std::is_reference_v<RootType> && !std::is_const_v<RootType>
		);

		template <typename HeadAttributeType, typename TailAttributeType>
		struct attribute_type
		{
		private:
			static constexpr auto internal_make_attribute_type(utilities::identity<HeadAttributeType> p_head_attribute, utilities::identity<TailAttributeType> p_tail_attribute)
			{
				p_head_attribute;
				p_tail_attribute;
				if constexpr (utilities::is_null_attribute_v<HeadAttributeType>)
				{
					return utilities::identity<TailAttributeType>();
				}
				else if constexpr (utilities::is_null_attribute_v<TailAttributeType>)
				{
					return utilities::identity<HeadAttributeType>();
				}
				else if constexpr (
					utilities::is_sequence_rule_v<HeadRuleType> &&
					utilities::is_tuple_v<HeadAttributeType> &&
					utilities::is_sequence_rule_v<TailRuleType> &&
					utilities::is_tuple_v<TailAttributeType>
				)
				{
					return utilities::identity<tuple_cat_t<HeadAttributeType, TailAttributeType>>();
				}
				else if constexpr (utilities::is_sequence_rule_v<HeadRuleType> && utilities::is_tuple_v<HeadAttributeType>)
				{
					return utilities::identity<utilities::tuple_cat_t<HeadAttributeType, std::tuple<TailAttributeType>>>();
				}
				else if constexpr (utilities::is_sequence_rule_v<TailRuleType> && utilities::is_tuple_v<TailAttributeType>)
				{
					return utilities::identity<utilities::tuple_cat_t<std::tuple<HeadAttributeType>, TailAttributeType>>();
				}
				else
				{
					return utilities::identity<std::tuple<HeadAttributeType, TailAttributeType>>();
				}
			};
		public:
			using type = typename decltype(internal_make_attribute_type(
				utilities::identity<HeadAttributeType>(),
				utilities::identity<TailAttributeType>()
			))::type;

			static constexpr auto make(HeadAttributeType&& p_head_attribute, TailAttributeType&& p_tail_attribute)
			{
				p_head_attribute;
				p_tail_attribute;
				if constexpr (utilities::is_null_attribute_v<HeadAttributeType>)
				{
					return std::move(p_tail_attribute);
				}
				else if constexpr (utilities::is_null_attribute_v<TailAttributeType>)
				{
					return std::move(p_head_attribute);
				}
				else if constexpr (utilities::is_sequence_rule_v<HeadRuleType> && utilities::is_tuple_v<HeadAttributeType> && utilities::is_sequence_rule_v<TailRuleType> && utilities::is_tuple_v<TailAttributeType>)
				{
					return std::tuple_cat(std::move(p_head_attribute), std::move(p_tail_attribute));
				}
				else if constexpr (utilities::is_sequence_rule_v<HeadRuleType> && utilities::is_tuple_v<HeadAttributeType>)
				{
					return std::tuple_cat(std::move(p_head_attribute), std::make_tuple(std::move(p_tail_attribute)));
				}
				else if constexpr (utilities::is_sequence_rule_v<TailRuleType> && utilities::is_tuple_v<TailAttributeType>)
				{
					return std::tuple_cat(std::make_tuple(std::move(p_head_attribute)), std::move(p_tail_attribute));
				}
				else
				{
					return std::make_tuple(std::move(p_head_attribute), std::move(p_tail_attribute));
				}
			};
		};

	public:

		sequence_rule(const HeadRuleType& p_ruleHead, const TailRuleType& p_ruleTail)
			: m_ruleHead(p_ruleHead), m_ruleTail(p_ruleTail)
		{}

		sequence_rule(HeadRuleType&& p_ruleHead, TailRuleType&& p_ruleTail)
			: m_ruleHead(std::move(p_ruleHead)), m_ruleTail(std::move(p_ruleTail))
		{}

		template <typename... RuleAttributeTypes>
		using head_attribute_template = typename HeadRuleType::template attribute_template<RuleAttributeTypes...>;

		template <typename... RuleAttributeTypes>
		using tail_attribute_template = typename TailRuleType::template attribute_template<RuleAttributeTypes...>;

		template <typename... RuleAttributeTypes>
		using attribute_template =
			typename attribute_type<
				head_attribute_template<RuleAttributeTypes...>,
				tail_attribute_template<RuleAttributeTypes...>
			>::type;

		template <typename IteratorType, typename SentinelType>
		std::optional<std::pair<IteratorType, utilities::attribute_type_t<RootType, attribute_template>>> parse(const IteratorType& p_iterator, const SentinelType& p_sentinel, const RootType& p_root) const
		{
			using head_attribute_t = utilities::parse_attribute_t<HeadRuleType, IteratorType, SentinelType, RootType>;
			using tail_attribute_t = utilities::parse_attribute_t<TailRuleType, IteratorType, SentinelType, RootType>;
			if (auto optpairitattrHead = m_ruleHead.parse(p_iterator, p_sentinel, p_root); optpairitattrHead.has_value())
			{
				if (auto optpairitattrTail = m_ruleTail.parse(optpairitattrHead.value().first, p_sentinel, p_root); optpairitattrTail.has_value())
				{
					return std::make_pair(optpairitattrTail.value().first, attribute_type<head_attribute_t, tail_attribute_t>::make(std::move(optpairitattrHead.value().second), std::move(optpairitattrTail.value().second)));
				}
			}
			return std::nullopt;
		}

	private:

		HeadRuleType m_ruleHead;
		TailRuleType m_ruleTail;
	};

	template <typename HeadRuleDefinitionType, typename TailRuleDefinitionType>
	struct sequence_rule_definition : rule_definition
	{
		sequence_rule_definition(const HeadRuleDefinitionType& p_rule1, const TailRuleDefinitionType& p_rule2)
			: m_ruleHead(p_rule1), m_ruleTail(p_rule2)
		{}

		sequence_rule_definition(HeadRuleDefinitionType&& p_rule1, TailRuleDefinitionType&& p_rule2)
			: m_ruleHead(std::move(p_rule1)), m_ruleTail(std::move(p_rule2))
		{}

		template <typename RootType>
		using rule_template = sequence_rule<
			typename HeadRuleDefinitionType::template rule_template<RootType>,
			typename TailRuleDefinitionType::template rule_template<RootType>,
			RootType
		>;

		template <typename RootType>
		constexpr rule_template<RootType> create()
		{
			return rule_template<RootType>(m_ruleHead.template create<RootType>(), m_ruleTail.template create<RootType>());
		}

		HeadRuleDefinitionType m_ruleHead;
		TailRuleDefinitionType m_ruleTail;
	};
}

template <typename HeadRuleDefinitionType, typename TailRuleDefinitionType>
constexpr auto sequence(HeadRuleDefinitionType&& ruleHead, TailRuleDefinitionType&& ruleTail)
{
	return utilities::sequence_rule_definition<std::decay_t<HeadRuleDefinitionType>, std::decay_t<TailRuleDefinitionType>>(std::forward<HeadRuleDefinitionType>(ruleHead), std::forward<TailRuleDefinitionType>(ruleTail));
};

template <typename RuleType1, typename RuleType2, std::enable_if_t<std::is_base_of_v<utilities::rule_definition, RuleType1>&& std::is_base_of_v<utilities::rule_definition, RuleType2>, int> = 0>
constexpr auto operator>>(RuleType1 && p_rule1, RuleType2 && p_rule2)
{
	return sequence(std::forward<RuleType1>(p_rule1), std::forward<RuleType2>(p_rule2));
}