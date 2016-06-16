# so-5.5.17: mchain_master_handler_t helper class {#so_5_5_17__mchain_master_handle}

There are cases when a mchain is created at the beginning of a scope and the mchain must be automatically closed at the end of the scope. Something like:

~~~~~{.cpp}
void some_work_distributor(so_5::environment_t & env)
{
    auto ch = create_mchain(env);
    ... // Some stuff.
    close_drop_content(ch);
}
~~~~~

The code shown above is not good and it can lead to errors in the case if some exception is raised between `create_mchain` and `close_drop_content` calls. It is better to use RAII idiom. Since v.5.5.16, there are `auto_close_mchains` helpers which allows us to write as follows:

~~~~~{.cpp}
void some_work_distributor(so_5::environment_t & env)
{
    auto ch = create_mchain(env);
    auto ch_closer = auto_close_mchains(
        so_5::mchain_props::close_mode_t::drop_content, ch);
    ... // Some stuff.
}
~~~~~

It is better but still it has some drawbacks: a user can forget to call `auto_close_mchains` or can forget to store the value returned. Which is why a new helper class `mchain_master_handle_t` was introduced in v.5.5.17. This class is intended to be used in the cases described above:

~~~~~{.cpp}
void some_work_distributor(so_5::environment_t & env)
{
    so_5::mchain_master_handle_t ch(
        so_5::mchain_props::close_mode_t::drop_content,
        so_5::create_mchain(env) );
    ... // Some stuff.
}
~~~~~

Note that `auto_close_mchains` helpers are good tools for cases when it is necessary to work with already created mchains. For example:

~~~~~{.cpp}
// Mchain is created by someone else.
void some_work_distributor(so_5::mchain_t ch)
{
    auto ch_closer = auto_close_mchains(
        so_5::mchain_props::close_mode_t::drop_content, ch);
    ... // Some stuff.
}
~~~~~

