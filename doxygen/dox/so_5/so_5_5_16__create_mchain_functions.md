# so-5.5.16: create_mchain helper functions {#so_5_5_16__create_mchain_functions}

Several new helper functions `so_5::create_mchain` have been added in v.5.5.16. They simplify creation of mchains. 

For example, creation of size-unlimited mchain can looks like:

~~~~~{.cpp}
so_5::environment_t & env = ...;
auto ch = create_mchain( env );
~~~~~

instead of:

~~~~~{.cpp}
so_5::environment_t & env = ...;
auto ch = env.create_mchain( so_5::make_unlimited_mchain_params() );
~~~~~

There are also `create_mchain` functions for creation of size-limited mchains (with or without waiting on overflow):

~~~~~{.cpp}
so_5::environment_t & env = ...;

// Creation of size-limited mchain without waiting on overflow.
auto ch1 = create_mchain( env,
    // No more than 200 messages in the chain.
    200,
    // Memory will be allocated dynamically.
    so_5::mchain_props::memory_usage_t::dynamic,
    // New messages will be ignored on chain's overflow.
    so_5::mchain_props::overflow_reaction_t::drop_newest );
    
// Creation of size-limited mchain with waiting on overflow.
auto ch2 = create_mchain( env,
    // Wait for 150ms.
    std::chrono::milliseconds{150},
    // No more than 200 messages in the chain.
    200,
    // Memory will be allocated dynamically.
    so_5::mchain_props::memory_usage_t::dynamic,
    // New messages will be ignored on chain's overflow.
    so_5::mchain_props::overflow_reaction_t::drop_newest );
~~~~~

Please note that the first argument for `create_mchain` function can be a reference to `environment_t` or to `wrapped_env_t` object. It allows to write:

~~~~~{.cpp}
so_5::wrapped_env_t sobj{...};
auto ch = create_mchain(sobj);
~~~~~

instead of:

~~~~~{.cpp}
so_5::wrapped_env_t sobj{...};
auto ch = create_mchain(sobj.environment());
~~~~~

