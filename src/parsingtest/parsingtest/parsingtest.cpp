#include <iostream>
#include <optional>
#include <tuple>
#include <variant>
#include <type_traits>
#include <memory>

#include <parsingtest/sequence.h>
#include <parsingtest/root.h>
#include <parsingtest/single.h>
#include <parsingtest/any.h>


//----------------------------//

//----------------------------//

//template <typename Type, typename Types>
//struct filter
//{
//	using type = std::tuple<>;
//};
//
//template <typename Type, typename... Types>
//struct filter<Type, std::tuple<Type, Types...>>
//{
//	using type = typename filter<Type, std::tuple<Types...>>::type;
//};
//
//template <typename Type, typename Type1, typename... Types>
//struct filter<Type, std::tuple<Type1, Types...>>
//{
//	using type = std::conditional_t <
//		std::is_same_v<Type, Type1>,
//		typename filter<Type, std::tuple<Types...>>::type,
//		decltype(
//			std::tuple_cat(
//				std::declval<std::tuple<Type1>>(),
//				std::declval<typename filter<Type, std::tuple<Types...>>::type>()
//			)
//		)
//	>;
//};
//
//template <typename Type, typename Types>
//using filter_t = typename filter<Type, Types>::type;
//
////----------------------------//
//
//template <typename Types>
//struct unique
//{
//	using type = std::tuple<>;
//};
//
//template <typename Type, typename... Types>
//struct unique<std::tuple<Type, Types...>>
//{
//	using type = decltype(std::tuple_cat(
//		std::declval<std::tuple<Type>>(),
//		std::declval<typename unique<filter_t<Type, std::tuple<Types...>>>::type>()
//	));
//};
//
//template <typename Types>
//using unique_t = typename unique<Types>::type;
//
////----------------------------//
//
//template <typename Tuple>
//struct tuple_to_variant;
//
//template <typename... Types>
//struct tuple_to_variant<std::tuple<Types...>>
//{
//	using type = std::variant<Types...>;
//};
//
//template <typename Tuple>
//using tuple_to_variant_t = typename std::variant<Tuple>::type;
//
//template <typename Variant>
//struct variant_to_tuple
//{
//	using type = std::tuple<>;
//};
//
//template <typename... Types>
//struct variant_to_tuple<std::variant<Types...>>
//{
//	using type = std::tuple<Types...>;
//};
//
//template <typename Variant>
//using variant_to_tuple_t = typename variant_to_tuple<Variant>::type;
//
//template <typename Type, typename Types>
//struct contains : std::false_type {};
//
//template <typename Type>
//struct contains<Type, std::tuple<>> : std::false_type {};
//
//template <typename Type, typename Type1, typename... Types>
//struct contains<Type, std::tuple<Type1, Types...>> :
//	std::bool_constant<
//		std::is_same_v<Type, Type1> ||
//		contains<Type, std::tuple<Types...>>::value
//	>
//{};
//
//template <typename Type, typename Types>
//inline constexpr bool contains_v = contains<Type, Types>::value;
//
//template <typename Tuple1, typename Tuple2>
//struct contains_all : std::true_type {};
//
//template <typename Type, typename... Types, typename Tuple2>
//struct contains_all<std::tuple<Type, Types...>, Tuple2> : std::integral_constant<bool, contains_v<Type, Tuple2>&& contains_all<std::tuple<Types...>, Tuple2>::value> {};
//
//template <typename Tuple1, typename Tuple2>
//inline constexpr bool contains_all_v = contains_all<Tuple1, Tuple2>::value;
//
//template <typename Tuple1, typename Tuple2>
//struct equivalent : std::integral_constant<bool, contains_all_v<Tuple1, Tuple2>&& contains_all_v<Tuple2, Tuple1>> {};
//
//template <typename Tuple1, typename Tuple2>
//inline constexpr bool equivalent_v = equivalent<Tuple1, Tuple2>::value;

//----------------------------//

int main()
{
	//auto rule = make_rule(sequence_rule(recurse_rule(), recurse_rule()));
	auto root = make_root(
		any<char>() >> recurse<0>()
	);
	//auto rule = make_rule(single('A'));

	std::string a = "AAA";
	std::string b = "a";
	std::string c = "{a}";
	//
	auto t = parse(root, std::begin(a), std::end(a));
	//std::cout << parse(root, std::begin(b), std::end(b)) << std::endl;
	//std::cout << parse(root, std::begin(c), std::end(c)) << std::endl;
	std::cin >> a;
	return 0;
}
