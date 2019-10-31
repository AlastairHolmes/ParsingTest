#include <iostream>
#include <optional>
#include <tuple>
#include <variant>
#include <type_traits>

// lambda that returns an optional pair of new itreator and attribute

// define and then instantiation

// Is this what you expect ???
// ParserIterator 

// Parser
	// Begin
	// End
	// GetParser

// Parser
// Ordered List of Base Rules
// Parse the base rules (with backtracking)
// Backtracking points contain a list of remaining rules and iterator into the string
// Each time a base parser is rule - test each remaining rule (elimate those that fail)


// template virtual pure base class

// action
// parse

// id using ptr to pure base class



// use simple parser
	// with actions
// creates a list of successful parsers
	// when backtacking remove unsucessful ones
// 

/*template <typename ParserType, typename ActionType>
class ActionParser
{
public:



	ActionParser(const ActionType& p_action)
		: m_action(p_action)
	{}

	ActionParser(ActionType&& p_action)
		: m_action(std::move(p_action))
	{}

	template <typename RootType, typename IteratorType, typename SentinelType>
	std::optional<IteratorType> parse(const RootType& p_root, const IteratorType& p_iterator, const SentinelType& p_sentinel) const
	{
		return internal_parse(p_root, p_iterator, p_sentinel, std::get<ParserTypes>(m_parsers)...);
	}

private:

	ParserType m_parser;
	ActionType m_action;

};*/

namespace traits
{

	template <typename Types>
	struct all_same : std::true_type {};

	template <typename Type>
	struct all_same<std::tuple<Type>> : std::true_type {};

	template <typename Type1, typename Type2, typename... Types>
	struct all_same<std::tuple<Type1, Type2, Types...>> :
		std::bool_constant<
			std::is_same_v<Type1, Type2> &&
			all_same<std::tuple<Type1, Types...>>::value
		>
	{};

	template <typename Types>
	inline constexpr bool all_same_v = all_same<Types>::value;

	//----------------------------//

	template <typename Type, typename Types>
	struct contains : std::false_type {};

	template <typename Type>
	struct contains<Type, std::tuple<>> : std::false_type {};

	template <typename Type, typename Type1, typename... Types>
	struct contains<Type, std::tuple<Type1, Types...>> :
		std::bool_constant<
			std::is_same_v<Type, Type1> ||
			contains<Type, std::tuple<Types...>>
		>
	{};

	template <typename Type, typename Types>
	inline constexpr bool contains_v = contains<Type, Types>::value;

	//----------------------------//

	template <typename Type, typename Types>
	struct filter
	{
		using type = std::tuple<>;
	};

	template <typename Type, typename Types>
	using filter_t = typename filter<Type, Types>::type;

	template <typename Type, typename... Types>
	struct filter<Type, std::tuple<Type, Types...>>
	{
		using type = typename filter<Type, std::tuple<Types...>>::type;
	};

	template <typename Type, typename Type1, typename... Types>
	struct filter<Type, std::tuple<Type1, Types...>>
	{
		using type = std::conditional_t <
			std::is_same_v<Type, Type1>,
			filter_t<Type, std::tuple<Types...>>,
			decltype(
				std::tuple_cat(
					std::declval<std::tuple<Type1>>(),
					std::declval<filter_t<Type, std::tuple<Types...>>>()
				)
			)
		>;		
	};

	//----------------------------//

	template <typename Types>
	struct unique
	{
		using type = std::tuple<>;
	};

	template <typename Types>
	using unique_t = typename unique<Types>::type;

	template <typename Type, typename... Types>
	struct unique<std::tuple<Type, Types...>>
	{
		using type = decltype(std::tuple_cat(
			std::declval<std::tuple<Type>>(),
			std::declval<unique_t<filter_t<Type, std::tuple<Types...>>>>()
		));
	};

	//----------------------------//

	template <typename Tuple>
	struct variant;

	template <typename... Types>
	struct variant<std::tuple<Types...>>
	{
		using type = std::variant<Types...>;
	};

	template <typename Tuple>
	using variant_t = typename variant<Tuple>::type;

}

struct null_attribute {};

template <typename AttributeType>
inline constexpr bool is_null_attribute_v = std::is_same_v<AttributeType, null_attribute>;

template <typename RootRuleType, typename HeadRuleDefinitionType, typename TailRuleDefinitionType>
class sequence_rule
{
	template <typename AttributeType1, typename AttributeType2>
	struct join_attribute_types
	{
		using type = std::tuple<AttributeType1, AttributeType2>;

		static type Create(
			AttributeType1&& p_attribute1,
			AttributeType2&& p_attribute2
		)
		{
			return std::make_tuple(std::move(p_attribute1), std::move(p_attribute2));
		}
	};

	template <typename AttributeType2>
	struct join_attribute_types<null_attribute, AttributeType2>
	{
		using type = AttributeType2;

		static type Create(
			null_attribute&& p_attribute1,
			AttributeType2&& p_attribute2
		)
		{
			return std::move(p_attribute2);
		}
	};

	template <typename AttributeType1>
	struct join_attribute_types<AttributeType1, null_attribute>
	{
		using type = AttributeType1;

		static type Create(
			AttributeType1&& p_attribute1,
			null_attribute&& p_attribute2
		)
		{
			return std::move(p_attribute1);
		}
	};

	template <typename AttributeType1, typename... AttributeTypes2>
	struct join_attribute_types<AttributeType1, std::tuple<AttributeTypes2...>>
	{
		using type = std::tuple<AttributeType1, AttributeTypes2...>;

		static type Create(
			AttributeType1&& p_attribute1,
			std::tuple<AttributeTypes2...>&& p_attribute2
		)
		{
			return std::tuple_cat(std::make_tuple(std::move(p_attribute1)), std::move(p_attribute2));
		}
	};

	template <typename... AttributeTypes1, typename AttributeType2>
	struct join_attribute_types<std::tuple<AttributeTypes1...>, AttributeType2>
	{
		using type = std::tuple<AttributeTypes1..., AttributeType2>;

		static type Create(
			std::tuple<AttributeTypes1...>&& p_attribute1,
			AttributeType2&& p_attribute2
		)
		{
			return std::tuple_cat(std::move(p_attribute1), std::make_tuple(std::move(p_attribute2)));
		}
	};

	template <typename... AttributeTypes1, typename... AttributeTypes2>
	struct join_attribute_types<std::tuple<AttributeTypes1...>, std::tuple<AttributeTypes2...>>
	{
		using type = std::tuple<AttributeTypes1..., AttributeTypes2...>;

		static type Create(
			std::tuple<AttributeTypes1...>&& p_attribute1,
			std::tuple<AttributeTypes2...>&& p_attribute2
		)
		{
			return std::tuple_cat(std::move(p_attribute1), std::move(p_attribute2));
		}
	};

public:

	using attribute_type = typename join_attribute_types<
		typename HeadRuleDefinitionType::attribute_type,
		typename TailRuleDefinitionType::attribute_type
	>::type;

	sequence_rule(const HeadRuleDefinitionType& p_ruleHead, const TailRuleDefinitionType& p_ruleTail)
		: m_ruleHead(p_ruleHead), m_ruleTail(p_ruleTail)
	{}

	sequence_rule(HeadRuleDefinitionType&& p_ruleHead, TailRuleDefinitionType&& p_ruleTail)
		: m_ruleHead(std::move(p_ruleHead)), m_ruleTail(std::move(p_ruleTail))
	{}

	template <typename IteratorType, typename SentinelType>
	std::optional<std::pair<IteratorType, attribute_type>> parse(const RootRuleType& p_root, const IteratorType& p_iterator, const SentinelType& p_sentinel) const
	{
		std::optional<
			std::pair<
				IteratorType,
				typename HeadRuleDefinitionType::attribute_type
			>
		> optpairitattrHead = m_ruleHead.parse(p_root, p_iterator, p_sentinel);
		if (optpairitattrHead.has_value())
		{
			std::optional<
				std::pair<
				IteratorType,
				typename TailRuleDefinitionType::attribute_type
				>
			> optpairitattrTail = m_ruleTail.parse(p_root, optpairitattrHead.value().first, p_sentinel);
			if (optpairitattrTail.has_value())
			{
				return join_attribute_types<
					typename HeadRuleDefinitionType::attribute_type,
					typename TailRuleDefinitionType::attribute_type
				>::Create(
					std::move(optpairitattrHead.value().second),
					std::move(optpairitattrTail.value().second)
				);
			}
		}
		return std::nullopt;
	}

private:

	HeadRuleDefinitionType m_ruleHead;
	TailRuleDefinitionType m_ruleTail;

};

inline constexpr auto sequence_rule = [](auto&& p_ruledefHead, auto&& p_ruledefTail)
{
	return [
		ruledefHead = std::forward<decltype(p_ruledefHead)>(p_ruledefHead),
		ruledefTail = std::forward<decltype(p_ruledefTail)>(p_ruledefTail)
	](const auto& p_ruleRoot)
	{
		return sequence_rule<
			decltype(p_ruleRoot),
			decltype(ruledefHead),
			decltype(ruledefTail)
		>(ruledefHead(p_ruleRoot), ruledefTail(p_ruleRoot));
	};
};

template <typename RootRuleType, typename ElementType>
class single_rule_instance
{
public:

	single_rule_instance(const ElementType& p_element)
		: m_element(p_element)
	{}

	single_rule_instance(ElementType&& p_element)
		: m_element(std::move(p_element))
	{}

	using attribute_type = ElementType;

	template <typename IteratorType, typename SentinelType>
	std::optional<std::pair<IteratorType, attribute_type>> parse(const RootRuleType& p_root, const IteratorType& p_iterator, const SentinelType& p_sentinel) const
	{
		if (p_sentinel != p_iterator && m_element == *p_iterator)
		{
			return std::make_pair(std::next(p_iterator), m_element);
		}
		else
		{
			return std::nullopt;
		}
	}

private:

	const ElementType m_element;

};

inline constexpr auto single_rule = [](auto&& p_element)
{
	return [element = std::forward<decltype(p_element)>(p_element)](const auto& p_ruleRoot)
	{
		return single_rule_instance(std::move(element));
	};
};

template <typename RootRuleType>
class recursive_rule_instance
{
public:

	recursive_rule_instance(const RootRuleType& p_ruleRoot)
		: m_ruleRoot(p_ruleRoot)
	{}

	using attribute_type = typename RootRuleType::attribute_type;

	template <typename IteratorType, typename SentinelType>
	std::optional<std::pair<IteratorType, attribute_type>> parse(const RootRuleType& p_root, const IteratorType& p_iterator, const SentinelType& p_sentinel) const
	{
		return m_ruleRoot.parse(p_root, p_iterator, p_sentinel);
	}

private:
	const RootRuleType& m_ruleRoot;
};

inline constexpr auto recurse_rule = []()
{
	return [](const auto& p_ruleRoot)
	{
		return recursive_rule_instance(p_ruleRoot);
	};
};

template <typename IteratorType, typename SentinelType>
bool parse(const IteratorType& p_iterator, const SentinelType& p_sentinel)
{

}

int main()
{
	sequence_rule(single_rule('a'), single_rule('b'));
	return 0;
}
