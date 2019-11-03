#include <iostream>
#include <optional>
#include <tuple>
#include <variant>
#include <type_traits>
#include <memory>

struct base_rule_definition {};
struct null_attribute {};

template <typename AttributeType>
inline constexpr bool is_null_attribute_v = std::is_same_v<AttributeType, null_attribute>;

//----------------------------//

template <typename... RuleDefinitionTypes>
struct root
{
public:

	template <typename RuleDefinitionType>
	using rule_type = typename RuleDefinitionType::template rule_template<root<RuleDefinitionTypes...>>;

	root(RuleDefinitionTypes&&... p_rules)
		: m_ruleInstance(p_rules.template create<root>()...)
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
auto make_root(RuleDefinitionTypes&& ... p_rule_definitions)
{
	return root<RuleDefinitionTypes...>(std::forward<RuleDefinitionTypes>(p_rule_definitions)...);
}

template <typename RootType, typename IteratorType, typename SentinelType>
bool parse(const RootType& p_root, const IteratorType& p_iterator, const SentinelType& p_sentinel)
{
	return p_root.parse(p_iterator, p_sentinel).has_value();
}

template <typename RootType, template<typename...> typename AttributeTemplate>
struct recursive_attribute;

template <typename RootType, template<typename...> typename AttributeTemplate>
struct attribute_type;

template <typename... RuleDefinitionTypes, template<typename...> typename AttributeTemplate>
struct attribute_type<root<RuleDefinitionTypes...>, AttributeTemplate>
{
	using type = AttributeTemplate<
		recursive_attribute<
			root<RuleDefinitionTypes...>,
			#if defined(__clang__) ||  defined(__GNUC__) || defined(__GNUG__)
				RuleDefinitionTypes::template rule_template<root<RuleDefinitionTypes...>>::template attribute_template
			#elif defined(_MSC_VER)
				typename RuleDefinitionTypes::template rule_template<root<RuleDefinitionTypes...>>::template attribute_template
			#else
				#error "Unknown Compiler"
			#endif
		>...
	>;
};

template <typename RootType, template<typename...> typename AttributeTemplate>
using attribute_type_t = typename attribute_type<RootType, AttributeTemplate>::type;

template <typename RootType, template<typename...> typename AttributeTemplate>
struct recursive_attribute
{
	recursive_attribute(attribute_type_t<RootType, AttributeTemplate>&& p_value) noexcept
		: m_attribute(std::make_unique<attribute_type_t<RootType, AttributeTemplate>>(std::move(p_value)))
	{}

	std::unique_ptr<attribute_type_t<RootType, AttributeTemplate>> m_attribute;
};

template <std::size_t RuleIndex, typename RootType>
class recurse_rule
{
public:

	template <typename... RuleAttributeTypes>
	using attribute_template = std::tuple_element_t<RuleIndex, std::tuple<RuleAttributeTypes...>>;

	template <typename IteratorType, typename SentinelType>
	auto parse(const IteratorType& p_iterator, const SentinelType& p_sentinel, const RootType& p_root) const
	{
		auto optpairitattr = p_root.template rule<RuleIndex>().parse(p_iterator, p_sentinel, p_root);
		if (optpairitattr.has_value())
		{
			return std::make_optional(std::make_pair(
				std::move(optpairitattr.value().first),
				attribute_type_t<RootType, attribute_template>(std::move(optpairitattr.value().second))
			));
		}
		else
		{
			return std::optional<std::pair<IteratorType, attribute_type_t<RootType, attribute_template>>>{std::nullopt};
		}
	}

};

template <std::size_t Index>
struct recurse_rule_definition : base_rule_definition
{
	template <typename RootType>
	using rule_template = recurse_rule<Index, RootType>;

	template <typename RootType>
	constexpr rule_template<RootType> create()
	{
		return rule_template<RootType>();
	}
};

template <std::size_t Index>
constexpr recurse_rule_definition<Index> recurse()
{
	return recurse_rule_definition<Index>();
};

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
		contains<Type, std::tuple<Types...>>::value
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

template <typename RuleType>
struct is_tuple : std::false_type {};

template <typename... Types>
struct is_tuple<std::tuple<Types...>> : std::true_type {};

template <typename Type>
inline constexpr bool is_tuple_v = is_tuple<Type>::value;

//----------------------------//

template <typename HeadRuleType, typename TailRuleType, typename RootType>
class sequence_rule;

template <typename RuleType>
struct is_sequence_rule : std::false_type {};

template <typename HeadRuleType, typename TailRuleType, typename RootType>
struct is_sequence_rule<sequence_rule<HeadRuleType, TailRuleType, RootType>> : std::true_type {};

template <typename RuleType>
inline constexpr bool is_sequence_rule_v = is_sequence_rule<RuleType>::value;

template <typename HeadRuleType, typename TailRuleType, typename RootType>
class sequence_rule
{
private:

	static_assert(
		!std::is_reference_v<HeadRuleType> && !std::is_const_v<HeadRuleType> &&
		!std::is_reference_v<TailRuleType> && !std::is_const_v<TailRuleType> &&
		!std::is_reference_v<RootType> && !std::is_const_v<RootType>
	);

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
	using attribute_template = std::tuple<
		head_attribute_template<RuleAttributeTypes...>,
		tail_attribute_template<RuleAttributeTypes...>
	>;

	template <typename IteratorType, typename SentinelType>
	std::optional<std::pair<IteratorType, attribute_type_t<RootType, attribute_template>>> parse(const IteratorType& p_iterator, const SentinelType& p_sentinel, const RootType& p_root) const
	{
		//using head_attribute_t = parse_attribute_t<HeadRuleType, IteratorType, SentinelType, RootType>;
		//using tail_attribute_t = parse_attribute_t<TailRuleType, IteratorType, SentinelType, RootType>;
		if (auto optpairitattrHead = m_ruleHead.parse(p_iterator, p_sentinel, p_root); optpairitattrHead.has_value())
		{
			if (auto optpairitattrTail = m_ruleTail.parse(optpairitattrHead.value().first, p_sentinel, p_root); optpairitattrTail.has_value())
			{
				return std::make_pair(optpairitattrTail.value().first, std::make_tuple(std::move(optpairitattrHead.value().second), std::move(optpairitattrTail.value().second)));
			}
		}
		return std::nullopt;
	}

private:

	HeadRuleType m_ruleHead;
	TailRuleType m_ruleTail;
};

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

int main()
{
	//auto rule = make_rule(sequence_rule(recurse_rule(), recurse_rule()));
	auto root = make_root(
		single('A') >> recurse<0>()
	);
	//auto rule = make_rule(single('A'));

	std::string a = "AAA";
	std::string b = "a";
	std::string c = "{a}";
	std::cout << parse(root, std::begin(a), std::end(a)) << std::endl;
	//std::cout << parse(root, std::begin(b), std::end(b)) << std::endl;
	//std::cout << parse(root, std::begin(c), std::end(c)) << std::endl;
	std::cin >> a;
	return 0;
}
