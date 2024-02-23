#pragma once

#include <vector>
#include <string>
#include <utility>

#include <so_5/subscription_storage_fwd.hpp>

[[nodiscard]]
std::vector< std::pair< std::string, so_5::subscription_storage_factory_t > >
build_subscr_storage_factories()
	{
		using namespace std::string_literals;
		return {
				{ "vector[1]"s, so_5::vector_based_subscription_storage_factory( 1 ) }
			,	{ "vector[8]"s, so_5::vector_based_subscription_storage_factory( 8 ) }
			,	{ "vector[16]"s, so_5::vector_based_subscription_storage_factory( 16 ) }
			,	{ "map"s, so_5::map_based_subscription_storage_factory() }
			,	{ "hash_table"s, so_5::hash_table_based_subscription_storage_factory() }
			,	{ "flat_set[1]"s, so_5::flat_set_based_subscription_storage_factory( 1 ) }
			,	{ "flat_set[8]"s, so_5::flat_set_based_subscription_storage_factory( 8 ) }
			,	{ "flat_set[16]"s, so_5::flat_set_based_subscription_storage_factory( 16 ) }
			,	{ "default"s, so_5::default_subscription_storage_factory() }
		};
	}

