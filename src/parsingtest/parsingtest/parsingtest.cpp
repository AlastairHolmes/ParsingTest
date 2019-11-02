#include <iostream>
#include <optional>
#include <tuple>
#include <variant>
#include <type_traits>

struct base_rule_definition {};
struct null_attribute {};

template <typename AttributeType>
inline constexpr bool is_null_attribute_v = std::is_same_v<AttributeType, null_attribute>;

template <typename RuleType, typename IteratorType, typename SentinelType, typename RootType>
using parse_result_t = decltype(std::declval<RuleType>().parse(std::declval<IteratorType>(), std::declval<SentinelType>(), std::declval<RootType>()));

template <typename RuleType, typename IteratorType, typename SentinelType, typename RootType>
using parse_attribute_t = decltype(std::declval<parse_result_t<RuleType, IteratorType, SentinelType, RootType>>().value().second);

//----------------------------//

template <typename Type, typename Types>
struct filter
{
	using type = std::tuple<>;
};

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
		typename filter<Type, std::tuple<Types...>>::type,
		decltype(
			std::tuple_cat(
				std::declval<std::tuple<Type1>>(),
				std::declval<typename filter<Type, std::tuple<Types...>>::type>()
			)
		)
	>;
};

template <typename Type, typename Types>
using filter_t = typename filter<Type, Types>::type;

//----------------------------//

template <typename Types>
struct unique
{
	using type = std::tuple<>;
};

template <typename Type, typename... Types>
struct unique<std::tuple<Type, Types...>>
{
	using type = decltype(std::tuple_cat(
		std::declval<std::tuple<Type>>(),
		std::declval<typename unique<filter_t<Type, std::tuple<Types...>>>::type>()
	));
};

template <typename Types>
using unique_t = typename unique<Types>::type;

//----------------------------//

template <typename Tuple>
struct tuple_to_variant;

template <typename... Types>
struct tuple_to_variant<std::tuple<Types...>>
{
	using type = std::variant<Types...>;
};

template <typename Tuple>
using tuple_to_variant_t = typename std::variant<Tuple>::type;

template <typename Variant>
struct variant_to_tuple
{
	using type = std::tuple<>;
};

template <typename... Types>
struct variant_to_tuple<std::variant<Types...>>
{
	using type = std::tuple<Types...>;
};

template <typename Variant>
using variant_to_tuple_t = typename variant_to_tuple<Variant>::type;

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

template <typename Tuple1, typename Tuple2>
struct contains_all : std::true_type {};

template <typename Type, typename... Types, typename Tuple2>
struct contains_all<std::tuple<Type, Types...>, Tuple2> : std::integral_constant<bool, contains_v<Type, Tuple2>&& contains_all<std::tuple<Types...>, Tuple2>::value> {};

template <typename Tuple1, typename Tuple2>
inline constexpr bool contains_all_v = contains_all<Tuple1, Tuple2>::value;

template <typename Tuple1, typename Tuple2>
struct equivalent : std::integral_constant<bool, contains_all_v<Tuple1, Tuple2>&& contains_all_v<Tuple2, Tuple1>> {};

template <typename Tuple1, typename Tuple2>
inline constexpr bool equivalent_v = equivalent<Tuple1, Tuple2>::value;

//----------------------------//

template <typename HeadRuleType, typename TailRuleType, typename RootType>
class sequence_rule_2
{
private:

	static_assert(
		!std::is_reference_v<HeadRuleType> && !std::is_const_v<HeadRuleType> &&
		!std::is_reference_v<TailRuleType> && !std::is_const_v<TailRuleType> &&
		!std::is_reference_v<RootType> && !std::is_const_v<RootType>
	);

public:

	sequence_rule_2(const HeadRuleType& p_ruleHead, const TailRuleType& p_ruleTail)
		: m_ruleHead(p_ruleHead), m_ruleTail(p_ruleTail)
	{}

	sequence_rule_2(HeadRuleType&& p_ruleHead, TailRuleType&& p_ruleTail)
		: m_ruleHead(std::move(p_ruleHead)), m_ruleTail(std::move(p_ruleTail))
	{}

	template <typename... RuleAttributeTypes>
	using attribute_template =
		std::tuple<
			typename HeadRuleType::template attribute_template<RuleAttributeTypes...>,
			typename TailRuleType::template attribute_template<RuleAttributeTypes...>
		>;

	template <typename IteratorType, typename SentinelType>
	auto parse(const IteratorType& p_iterator, const SentinelType& p_sentinel, const RootType& p_root) const
	{
		using head_attribute_t = parse_attribute_t<HeadRuleType, IteratorType, SentinelType, RootType>;
		using tail_attribute_t = parse_attribute_t<TailRuleType, IteratorType, SentinelType, RootType>;

		if (auto optpairitattrHead = m_ruleHead.parse(p_iterator, p_sentinel, p_root); optpairitattrHead.has_value())
		{
			if (auto optpairitattrTail = m_ruleTail.parse(optpairitattrHead.value().first, p_sentinel, p_root); optpairitattrTail.has_value())
			{
				return std::make_optional(std::make_pair(
					optpairitattrTail.value().first,
					std::make_tuple(
						std::move(optpairitattrHead.value().second),
						std::move(optpairitattrTail.value().second)
					)
				));
			}
		}
		return std::optional<std::pair<IteratorType, std::tuple<head_attribute_t, tail_attribute_t>>>{std::nullopt};
	}

private:

	HeadRuleType m_ruleHead;
	TailRuleType m_ruleTail;
};

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
	constexpr auto internal_make_attribute(HeadAttributeType&& p_head_attribute, TailAttributeType&& p_tail_attribute) const;

public:

	sequence_rule(const HeadRuleType& p_ruleHead, const TailRuleType& p_ruleTail)
		: m_ruleHead(p_ruleHead), m_ruleTail(p_ruleTail)
	{}

	sequence_rule(HeadRuleType&& p_ruleHead, TailRuleType&& p_ruleTail)
		: m_ruleHead(std::move(p_ruleHead)), m_ruleTail(std::move(p_ruleTail))
	{}

	template <typename... RuleAttributeTypes>
	using attribute_template =
		decltype(internal_make_attribute(
			std::move(std::declval<typename HeadRuleType::template attribute_template<RuleAttributeTypes...>>()),
			std::move(std::declval<typename TailRuleType::template attribute_template<RuleAttributeTypes...>>())
		));

	template <typename IteratorType, typename SentinelType>
	auto parse(const IteratorType& p_iterator, const SentinelType& p_sentinel, const RootType& p_root) const;

private:

	HeadRuleType m_ruleHead;
	TailRuleType m_ruleTail;
};

template <typename RuleType>
struct is_sequence_rule : std::false_type {};

template <typename HeadRuleType, typename TailRuleType, typename RootType>
struct is_sequence_rule<sequence_rule<HeadRuleType, TailRuleType, RootType>> : std::true_type {};

template <typename RuleType>
inline constexpr bool is_sequence_rule_v = is_sequence_rule<RuleType>::value;

template <typename RuleType>
struct is_tuple : std::false_type {};

template <typename... Types>
struct is_tuple<std::tuple<Types...>> : std::true_type {};

template <typename Type>
inline constexpr bool is_tuple_v = is_tuple<Type>::value;

template <typename HeadRuleType, typename TailRuleType, typename RootType>
template <typename HeadAttributeType, typename TailAttributeType>
constexpr auto sequence_rule<HeadRuleType, TailRuleType, RootType>::internal_make_attribute(HeadAttributeType&& p_head_attribute, TailAttributeType&& p_tail_attribute) const
{
	p_head_attribute;
	p_tail_attribute;
	using head_attribute_t = std::decay_t<HeadAttributeType>;
	using tail_attribute_t = std::decay_t<TailAttributeType>;
	if constexpr (is_null_attribute_v<head_attribute_t>)
	{
		return std::move(p_tail_attribute);
	}
	else if constexpr (is_null_attribute_v<tail_attribute_t>)
	{
		return std::move(p_head_attribute);
	}
	else if constexpr (is_sequence_rule_v<HeadRuleType> && is_tuple_v<head_attribute_t> && is_sequence_rule_v<TailRuleType> && is_tuple_v<tail_attribute_t>)
	{
		return std::tuple_cat(std::move(p_head_attribute), std::move(p_tail_attribute));
	}
	else if constexpr (is_sequence_rule_v<HeadRuleType> && is_tuple_v<head_attribute_t>)
	{
		return std::tuple_cat(std::move(p_head_attribute), std::make_tuple(std::move(p_tail_attribute)));
	}
	else if constexpr (is_sequence_rule_v<TailRuleType> && is_tuple_v<tail_attribute_t>)
	{
		return std::tuple_cat(std::make_tuple(std::move(p_head_attribute)), std::move(p_tail_attribute));
	}
	else
	{
		return std::make_tuple(std::move(p_head_attribute), std::move(p_tail_attribute));
	}
};

template <typename HeadRuleType, typename TailRuleType, typename RootType>
template <typename IteratorType, typename SentinelType>
auto sequence_rule<HeadRuleType, TailRuleType, RootType>::parse(const IteratorType& p_iterator, const SentinelType& p_sentinel, const RootType& p_root) const
{
	using head_attribute_t = parse_attribute_t<HeadRuleType, IteratorType, SentinelType, RootType>;
	using tail_attribute_t = parse_attribute_t<TailRuleType, IteratorType, SentinelType, RootType>;

	if (auto optpairitattrHead = m_ruleHead.parse(p_iterator, p_sentinel, p_root); optpairitattrHead.has_value())
	{
		if (auto optpairitattrTail = m_ruleTail.parse(optpairitattrHead.value().first, p_sentinel, p_root); optpairitattrTail.has_value())
		{
			return std::make_optional(std::make_pair(
				optpairitattrTail.value().first,
				internal_make_attribute(
					std::move(optpairitattrHead.value().second),
					std::move(optpairitattrTail.value().second)
				)
			));
		}
	}
	return std::optional<std::pair<IteratorType, decltype(internal_make_attribute(std::move(std::declval<head_attribute_t>()), std::move(std::declval<tail_attribute_t>())))>>{std::nullopt};
}

template <typename HeadRuleDefinitionType, typename TailRuleDefinitionType>
struct sequence_rule_definition : base_rule_definition
{
	sequence_rule_definition(const HeadRuleDefinitionType& p_rule1, const TailRuleDefinitionType& p_rule2)
		: m_ruleHead(p_rule1), m_ruleTail(p_rule2)
	{}

	sequence_rule_definition(HeadRuleDefinitionType&& p_rule1, TailRuleDefinitionType&& p_rule2)
		: m_ruleHead(std::move(p_rule1)), m_ruleTail(std::move(p_rule2))
	{}

	template <typename RootType>
	using rule_template = sequence_rule_2<
		typename HeadRuleDefinitionType::template rule_template<RootType>,
		typename TailRuleDefinitionType::template rule_template<RootType>,
		RootType
	>;

	template <typename RootType>
	constexpr auto create()
	{
		return sequence_rule_2<
			decltype(m_ruleHead.create<RootType>()),
			decltype(m_ruleTail.create<RootType>()),
			RootType
		>(
			m_ruleHead.create<RootType>(),
			m_ruleTail.create<RootType>()
		);
	}

	HeadRuleDefinitionType m_ruleHead;
	TailRuleDefinitionType m_ruleTail;
};

template <typename HeadRuleDefinitionType, typename TailRuleDefinitionType>
constexpr auto sequence(HeadRuleDefinitionType&& ruleHead, TailRuleDefinitionType&& ruleTail)
{
	return sequence_rule_definition<std::decay_t<HeadRuleDefinitionType>, std::decay_t<TailRuleDefinitionType>>(std::forward<HeadRuleDefinitionType>(ruleHead), std::forward<TailRuleDefinitionType>(ruleTail));
};

template <typename RuleType1, typename RuleType2, std::enable_if_t<std::is_base_of_v<base_rule_definition, RuleType1>&& std::is_base_of_v<base_rule_definition, RuleType2>, int> = 0>
constexpr auto operator>>(RuleType1&& p_rule1, RuleType2&& p_rule2)
{
	return sequence(std::forward<RuleType1>(p_rule1), std::forward<RuleType2>(p_rule2));
}

//----------------------------//

template <typename RuleType1, typename RuleType2, typename RootType>
class alternative_rule
{
private:

	static_assert(
		!std::is_reference_v<RuleType1> && !std::is_const_v<RuleType1> &&
		!std::is_reference_v<RuleType2> && !std::is_const_v<RuleType2> &&
		!std::is_reference_v<RootType> && !std::is_const_v<RootType>
	);

	template <typename AttributeType1, typename AttributeType2>
	constexpr auto internal_make_attribute1(AttributeType1&& p_attribute1) const;

	template <typename AttributeType1, typename AttributeType2>
	constexpr auto internal_make_attribute2(AttributeType2&& p_attribute2) const;

public:

	alternative_rule(const RuleType1& p_rule1, const RuleType2& p_rule2)
		: m_rule1(p_rule1), m_rule2(p_rule2)
	{}

	alternative_rule(RuleType1&& p_rule1, RuleType2&& p_rule2)
		: m_rule1(std::move(p_rule1)), m_rule2(std::move(p_rule2))
	{}

	template <typename... RuleAttributeTypes>
	using attribute_template =
		decltype(
			internal_make_attribute1<
				typename RuleType1::template attribute_template<RuleAttributeTypes...>,
				typename RuleType2::template attribute_template<RuleAttributeTypes...>
			>(
				std::move(std::declval<typename RuleType1::template attribute_template<RuleAttributeTypes...>>())
			)
		);

	template <typename IteratorType, typename SentinelType>
	auto parse(const IteratorType& p_iterator, const SentinelType& p_sentinel, const RootType& p_root) const;

private:

	RuleType1 m_rule1;
	RuleType2 m_rule2;
};

template <typename RuleType>
struct is_alternative_rule : std::false_type {};

template <typename RuleType1, typename RuleType2, typename RootType>
struct is_alternative_rule<alternative_rule<RuleType1, RuleType2, RootType>> : std::true_type {};

template <typename RuleType>
inline constexpr bool is_alternative_rule_v = is_alternative_rule<RuleType>::value;

template <typename RuleType>
struct is_variant : std::false_type {};

template <typename... Types>
struct is_variant<std::variant<Types...>> : std::true_type {};

template <typename Type>
inline constexpr bool is_variant_v = is_variant<Type>::value;

template <typename RuleType1, typename RuleType2, typename RootType>
template <typename AttributeType1, typename AttributeType2>
constexpr auto alternative_rule<RuleType1, RuleType2, RootType>::internal_make_attribute1(AttributeType1&& p_attribute1) const
{
	using attribute1_t = std::decay_t<AttributeType1>;
	using attribute2_t = std::decay_t<AttributeType2>;
	if constexpr (is_alternative_rule_v<RuleType1> && is_variant_v<attribute1_t> && is_alternative_rule_v<RuleType2> && is_variant_v<attribute2_t>)
	{
		return std::visit(
			[](auto& p_attribute) constexpr
			{
				using variant_return_type = tuple_to_variant<unique_t<
					decltype(std::tuple_cat(
						std::declval<variant_to_tuple_t<attribute1_t>>(),
						std::declval<variant_to_tuple_t<attribute2_t>>()
					))
				>>;
				return variant_return_type(std::move(p_attribute));
			},
			p_attribute1
		);
	}
	else if constexpr (is_alternative_rule_v<RuleType1> && is_variant_v<attribute1_t>)
	{
		using variant_return_type = tuple_to_variant<unique_t<
			decltype(std::tuple_cat(
				std::declval<variant_to_tuple_t<attribute1_t>>(),
				std::declval<std::tuple<attribute2_t>>()
			))
		>>;
		return std::visit(
			[](auto& p_attribute) constexpr
			{
				return variant_return_type(std::move(p_attribute));
			},
			p_attribute1
		);
	}
	else if constexpr (is_alternative_rule_v<RuleType2> && is_variant_v<attribute2_t>)
	{
		using variant_return_type = tuple_to_variant<unique_t<
			decltype(std::tuple_cat(
				std::declval<std::tuple<attribute1_t>>(),
				std::declval<variant_to_tuple_t<attribute2_t>>()
			))
		>>;
		return variant_return_type(std::move(p_attribute1));
	}
	else if constexpr (is_variant_v<attribute1_t> && is_variant_v<attribute2_t> && equivalent_v<variant_to_tuple_t<attribute1_t>, variant_to_tuple_t<attribute2_t>>)
	{
		return std::visit(
			[](auto& p_attribute) constexpr
			{
				return attribute1_t(std::move(p_attribute));
			},
			p_attribute1
		);
	}
	else if constexpr (std::is_same_v<attribute1_t, attribute2_t>)
	{
		return p_attribute1;
	}
	else
	{
		return std::variant<attribute1_t, attribute2_t>(p_attribute1);
	}
};

template <typename RuleType1, typename RuleType2, typename RootType>
template <typename AttributeType1, typename AttributeType2>
constexpr auto alternative_rule<RuleType1, RuleType2, RootType>::internal_make_attribute2(AttributeType2&& p_attribute2) const
{
	using attribute1_t = std::decay_t<AttributeType1>;
	using attribute2_t = std::decay_t<AttributeType2>;
	if constexpr (is_alternative_rule_v<RuleType1> && is_variant_v<attribute1_t> && is_alternative_rule_v<RuleType2> && is_variant_v<attribute2_t>)
	{
		return std::visit(
			[](auto& p_attribute) constexpr
			{
				using variant_return_type = tuple_to_variant<unique_t<
					decltype(std::tuple_cat(
						std::declval<variant_to_tuple_t<attribute1_t>>(),
						std::declval<variant_to_tuple_t<attribute2_t>>()
					))
				>>;
				return variant_return_type(std::move(p_attribute));
			},
			p_attribute2
		);
	}
	else if constexpr (is_alternative_rule_v<RuleType1> && is_variant_v<attribute1_t>)
	{
		using variant_return_type = tuple_to_variant<unique_t<
			decltype(std::tuple_cat(
				std::declval<variant_to_tuple_t<attribute1_t>>(),
				std::declval<std::tuple<attribute2_t>>()
			))
		>>;
		return std::visit(
			[](auto& p_attribute) constexpr
			{
				return variant_return_type(std::move(p_attribute));
			},
			p_attribute2
		);
	}
	else if constexpr (is_alternative_rule_v<RuleType2> && is_variant_v<attribute2_t>)
	{
		using variant_return_type = tuple_to_variant<unique_t<
			decltype(std::tuple_cat(
				std::declval<std::tuple<attribute1_t>>(),
				std::declval<variant_to_tuple_t<attribute2_t>>()
			))
		>>;
		return variant_return_type(std::move(p_attribute2));
	}
	else if constexpr (is_variant_v<attribute1_t> && is_variant_v<attribute2_t> && equivalent_v<variant_to_tuple_t<attribute1_t>, variant_to_tuple_t<attribute2_t>>)
	{
		return std::visit(
			[](auto& p_attribute) constexpr
			{
				return attribute1_t(std::move(p_attribute));
			},
			p_attribute2
		);
	}
	else if constexpr (std::is_same_v<attribute1_t, attribute2_t>)
	{
		return p_attribute2;
	}
	else
	{
		return std::variant<attribute1_t, attribute2_t>(p_attribute2);
	}
};

template <typename RuleType1, typename RuleType2, typename RootType>
template <typename IteratorType, typename SentinelType>
auto alternative_rule<RuleType1, RuleType2, RootType>::parse(const IteratorType& p_iterator, const SentinelType& p_sentinel, const RootType& p_root) const
{
	using attribute1_t = parse_attribute_t<RuleType1, IteratorType, SentinelType, RootType>;
	using attribute2_t = parse_attribute_t<RuleType2, IteratorType, SentinelType, RootType>;

	if (auto optpairitattr = m_rule1.parse(p_iterator, p_sentinel, p_root); optpairitattr.has_value())
	{
		return std::make_optional(std::make_pair(
			optpairitattr.value().first,
			internal_make_attribute1<attribute1_t, attribute2_t>(std::move(optpairitattr.value().second))
		));
	}
	else if (auto optpairitattr2 = m_rule2.parse(p_iterator, p_sentinel, p_root); optpairitattr2.has_value())
	{
		return std::make_optional(std::make_pair(
			optpairitattr2.value().first,
			internal_make_attribute2<attribute1_t, attribute2_t>(std::move(optpairitattr2.value().second))
		));
	}
	else
	{
		return std::optional<std::pair<IteratorType, decltype(internal_make_attribute1<attribute1_t, attribute2_t>(std::declval<attribute1_t>()))>>{std::nullopt};
	}
}

template <typename RuleDefinitionType1, typename RuleDefinitionType2>
struct alternative_rule_definition : base_rule_definition
{
	alternative_rule_definition(const RuleDefinitionType1& p_rule1, const RuleDefinitionType2& p_rule2)
		: m_rule1(p_rule1), m_rule2(p_rule2)
	{}

	alternative_rule_definition(RuleDefinitionType1&& p_rule1, RuleDefinitionType2&& p_rule2)
		: m_rule1(std::move(p_rule1)), m_rule2(std::move(p_rule2))
	{}

	template <typename RootType>
	using rule_template = alternative_rule<typename RuleDefinitionType1::template rule_template<RootType>, typename RuleDefinitionType2::template rule_template<RootType>, RootType>;

	template <typename RootType>
	constexpr auto create()
	{
		return alternative_rule<
			decltype(m_rule1.create<RootType>()),
			decltype(m_rule2.create<RootType>()),
			RootType
		>(
			m_rule1.create<RootType>(),
			m_rule2.create<RootType>()
		);
	}

	RuleDefinitionType1 m_rule1;
	RuleDefinitionType2 m_rule2;
};

template <typename RuleDefinitionType1, typename RuleDefinitionType2>
constexpr auto alternative(RuleDefinitionType1&& rule1, RuleDefinitionType2&& rule2)
{
	return alternative_rule_definition<std::decay_t<RuleDefinitionType1>, std::decay_t<RuleDefinitionType2>>(std::forward<RuleDefinitionType1>(rule1), std::forward<RuleDefinitionType2>(rule2));
};

template <typename RuleType1, typename RuleType2, std::enable_if_t<std::is_base_of_v<base_rule_definition, std::decay_t<RuleType1>> && std::is_base_of_v<base_rule_definition, std::decay_t<RuleType2>>, int> = 0>
constexpr auto operator|(RuleType1&& p_rule1, RuleType2&& p_rule2)
{
	return alternative(std::forward<RuleType1>(p_rule1), std::forward<RuleType2>(p_rule2));
}

//----------------------------//

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
struct any_rule_definition : base_rule_definition
{

	template <typename RootType>
	using rule_template = any_rule<ElementType, RootType>;

	template <typename RootType>
	constexpr auto create()
	{
		return any_rule<ElementType, RootType>();
	}
};

template <typename ElementType>
constexpr auto any()
{
	return any_rule_definition<ElementType>();
};

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
	using attribute_template = null_attribute;

	template <typename IteratorType, typename SentinelType>
	auto parse(const IteratorType& p_iterator, const SentinelType& p_sentinel, const RootType&) const
	{
		return p_sentinel != p_iterator && m_element == *p_iterator ? std::make_optional(std::make_pair(std::next(p_iterator), null_attribute{})) : std::nullopt;
	}
private:
	const ElementType m_element;
};

template <typename ElementType>
struct single_rule_definition : base_rule_definition
{
	single_rule_definition(const ElementType& p_element)
		: m_element(p_element)
	{}

	single_rule_definition(ElementType&& p_element)
		: m_element(std::move(p_element))
	{}

	template <typename RootType>
	using rule_template = single_rule<ElementType, RootType>;

	template <typename RootType>
	constexpr auto create()
	{
		return single_rule<ElementType, RootType>(std::move(m_element));
	}

	ElementType m_element;
};

template <typename ElementType>
constexpr auto single(ElementType&& p_element)
{
	return single_rule_definition<ElementType>(std::forward<ElementType>(p_element));
};

template <template<typename...> typename AttributeTemplate, template<typename...> typename... AttributeTemplates>
class recursive_attribute
{
public:
	std::unique_ptr<AttributeTemplate<recursive_attribute<AttributeTemplates, AttributeTemplates...>...>> m_attribute;
};

template<template <typename...> typename AttributeTemplate, typename... RuleTypes>
using attribute_type = AttributeTemplate<recursive_attribute<typename RuleTypes::attribute_template, typename RuleTypes::template attribute_template...>...>;

template <std::size_t RuleIndex, typename RootType>
class recurse_rule
{
public:

	template <typename... RuleAttributeTypes>
	using attribute_template = std::tuple_element_t<RuleIndex, std::tuple<RuleAttributeTypes...>>;

	template <typename IteratorType, typename SentinelType>
	std::optional<std::pair<IteratorType, typename RootType::template rule_traits<RuleIndex>::template attribute_type>> parse(const IteratorType& p_iterator, const SentinelType& p_sentinel, const RootType& p_root) const
	{
		auto optpairitattr = p_root.template rule<RuleIndex>().parse(p_iterator, p_sentinel, p_root);
		if (optpairitattr.has_value())
		{
			return std::make_pair(std::move(optpairitattr.value().first), typename RootType::template rule_traits<RuleIndex>::template attribute_type{std::make_unique<decltype(optpairitattr.value().second)>(std::move(optpairitattr.value().second))});
		}
		else
		{
			return std::nullopt;
		}
	}

};

template <std::size_t Index>
struct recurse_rule_definition : base_rule_definition
{
	template <typename RootType>
	using rule_template = recurse_rule<Index, RootType>;

	template <typename RootType>
	constexpr recurse_rule<Index, RootType> create()
	{
		return recurse_rule<Index, RootType>();
	}
};

template <std::size_t Index>
constexpr recurse_rule_definition<Index> recurse()
{
	return recurse_rule_definition<Index>();
};

//----------------------------//

template <typename... RuleDefinitionTypes>
struct root
{
public:

	template <typename RuleDefinitionType>
	using rule_type = typename RuleDefinitionType::template rule_template<root<RuleDefinitionTypes...>>;

	template <std::size_t RuleIndex>
	struct rule_traits
	{
		template <typename... RuleAttributeTypes>
		using attribute_template = typename rule_type<std::tuple_element_t<RuleIndex, std::tuple<RuleDefinitionTypes...>>>::template attribute_template<RuleAttributeTypes...>;

		template <typename IndexSequence>
		struct attribute_type_details
		{};

		template <std::size_t... Indices>
		struct attribute_type_details<std::index_sequence<Indices...>>
		{
			using type = recursive_attribute<typename rule_traits<RuleIndex>::template attribute_template, typename rule_traits<Indices>::template attribute_template...>;
		};

		using attribute_type = typename attribute_type_details<std::make_index_sequence<sizeof...(RuleDefinitionTypes)>>::type;
	};

	root(RuleDefinitionTypes&&... p_rules)
		: m_ruleInstance(
			p_rules.create<root<RuleDefinitionTypes...>>()...
		)
	{}

	template <typename IteratorType, typename SentinelType>
	auto parse(const IteratorType& p_iterator, const SentinelType& p_sentinel) const
	{
		return rule<0>().parse(p_iterator, p_sentinel, *this);
	}

	template <std::size_t Index>
	const auto& rule() const
	{
		return std::get<Index>(m_ruleInstance);
	}

private:

	std::tuple<rule_type<RuleDefinitionTypes>...> m_ruleInstance;

};

template <typename... RuleDefinitionTypes>
auto make_root(RuleDefinitionTypes&&... p_rule_definitions)
{
	return root<RuleDefinitionTypes...>(std::forward<RuleDefinitionTypes>(p_rule_definitions)...);
}

template <typename RootType, typename IteratorType, typename SentinelType>
bool parse(const RootType& p_root, const IteratorType& p_iterator, const SentinelType& p_sentinel)
{
	return p_root.parse(p_iterator, p_sentinel).has_value();
}

int main()
{
	//auto rule = make_rule(sequence_rule(recurse_rule(), recurse_rule()));
	auto root = make_root(
		single('A') >> recurse<0>()
	);
	//auto rule = make_rule(single('A'));

	std::string a = "b";
	std::string b = "a";
	std::string c = "{a}";
	//std::cout << parse(root, std::begin(a), std::end(a)) << std::endl;
	//std::cout << parse(root, std::begin(b), std::end(b)) << std::endl;
	//std::cout << parse(root, std::begin(c), std::end(c)) << std::endl;
	std::cin >> a;
	return 0;
}
