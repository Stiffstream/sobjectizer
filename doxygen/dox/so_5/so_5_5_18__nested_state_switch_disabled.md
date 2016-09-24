# so-5.5.18: Nested state switch is explicitly disabled {#so_5_5_18__nested_state_switch}

After adding handlers of  entry/exit in/out agent’s state to SObjectizer there
is an opportunity to make an error: in `on_enter/on_exit` the developer can
initiate the switch-over to another state. Something similar to:

~~~~~{.cpp}
class demo : public so_5::agent_t {
  state_t st_one{ this };
  state_t st_two{ this };
  ...
  virtual void so_define_agent() override {
    st_one.on_enter( [this]{ st_two.activate(); } );
    ...
  }
}; 
~~~~~

Such behavior is wrong and can lead to serious problems:

First, there can be wrong call order of `on_enter/on_exit`. Because states can
be nested, the procedure of changing state can lead to calling of several
different `on_enter/on_exit` handlers. Repeated state changing doesn’t
interrupt this chain but only "embeds" one more chain of calls. When the new
"embedded" chain finishes working the execution will be returned to the
original chain of events which can’t be treated as actual.

Second, such state changing doesn’t lead to the desired result. If the transfer
procedure to state A is being performed and a nested switch to state B is
initiated then the agent will have state A after completion of both operations.
It is because the transfer to state B doesn’t interrupt the transfer procedure
to state A, but performs sort of "embedding" to this transfer.

Third, such nested state changes can lead to infinite loop. For example,
`on_enter` for A performs changing to B while `on_enter` for B performs
changing to A. In this case the application will abort due to stack overflow.

An attempt to change agent's state inside `on_enter/on_exit` is the developer's
mistake. One shouldn’t change agent’s state inside `on_enter/on_exit`. But
before version 5.5.18 SObjectizer didn’t consider this case.

Since version 5.5.18 when changing agent’s state SObjectizer verifies if such
an operation is already being run. If there is such an operation then the
exception is generated. So now SObjectizer will explicitly notify the developer
if he tries to change agent’s state in `on_enter/on_exit` handlers.

**Note**: `on_enter/on_exit` handlers should be noexcept-functions. It means
that if inside such handler SObjectizer throws an exception due to attempt of
nested state change the application is likely to abort calling `std::terminate`
(due to attempt of throwing an exception from noexcept-function). So now the
code above will crash the application.
