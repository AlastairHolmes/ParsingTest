#include <iostream>
#include <optional>
#include <tuple>
#include <variant>
#include <type_traits>

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

	//----------------------------//

	template <typename Type>
	struct identity
	{
		using type = Type;
	};

}

//----------------------------//

struct null_attribute {};

template <typename AttributeType>
inline constexpr bool is_null_attribute_v = std::is_same_v<AttributeType, null_attribute>;

//----------------------------//

template <typename AttributeType1, typename AttributeType2>
struct join_attributes
{
	using type = std::tuple<AttributeType1, AttributeType2>;
	static type join(AttributeType1&& p_attribute1, AttributeType2&& p_attribute2)
	{
		return std::make_tuple(std::move(p_attribute1), std::move(p_attribute2));
	}
};

template <typename AttributeType2>
struct join_attributes<null_attribute, AttributeType2>
{
	using type = AttributeType2;
	static type join(null_attribute&& p_attribute1, AttributeType2&& p_attribute2)
	{
		return std::move(p_attribute2);
	}
};

template <typename AttributeType1>
struct join_attributes<AttributeType1, null_attribute>
{
	using type = AttributeType1;
	static type join(AttributeType1&& p_attribute1, null_attribute&& p_attribute2)
	{
		return std::move(p_attribute1);
	}
};

template <typename AttributeType1, typename... AttributeTypes2>
struct join_attributes<AttributeType1, std::tuple<AttributeTypes2...>>
{
	using type = std::tuple<AttributeType1, AttributeTypes2...>;
	static type join(AttributeType1&& p_attribute1, std::tuple<AttributeTypes2...>&& p_attribute2)
	{
		return std::tuple_cat(std::make_tuple(std::move(p_attribute1)), std::move(p_attribute2));
	}
};

template <typename... AttributeTypes1, typename AttributeType2>
struct join_attributes<std::tuple<AttributeTypes1...>, AttributeType2>
{
	using type = std::tuple<AttributeTypes1..., AttributeType2>;
	static type join(std::tuple<AttributeTypes1...>&& p_attribute1, AttributeType2&& p_attribute2)
	{
		return std::tuple_cat(std::move(p_attribute1), std::make_tuple(std::move(p_attribute2)));
	}
};

template <typename... AttributeTypes1, typename... AttributeTypes2>
struct join_attributes<std::tuple<AttributeTypes1...>, std::tuple<AttributeTypes2...>>
{
	using type = std::tuple<AttributeTypes1..., AttributeTypes2...>;

	static type join(std::tuple<AttributeTypes1...>&& p_attribute1, std::tuple<AttributeTypes2...>&& p_attribute2)
	{
		return std::tuple_cat(std::move(p_attribute1), std::move(p_attribute2));
	}
};

//----------------------------//

template <template<typename> typename AttributeTemplate>
class recursive_attribute
{
public:
	recursive_attribute(AttributeTemplate<recursive_attribute<AttributeTemplate>>&& p_attribute)
		: attribute(std::make_unique<AttributeTemplate<recursive_attribute<AttributeTemplate>>>(std::move(p_attribute)))
	{}

	std::unique_ptr<AttributeTemplate<recursive_attribute<AttributeTemplate>>> attribute;
};

template<typename RootRuleType>
using recursive_type = recursive_attribute<typename RootRuleType::template attribute_template>;

template<template <typename> typename AttributeTemplate, typename RootRuleType>
using attribute_type = AttributeTemplate<recursive_type<RootRuleType>>;

template <typename RootRuleType>
class recursive_rule_instance
{
public:
	static_assert(!std::is_reference_v<RootRuleType>);
	recursive_rule_instance(const RootRuleType&) {}

	template <typename RecursiveAttributeType>
	using attribute_template = RecursiveAttributeType;

	template <typename IteratorType, typename SentinelType>
	auto parse(const RootRuleType& p_root, const IteratorType& p_iterator, const SentinelType& p_sentinel) const
	{
		auto optpairitattr = p_root.parse(p_root, p_iterator, p_sentinel);
		return optpairitattr.has_value() ?
			std::make_optional(std::make_pair(optpairitattr.value().first, recursive_attribute<typename RootRuleType::template attribute_template>(std::move(optpairitattr.value().second)))) :
			std::nullopt;
	}
};

inline constexpr auto recurse_rule = []()
{
	return [](const auto& p_ruleRoot)
	{
		return recursive_rule_instance<std::decay_t<decltype(p_ruleRoot)>>(p_ruleRoot);
	};
};

//----------------------------//

/*template <typename RootRuleType, typename AltRuleDefinitionType1, typename AltRuleDefinitionType2>
class alternative_rule_instance
{
	template <typename RuleType>
	struct is_alternative : std::false_type {};

	template <typename OtherAltRuleDefinitionType1, typename OtherAltRuleDefinitionType2>
	struct is_alternative<alternative_rule_instance<RootRuleType, OtherAltRuleDefinitionType1, OtherAltRuleDefinitionType2>> : std::true_type {};

	template <typename RuleType, typename AttributeType>
	struct to_tuple
	{
		using type = std::tuple<AttributeType>;
	};

	template <typename RuleType, typename... AttributeTypes>
	struct to_tuple<RuleType, std::variant<AttributeTypes...>>
	{
		using type = std::conditional_t<is_alternative<RuleType>::value, std::tuple<AttributeTypes...>, std::tuple<std::variant<AttributeTypes...>>>;
	};

	template <typename RecursiveType>
	using to_variant_t = traits::variant_t<decltype(std::tuple_cat(
		std::declval<to_tuple<AltRuleDefinitionType1, typename AltRuleDefinitionType1::template attribute_template<RecursiveType>>>(),
		std::declval<to_tuple<AltRuleDefinitionType2, typename AltRuleDefinitionType2::template attribute_template<RecursiveType>>>()
	));

	// null_attribute | null_attribute => null_attribute
	// A | null_attribute => variant<A,null_attribute>
	// null_attribute | A => variant<A,null_attribute>
	// variant<A...> (from alt) | B => variant<unique<A..., B>>
	// B | variant<A...> (from alt) => variant<unique<A..., B>>
	// variant<A...> (from alt) | variant<B...> (from alt) => variant<unique<A...,B...>>

public:

	static_assert(!std::is_reference_v<RootRuleType>);

	template <typename RecursiveType>
	using attribute_template = std::conditional_t<std::variant_size_v<to_variant_t<RecursiveType>> == 1, std::variant_alternative_t<0, to_variant_t<RecursiveType>>, to_variant_t<RecursiveType>>;

	alternative_rule_instance(const AltRuleDefinitionType1& p_ruleAlt1, const AltRuleDefinitionType2& p_ruleAlt2)
		: m_ruleAlt1(p_ruleAlt1), m_ruleAlt2(p_ruleAlt2)
	{}

	alternative_rule_instance(AltRuleDefinitionType1&& p_ruleAlt1, AltRuleDefinitionType2&& p_ruleAlt2)
		: m_ruleAlt1(std::move(p_ruleAlt1)), m_ruleAlt2(std::move(p_ruleAlt2))
	{}

	template <typename IteratorType, typename SentinelType>
	auto parse(const RootRuleType& p_root, const IteratorType& p_iterator, const SentinelType& p_sentinel) const
	{
		using base_variant_t = traits::variant_t<decltype(std::tuple_cat(
			std::declval<to_tuple<AltRuleDefinitionType1, decltype(m_ruleAlt1.parse(p_root, p_iterator, p_sentinel).value().second)>>(),
			std::declval<to_tuple<AltRuleDefinitionType2, decltype(m_ruleAlt2.parse(p_root, p_iterator, p_sentinel).value().second)>>()
		))>;

		using variant_type =
			std::conditional_t<std::variant_size_v<to_variant_t<base_variant_t>> == 1, std::variant_alternative_t<0, to_variant_t<base_variant_t>>, to_variant_t<base_variant_t>>;
			std::conditional_t<
				std::tuple_size_v<tuple_t> == 1,
				std::tuple_element_t<0, tuple_t>,
				traits::variant_t<decltype(std::tuple_cat(
					std::declval<to_tuple<AltRuleDefinitionType1, decltype(m_ruleAlt1.parse(p_root, p_iterator, p_sentinel).value().second)>>(),
					std::declval<to_tuple<AltRuleDefinitionType2, decltype(m_ruleAlt2.parse(p_root, p_iterator, p_sentinel).value().second)>>()
				))>
			>;

		auto optpairitattr1 = m_ruleAlt1.parse(p_root, p_iterator, p_sentinel);
		if (optpairitattr1.has_value())
		{
			return std::make_optional(std::make_pair(optpairitattr1.value().first, variant_type(std::move(optpairitattr1.value().second))));
		}
		else
		{
			auto optpairitattr2 = m_ruleAlt2.parse(p_root, p_iterator, p_sentinel);
			if (optpairitattr2.has_value())
			{
				return std::make_optional(std::make_pair(optpairitattr2.value().first, variant_type(std::move(optpairitattr2.value().second))));
			}
		}
		return std::optional<std::pair<IteratorType, tuple_t>>(std::nullopt);
	}

private:

	AltRuleDefinitionType1 m_ruleAlt1;
	AltRuleDefinitionType2 m_ruleAlt2;

};

inline constexpr auto alternative_rule = [](auto&& p_ruledefAlt1, auto&& p_ruledefAlt2)
{
	return[
		ruledefAlt1 = std::forward<decltype(p_ruledefAlt1)>(p_ruledefAlt1),
		ruledefAlt2 = std::forward<decltype(p_ruledefAlt2)>(p_ruledefAlt2)
	](const auto& p_ruleRoot)
	{
		return alternative_rule_instance<
			std::decay_t<decltype(p_ruleRoot)>,
			std::decay_t<decltype(ruledefAlt1(p_ruleRoot))>,
			std::decay_t<decltype(ruledefAlt2(p_ruleRoot))>
		>(ruledefAlt1(p_ruleRoot), ruledefAlt2(p_ruleRoot));
	};
};*/

template <typename RootRuleType, typename HeadRuleDefinitionType, typename TailRuleDefinitionType>
class sequence_rule_instance
{
public:

	static_assert(!std::is_reference_v<RootRuleType>);

	template <typename RecursiveType>
	using joiner_template = join_attributes<
		typename HeadRuleDefinitionType::template attribute_template<RecursiveType>,
		typename TailRuleDefinitionType::template attribute_template<RecursiveType>
	>;

	template <typename RecursiveType>
	using attribute_template = typename joiner_template<RecursiveType>::type;

	sequence_rule_instance(const HeadRuleDefinitionType& p_ruleHead, const TailRuleDefinitionType& p_ruleTail)
		: m_ruleHead(p_ruleHead), m_ruleTail(p_ruleTail)
	{}

	sequence_rule_instance(HeadRuleDefinitionType&& p_ruleHead, TailRuleDefinitionType&& p_ruleTail)
		: m_ruleHead(std::move(p_ruleHead)), m_ruleTail(std::move(p_ruleTail))
	{}

	template <typename IteratorType, typename SentinelType>
	auto parse(const RootRuleType& p_root, const IteratorType& p_iterator, const SentinelType& p_sentinel) const
	{
		using attribute_joiner = joiner_template<recursive_type<RootRuleType>>;

		using return_type = std::optional<std::pair<
			IteratorType,
			typename attribute_joiner::type
		>>;

		auto optpairitattrHead = m_ruleHead.parse(p_root, p_iterator, p_sentinel);
		if (optpairitattrHead.has_value())
		{
			auto optpairitattrTail = m_ruleTail.parse(p_root, optpairitattrHead.value().first, p_sentinel);
			if (optpairitattrTail.has_value())
			{
				return std::make_optional(std::make_pair(
					optpairitattrTail.value().first,
					attribute_joiner::join(
						std::move(optpairitattrHead.value().second),
						std::move(optpairitattrTail.value().second)
					)
				));
			}
		}
		return return_type(std::nullopt);
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
		return sequence_rule_instance<
			std::decay_t<decltype(p_ruleRoot)>,
			std::decay_t<decltype(ruledefHead(p_ruleRoot))>,
			std::decay_t<decltype(ruledefTail(p_ruleRoot))>
		>(ruledefHead(p_ruleRoot), ruledefTail(p_ruleRoot));
	};
};

template <typename RootRuleType, typename ElementType>
class single_rule_instance
{
public:

	static_assert(!std::is_reference_v<RootRuleType>);

	single_rule_instance(const ElementType& p_element)
		: m_element(p_element)
	{}

	single_rule_instance(ElementType&& p_element)
		: m_element(std::move(p_element))
	{}

	template <typename RecursiveType>
	using attribute_template = ElementType;

	template <typename IteratorType, typename SentinelType>
	std::optional<std::pair<IteratorType, ElementType>> parse(const RootRuleType&, const IteratorType& p_iterator, const SentinelType& p_sentinel) const
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
		return single_rule_instance<std::decay_t<decltype(p_ruleRoot)>, std::decay_t<decltype(p_element)>>(std::move(element));
	};
};

template <typename RuleType>
struct root_rule_instance
{
public:

	using rule_instance_type = decltype(std::declval<RuleType>()(std::declval<root_rule_instance<RuleType>>()));
	template <typename RecursiveAttributeType>
	using attribute_template = typename rule_instance_type::template attribute_template<RecursiveAttributeType>;

	root_rule_instance(RuleType&& p_rule)
		: m_ruleInstance(std::forward<RuleType>(p_rule)(*this))
	{}

	template <typename IteratorType, typename SentinelType>
	std::optional<std::pair<IteratorType, attribute_template<recursive_attribute<attribute_template>>>> parse(const root_rule_instance&, const IteratorType& p_iterator, const SentinelType& p_sentinel) const
	{
		return m_ruleInstance.parse(*this, p_iterator, p_sentinel);
	}

private:

	rule_instance_type m_ruleInstance;

};

template <typename RuleType>
auto make_rule(RuleType&& p_rule)
{
	return root_rule_instance<RuleType>(std::forward<RuleType>(p_rule));
}


template <typename RuleInstanceType, typename IteratorType, typename SentinelType>
bool parse(const RuleInstanceType& p_ruleInstance, const IteratorType& p_iterator, const SentinelType& p_sentinel)
{
	return p_ruleInstance.parse(p_ruleInstance, p_iterator, p_sentinel).has_value();
}

int main()
{
	//auto rule = make_rule(sequence_rule(recurse_rule(), recurse_rule()));
	auto rule = make_rule(sequence_rule(single_rule('a'), recurse_rule()));

	std::string a = "hello";
	std::string b = "ab";

	//std::cout << parse(rule, std::begin(a), std::end(a)) << std::endl;
	std::cout << parse(rule, std::begin(b), std::end(b)) << std::endl;

	std::cin >> a;
	return 0;
}
