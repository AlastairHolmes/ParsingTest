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
struct any_rule_definition
{
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
struct single_rule_definition
{
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
	return single_rule_definition<ElementType>{std::forward<ElementType>(p_element)};
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
struct recurse_rule_definition
{
	template <typename RootType>
	constexpr recurse_rule<Index, RootType> create()
	{
		return recurse_rule<Index, RootType>();
	}
};

template <std::size_t Index>
constexpr recurse_rule_definition<Index> recurse()
{
	return recurse_rule_definition<Index>{};
};

//----------------------------//

template <typename... RuleDefinitionTypes>
struct root
{
public:

	template <typename RuleDefinitionType>
	using rule_type = decltype(std::declval<RuleDefinitionType>().create<root<RuleDefinitionTypes...>>());

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
auto make_rule(RuleDefinitionTypes&&... p_rule_definitions)
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
	auto rule = make_rule(
		recurse<1>(),
		single('a')
	);
	//auto rule = make_rule(single('A'));

	std::string a = "hello";
	std::string b = "ab";

	std::cout << parse(rule, std::begin(a), std::end(a)) << std::endl;
	//std::cout << parse(rule, std::begin(b), std::end(b)) << std::endl;

	std::cin >> a;
	return 0;
}
