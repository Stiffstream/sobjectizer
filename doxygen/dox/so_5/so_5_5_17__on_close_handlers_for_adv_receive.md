# so-5.5.17: on_close handlers for advanced receive and select functions {#so_5_5_17__on_close_handlers_for_adv_receive}

An advancend version of mchain's `receive` and `select` functions are extended: currently it is possible to specify a user-defined function to handle a close event for a mchain.

Let's imagine a situation when several mchains must be handled in `select` function. But the handling must be stopped if one of the chains is closed. With a close event handler it could be done in the following way:

~~~~~{.cpp}
void read_and_handle_several_chains(
    so_5::mchain_t main_chain,
    so_5::mchain_t aux_chain1,
    so_5::mchain_t aux_chain2 )
{
    bool main_chain_closed = false;
    so_5::select( so_5::from_all()
        // Stop handling if main_chain_closes becomes true.
        .stop_on( [&main_chain_closed]{ return main_chain_closed; } )
        // Handle chain close events.
        .on_close(
            [main_chain, &main_chain_closed]( const so_5::mchain_t & ch ) {
                if( ch == main_chain )
                    main_chain_closed = true;
            } ),
        // Message handlers for chains...
        case_( main_chain, ... ),
        case_( aux_chain1, ... ),
        case_( aux_chain2, ... ) );
}
~~~~~
