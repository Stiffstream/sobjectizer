#pragma once

#include <vector>
#include <string>
#include <utility>

#include <so_5/all.hpp>

std::vector< std::pair< std::string, so_5::mchain_params_t > >
build_mchain_params()
	{
		using namespace std;

		namespace props = so_5::mchain_props;

		vector< pair< string, so_5::mchain_params_t > > params;
		params.emplace_back( "unlimited",
				so_5::make_unlimited_mchain_params() );
		params.emplace_back( "limited(dynamic,nowait)",
				so_5::make_limited_without_waiting_mchain_params(
						5,
						props::memory_usage_t::dynamic,
						props::overflow_reaction_t::drop_newest ) );
		params.emplace_back( "limited(preallocated,nowait)",
				so_5::make_limited_without_waiting_mchain_params(
						5,
						props::memory_usage_t::preallocated,
						props::overflow_reaction_t::drop_newest ) );
		params.emplace_back( "limited(dynamic,wait)",
				so_5::make_limited_with_waiting_mchain_params(
						5,
						props::memory_usage_t::dynamic,
						props::overflow_reaction_t::drop_newest,
						chrono::milliseconds(200) ) );
		params.emplace_back( "limited(preallocated,wait)",
				so_5::make_limited_with_waiting_mchain_params(
						5,
						props::memory_usage_t::preallocated,
						props::overflow_reaction_t::drop_newest,
						chrono::milliseconds(200) ) );

		return params;
	}

