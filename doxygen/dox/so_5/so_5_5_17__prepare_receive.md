# so-5.5.17: prepare_receive and prepare_select functions {#so_5_5_17__prepare_receive}

Usage of ordinary forms of `so_5::receive` functions inside loops could be inefficient because of wasting resources on constructions of internal objects with descriptions of handlers on each `receive()` call. More efficient way is the preparation of all receive params and reusing them later. A combination of `so_5::prepare_receive()` and `so_5::receive()` allows us to do that.

Usage example:
~~~~~{.cpp}
auto prepared = so_5::prepare_receive(
  so_5::from(ch).extract_n(10).empty_timeout(200ms),
  some_handlers... );
...
while( !some_condition )
{
  auto r = so_5::receive( prepared );
  ...
}
~~~~~

The exactly same situation is for `so_5::select` functions: some resources are allocated on each call of ordinary `so_5::select()` function and deallocated at exit. This reallocation can be avoided by using `so_5::prepare_select()` and appropriate `so_5::select()` function:

~~~~~{.cpp}
auto prepared = so_5::prepare_select(
  so_5::from_all().extract_n(10).empty_timeout(200ms),
  case_( ch1, some_handlers... ),
  case_( ch2, more_handlers... ),
  case_( ch3, yet_more_handlers... ) );
...
while( !some_condition )
{
  auto r = so_5::select( prepared );
  ...
}
~~~~~
