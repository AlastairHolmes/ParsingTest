add_executable(parsingtest "")

if(MSVC)
  target_compile_options(parsingtest PRIVATE /W4 /WX /permissive-)
else()
  target_compile_options(parsingtest PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

target_include_directories(parsingtest PRIVATE "${CMAKE_CURRENT_LIST_DIR}/..")
target_sources(parsingtest PRIVATE
	"${CMAKE_CURRENT_LIST_DIR}/parsingtest.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/sequence.h"
	"${CMAKE_CURRENT_LIST_DIR}/utilities.h"
	"${CMAKE_CURRENT_LIST_DIR}/type_traits.h"
	"${CMAKE_CURRENT_LIST_DIR}/recurse.h"
	"${CMAKE_CURRENT_LIST_DIR}/root.h"
	"${CMAKE_CURRENT_LIST_DIR}/any.h"
	"${CMAKE_CURRENT_LIST_DIR}/single.h"
)