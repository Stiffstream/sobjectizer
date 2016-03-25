# so-5.5 In Depth - Agent States {#so_5_5__in_depth__agent_states}

# Basic

Agent in SObjectizer is a finite-state machine.

The behaviour of an agent depends on the current state of the agent and the
received message. An agent can receive and process different messages in each
state. In other words an agent can receive a message in one state but ignore it
in another state. Or, if an agent receives the same message in several states,
it can handle the message differently in each state.

Let’s imagine a simple agent which controls LED indicator on some device.

It receives just one signal turn_on_off. When LED indicator is off this signal
should turn indicator on. If LED indicator is on this signal should turn
indicator off.

This logic can be represented by simple statechart:
![](https://svn.code.sf.net/p/sobjectizer/repo/branches/so_5/5.5.15--dev-trunk/doxygen/dox/so_5/led_indicator_statechart.png?p=2181)

It's easy to see that led_indicator agent requires two states: `off` and `on`.
They can be directly expressed in C++ code via usage of `so_5::state_t` class.
The definition of new state for an agent means creation of new instance of
state_t.

States are usually represented as members of agent’s class:

~~~~~{.cpp}
class led_indicator final : public so_5::agent_t
{
   state_t off{ this }, on{ this };
~~~~~

A state can have a textual name:

~~~~~{.cpp}
class led_indicator final : public so_5::agent_t
{
   state_t off{ this, "off" }, on{ this, "on" };
~~~~~

It could be useful for debugging and logging.

There are several ways of changing agent’s state:

~~~~~{.cpp}
// Very old and basic way.
so_change_state( off );

// More modern and short way.
off.activate();

// Yet more modern way.
this >>= off;
~~~~~

The current state can be obtained via `so_current_state()` method:

~~~~~{.cpp}
if( off == so_current_state() )
    ... // 'off' is active state
~~~~~

Method `so_is_active_state()`  can be used for checking of state activity:

~~~~~{.cpp}
if( so_is_active_state(off) )
    ... // 'off' is active state
~~~~~

Every agent already has one state: the default one. The default state can be
accessed via `so_default_state()` method:

~~~~~{.cpp}
// Returning agent to the default state.
this >>= so_default_state();
~~~~~

Even ad-hoc agents have the default state. But it is the only one state they
have. Because there is no user-defined class for an ad-hoc agent then there is
no possibility to define new states for ad-hoc agent. Thus there is no
possibility to change state of ad-hoc agent.

The most important part of usage of agent’s states is subscription to a message
with respect to a specific state. The simple usage of so_subscribe() and
so_subscribe_self() methods leads to subscription only for the default agent’s
state. It means that:

~~~~~{.cpp}
so_subscribe_self().event(...);
~~~~~

is the equivalent of:

~~~~~{.cpp}
so_subscribe_self().in( so_default_state() ).event(...);
~~~~~

To make subscription to a message for a specific state it is necessary to use
in() method in a subscription chain:

~~~~~{.cpp}
so_subscribe_self().in( off ).event(...);
so_subscribe_self().in( on ).event(...);
~~~~~

The in() methods can be chained if the same event handler is used in several
states:

~~~~~{.cpp}
so_subscribe_self()
   .in( off )
   .in( on )
   .event< get_name >( [] -> std::string { return "led_indicator"; } );

so_subscribe_self()
   .in( off )
   .event< get_status >( [] -> std::string { return "off"; } );
~~~~~

There is another way to make subscription for a specific state:

~~~~~{.cpp}
off.event< get_name >( [] -> std::string { return "led_indicator"; } )
   .event< get_status >( [] -> std::string { return "off"; } );

on.event< get_name >( [] -> std::string { return "led_indicator"; } )
   .event(...)
   ...;
~~~~~

There are also on_enter/on_exit methods in state_t class.

Method on_enter sets up an enter handler. Enter handler is automatically called
when the state is activated. Contrary on_exit method sets an exit handler. Exit
handler is automatically called when the state is activated. Enter and exit
handler can be a lambda-function or a pointer to member method of agent's
class. 

 **Note**: *enter and exit handlers must be `noexcept` functions.*

In the led_indicator example enter and exit handlers are necessary for `on`
state:

~~~~~{.cpp}
on.on_enter( [this]{ ... } ); // some device-dependent code.
    .on_exit( [this]{ ... } ); // some device-dependent code.
    ...
~~~~~

And now we can write the full code of led_indicator example:

~~~~~{.cpp}
class led_indicator final : public so_5::agent_t
{
   state_t off{ this, "off" }, on{ this, "on" };

public :
   struct turn_on_off : public so_5::signal_t {};

   led_indicator( context_t ctx ) : so_5::agent_t{ ctx }
   {
      // Initial state for any agent is the default one.
      // We must switch agent to 'off' state manually.
      this >>= off;

      off.event< turn_on_off >( [this]{ this >>= on; } );

      on.on_enter( [this]{ ... } ); // some device-dependent code
        .on_exit( [this]{ ... } ); // some device-dependent code
        .event< turn_on_off >( [this]{ this >>= off; } );
   }
};
~~~~~

The led_indicator example demonstrates simple finite-state machine. Since
v.5.5.15 SObjectizer supports more advanced features of agents' states like
composite states, shallow- and deep-history, time limitations and so on. These
advanced features allow to implement agents as hierarchical state machines.

Let's see an ability to create hierarchical state machines on a slightly
complex example: an agent which implements blinking of LED indicator. This
agent receives `turn_on_off` signal for turning blinking on and off. When
blinking is turned on then agent switches LED indicator on for 1s then switches
it off for 1s then switches on again and so on until the agent receives next
`turn_on_off` signal. A statechart for that agent can be represented as (note
that this statechart represent yet more complex case which will be
described later):

![](https://svn.code.sf.net/p/sobjectizer/repo/branches/so_5/5.5.15--dev-trunk/dev/sample/so_5/blinking_led/statechart.png?p=2176)

Agent blinking_led wll have two top level states: `off` and `blinking`. State
`blinking` is a composite state with two substates: `blink_on` and `blink_off`.
In C++ code this can be expressed this way:

~~~~~{.cpp}
class blinking_led final : public so_5::agent_t
{
   state_t
       off{ this, "off" },
       blinking{ this, "blinking" },
           blink_on{ initial_substate_of{ blinking }, "on" },
           blink_off{ substate_of{ blinking }, "off" };
~~~~~

Substate `blink_on` is marked as initial substate of composite state blinking.
It means that when state `blinking` is activated then substate `blink_on` is
activated too.

Moreover `so_current_state()` method will return a reference to `blink_on`, not
to `blinking`. It is because `so_current_state()` always returns a reference to
a leaf state in agent's state tree.

Every composite state must have the initial substate.

It means that exactly one substate must be declared by using
`initial_substate_of` indicator:

~~~~~{.cpp}
blink_on{ initial_substate_of{ blinking }, "on" },
blink_off{ substate_of{ blinking }, "off" };
~~~~~

An attempt to activate a composite state without the initial substate defined
will lead to an error at run-time.

Definition of behaviour for composite states and substates is done usual way:

~~~~~{.cpp}
off
   .event< turn_on_off >( [this]{ this >>= blinking; } );

blinking
   .on_enter( [this] {
         m_timer = so_5::send_periodic< timer >(
               *this, std::chrono::seconds::zero(), std::chrono::seconds{1} );
      } )
   .on_exit( [this]{ m_timer.release(); } )
   .event< turn_on_off >( [this]{ this >>= off; } );

blink_on
   .on_enter( &blinking_led::led_on )
   .on_exit( &blinking_led::led_off )
   .event< timer >( [this]{ this >>= blink_off; } );

blink_off
   .event< timer >( [this]{ this >>= blink_on; } );
~~~~~

It's easy to see that many events look like:

~~~~~{.cpp}
event<Message>([this] { this >>= State; });
~~~~~

This is very typical case in complex statecharts.

There is a special method `just_switch_to()` which can simplify such cases. By
using this method we can rewrite states behaviour that way:

~~~~~{.cpp}
off
   .just_switch_to< turn_on_off >( blinking );

blinking
   .on_enter( [this] {
         m_timer = so_5::send_periodic< timer >(
               *this, std::chrono::seconds::zero(), std::chrono::seconds{1} );
      } )
   .on_exit( [this]{ m_timer.release(); } )
   .just_switch_to< turn_on_off >( off );

blink_on
   .on_enter( &blinking_led::led_on )
   .on_exit( &blinking_led::led_off )
   .just_switch_to< timer >( blink_off );

blink_off
   .just_switch_to< timer >( blink_on );
~~~~~

Note that reaction to `turn_on_off` signal is defined only in `off` and
`blinking` states. There are no such handles in substates `blink_on` and
`blink_off`. It is not necessary because substates inherit event handlers from
their parent states. Inheritance of event handlers means that event handler
will be searched in the current state, then in its parent state, then in its
parent state and so on...

In blinking_led agent an event handler for `turn_on_off` signal will be
searched in `blink_on` and `blink_off` states and then in `blinking` state.
That is why we don't need to create subscription for `turn_on_off` in
`blink_on` and `blink_off` states.

Now we can write the full code of blinking_led agent:

~~~~~{.cpp}
class blinking_led final : public so_5::agent_t
{
   state_t
      off{ this, "off" },
      blinking{ this, "blinking" },
         blink_on{ initial_substate_of{ blinking }, "on" },
         blink_off{ substate_of{ blinking }, "off" };

   struct timer : public so_5::signal_t {};

public :
   struct turn_on_off : public so_5::signal_t {};
   
   blinking_led( context_t ctx ) : so_5::agent_t{ ctx }
   {
      this >>= off;

      off
         .just_switch_to< turn_on_off >( blinking );

      blinking
         .on_enter( [this] {
               m_timer = so_5::send_periodic< timer >(
                     *this, std::chrono::seconds::zero(), std::chrono::seconds{1} );
            } )
         .on_exit( [this]{ m_timer.release(); } )
         .just_switch_to< turn_on_off >( off );
         
      blink_on
         .on_enter( &blinking_led::led_on )
         .on_exit( &blinking_led::led_off )
         .just_switch_to< timer >( blink_off );

      blink_off
         .just_switch_to< timer >( blink_on );
   }

private :
   so_5::timer_id_t m_timer;

   void led_on() { ... } // some device-dependent code
   void led_off() { ... } // some device-dependent code
};
~~~~~

But what if we have to change the behaviour of our blinking_led agent? Let's
imagine that blinking_led agent have to switch LED on for 1.5s and then switch
it off for 0.7s. How can we do that?

The obvious way is to use delayed signals with different timeouts in enter
handlers for `blink_on` and `blink_off` states. But this way is not very easy
in fact...

Usage of `time_limit` feature is much simpler. The time_limit feature dictates
SObjectizer to limit time spent in the specific state. A clause like:

~~~~~{.cpp}
SomeState.time_limit(Timeout, AnotherState);
~~~~~

Tells the SObjectizer that agent must be automatically switched from
`SomeState` to `AnotherState` after `Timeout` spent in `SomeState`.

With `time_limit` feature the bllinking_led agent's implementation looks
simpler and shorter:

~~~~~{.cpp}
class blinking_led final : public so_5::agent_t
{
   state_t
      off{ this, "off" },
      blinking{ this, "blinking" },
         blink_on{ initial_substate_of{ blinking }, "on" },
         blink_off{ substate_of{ blinking }, "off" };

public :
   struct turn_on_off : public so_5::signal_t {};
   
   blinking_led( context_t ctx ) : so_5::agent_t{ ctx }
   {
      this >>= off;

      off
        .just_switch_to< turn_on_off >( blinking );

      blinking
        .just_switch_to< turn_on_off >( off );

      blink_on
         .on_enter( &blinking_led::led_on )
         .on_exit( &blinking_led::led_off )
         .time_limit( std::chrono::milliseconds{1500}, blink_off );

       blink_off
         .time_limit( std::chrono::milliseconds{750}, blink_on );
   }

private :
   void led_on() { ... } // some device-dependent code
   void led_off() { ... } // some device-dependent code
};
~~~~~

# More Details

## Do Not Change Agent State From Enter/Exit Handlers

**An enter/exit handler should not change state of agent.** 

## Class state_t Is Not Thread Safe

Class `so_5::state_t` is not thread safe. It is designed to be used inside an
owner agent only. For example:

~~~~~{.cpp}
class my_agent : public so_5::agent_t
{
  state_t first_state{ this, "first" };
  state_t second_state{ this, "second" };
...
public :
  my_agent( context_t ctx ) : so_5::agent_t{ ctx }
  {
    // It is a safe usage of state.
    first_state.on_enter( &my_agent::first_on_enter );
    second_state.on_exit( &my_agent::second_on_exit );
    ...
  }
  virtual void so_define_agent() override
  {
    // It is a safe usage of state.
    first_state.event( &my_agent::some_event_handler );
    second_state.time_limit( std::chrono::seconds{20}, first_state );
    second_state.event( [this]( const some_message & msg ) {
        // It is also safe usage of state because event handler
        // will be called on the context of agent's working thread.
        second_state.drop_time_limit();
        ...
      } );
  }
  void some_public_method()
  {
    // It is a safe usage if this method is called by the agent itself.
    // This will be unsafe usage if this method is called from outside of
    // the agent: a data damage or something like that can happen.
    second_state.time_limit( std::chrono::seconds{30}, first_state );
  }
...
};
~~~~~

Because of that it is necessary to be very careful during manipulation of
agent's states outside of agent's event handlers. 

## Limitation For Deep Of Substates

There is a limitation for deep of substates in SObjectizer v.5.5.15: there can
be at most 16 nested states.

## Enter/Exit Handlers And noexcept

All enter/exit handlers must be `noexcept` functions. If an enter/exit handler
throws an exception the whole application will be aborted. It is because there
is no way to revert changes of agent's state change procedure if an enter/exit
throws an exception.

## Guarantee For Calling Of Exit Handler

If agent `A` is successfully registered and switched to state `S` then
SObjectizer is guaranteed the call of exit handler for state `S` in the
following cases:

* switching to another state;
* an invocation of `so_evt_finish()` as result of: deregistration of agent's coop or shutdown of SObjectizer Environment. An exit handler for `S` will be called after return from `so_evt_finish()`.

It means that once agent is registered the SObjectizer guaratees the call of
exit handler. But exit handler will not be called if agent is not registered.
For example:

~~~~~{.cpp}
class some_agent : public so_5::agent_t
{
    state_t S{ this };
    ...
public :
    some_agent( context_t ctx ) : so_5::agent_t{ ctx }
    {
        S.on_exit(...); // some handler
        this >>= S;
    }
    ...
};
...
env.introduce_coop( []( so_5::coop_t & coop ) {
    coop.make_agent< some_agent >();
    ... // Some other actions.
    throw std::runtime_error( "Just a demo!" );
} );
~~~~~

In this case an agent of type `some_agent` will be created, the initial state
for that agent will be `S`, but exit handler for `S` will not be called because
the failure of coop registration.

What does it mean?

It means that if you need to do some important task in exit handler then
consider to switch agent to that state only after successful registration of
your agent. For example: in `so_evt_start()` method:

~~~~~{.cpp}
class some_agent : public so_5::agent_t
{
    state_t S{ this };
    ...
public :
    some_agent( context_t ctx ) : so_5::agent_t{ ctx }
    {
        S.on_exit(...); // some important code in the handler.
    }
    ...
    virtual void so_evt_start() override
    {
        this >>= S;
    }
};
~~~~~

## Shallow- And Deep-history

SObjectizer supports states with shallow- and deep-histories. To define a state
with history it is necessary to use a constructor of `state_t` with argument of
type `state_t::history_t`:

~~~~~{.cpp}
class demo : public so_5::agent_t
{
    // An anonymous state with shallow history.
    state_t state1{ this, shallow_history };
    // A state with name and shallow history.
    state_t state2{ this, "two", shallow_history };
    
    // An anonymous state with deep history.
    state_t state3{ this, deep_history };
    // A state with name and deep history.
    state_t state4{ this, "four", deep_history };
    
    // An initial substate with name and deep history.
    state_t state5{ initial_substate_of{ state1 }, "five", deep_history };
    // An anonymous substate with shallow history.
    state_t state6{ substate_of{ state1 }, shallow_history };
    ...
};
~~~~~

There is `state_t::clear_history()` method. But `S.clear_history()` clears
history only for state `S`. History of any substate of `S` remains intact:

~~~~~{.cpp}
class demo : public so_5::agent_t
{
  state_t A{ this, "A", deep_history };
  state_t B{ initial_substate_of{ A }, "B", shallow_history };
  state_t C{ initial_substate_of{ B }, "C" };
  state_t D{ substate_of{ B }, "D" };
  state_t E{ this, "E" };
  ...
  void some_event()
  {
    this >>= A; // The current state is "A.B.C"
                // Because B is initial substate of A, and
                // C is initial substate of B.
    this >>= D; // The current state is "A.B.D".
    this >>= E; // The current state is "E".
    this >>= A; // The current state is "A.B.D" because deep history of A.
    this >>= E;
    A.clear_history();
    this >>= A; // The current state is "A.B.D" because:
                // B is the initial substate of A and B has shallow history;
                // D is the last active substate of B.
  }
};
~~~~~

## The transfer_to_state Feature

Method `state_t::transfer_to_state()` is intended for defering an event to
another state. For example:

~~~~~{.cpp}
class device : public so_5::agent_t
{
    state_t off{ this };
    state_t on{ this };
public :
    struct volume_up : public so_5::signal_t {};
    struct volume_down : public so_5::signal_t {};
    ...
    virtual void so_define_agent() override
    {
        off.transfer_to_state< volume_up >( on )
            .transfer_to_state< volume_down >( on );
        on.event< volume_up >(...) // some reaction
            .event< volume_down >(...); // some reaction
    }
};
~~~~~

When agent device in state `off` and receives `volume_up` signal it
automatically switches to state `on` and handler for `volume_up` is searched in
state `on`.

Note that `transfer_to_state` can change agent state several times:

~~~~~{.cpp}
class demo : public so_5::agent_t
{
    state_t A{ this };
    state_t B{ this };
    state_t C{ this };
public :
    struct do_something : public so_5::signal_t {};
    ...
    virtual void so_define_agent() override
    {
        A.transfer_to_state< do_something >( B );
        B.transfer_to_state< do_something >( C );
        C.event< do_something >( ... );
        ...
    }
};
~~~~~

If agent demo is in state `A` and receives `do_something` signal it switches
from `A` to `B` and then to `C`.

The `transfer_to_state` is a powerful but dangerous feature. There is no a
limitation for the deep of transfers of event from one state to another.
Because of that a user can create an infinite loop of transfering event between
states.

## The suppress Feature

Sometimes it is necessary to disable an event handler defined in parent state.
It can be done by `state_t::suppress` method:

~~~~~{.cpp}
class device : public so_5::agent_t
{
    state_t off{ this },
        on{ this },
        number_selection{ initial_substate_of{ on } },
        dialling{ substate_of{ on } };
    ...
public :
    ...
    virtual void so_define_agent() override
    {
        on.event< key_cancel >(...) // some reaction
            .event< key_grid >(...) // some reaction
            .event< key_digit >(...) // some reaction
            ...;
        dialling
            // Only key_cancel can be processed in dialling state.
            // Other events must be suppressed.
            .suppress< key_digit >()
            .suppress< key_grid >()
            .event< key_cancel >(...); // some reaction
    }
};
~~~~~

Without suppressing of `key_digit` and `key_grid` events in `dialling` state
handlers for them will be automatically found in `on` state (because of
inheritance of event handlers).

## Redefenition Of Enter/Exit Handler

Redefenition of enter or exit handler can be necessary when class inheritance
is used. For example:

~~~~~{.cpp}
class basic_device : public so_5::agent_t
{
protected :
    state_t activated{ this };
    ...
public :
    ...
    virtual void so_define_agent() override
    {
        activated.on_enter(
                ... // some complex code here
            )
            .event(...)
            ...;
    }
};
...
class specific_device : public basic_device
{
public :
    ...
    virtual void so_define_agent() override
    {
        basic_device::so_define_agent();
        ...
        // Addition of some specific action to activated.on_enter handler.
        auto old_handler = activated.on_enter();
        activated.on_enter( [old_handler] {
            old_handler(); // Actions from base class.
            ... // some additional actions.
        );
    }
};
~~~~~

## Reseting Of time_limit

The `state_t::time_limit()` method can be called several times in different
places. Usually it is called only once somewhere in constructor or
`so_define_agent()`, for example:

~~~~~{.cpp}
blinking_led::blinking_led( context_t ctx ) : so_5::agent_t{ ctx }
{
    blink_on.time_limit( std::chrono::milliseconds{1500}, blink_off )
        ...;
    blink_off.time_limit( std::chrono::milliseconds{750}, blink_on )
        ...;
   ...
}
~~~~~

But sometimes it is necessary to call `time_limit` in different places. It is
possible and allowed. But there is a speciality: if `S.time_limit` is called
when agent is already in `S` state then time will ticks on from the beginning.
For example:

~~~~~{.cpp}
class inactivity_watcher final : public so_5::agent_t
{
    state_t off{ this }, active{ this };
public :
    inactivity_watcher( context_t ctx, const so_5::mbox_t & input )
        : so_5::agent_t{ ctx }
    {
        off.transfer_to_state< key_cancel >( input, active )
            .transfer_to_state< key_grid >( input, active )
            .transfer_to_state< key_digit >( input, active );
            
        // Switching to off state after 30 seconds of inactivity.
        active.time_limit( std::chrono::seconds{30}, off )
            .event< key_cancel >( [this]{ update_timeout(); } )
            .event< key_grid >( [this]{ update_timeout(); } )
            .event( [this]( const key_digit & ){ update_timeout(); } );
    }
    
public :
    void update_timeout()
    {
        active.time_limit( std::chrono::seconds{30}, off );
    }
};
~~~~~

## Lifetime Of state_t Objects

Class `state_t` was developed to be used as type of members of some agent class:

~~~~~{.cpp}
class basic_device : public so_5::agent_t
{
    state_t deactivated{...};
    state_t activated{...};
    ...
};
class specific_device : public basic_device
{
    state_t paused{...};
    state_t stopped{...};
    ...
};
~~~~~

It means that lifetime of `state_t` objects will be automatically synchronized
with lifetime of their owners.

This synchronization of lifetimes significantly simplifies implementation of
various aspects of agents in SObjectizer. Because of that the recommended way
of agent's state creation is declaration of `state_t` object as member of
corresponding agent's class.

But it is possible to create and destroy `state_t` objects manually. It is hard
to imagine a situation were it can be necessary but it is possible to write
something like that:

~~~~~{.cpp}
class some_crazy_agent : public so_5::agent_t
{
    std::vector< std::unique_ptr< state_t > > m_additional_states;
    ...
    void some_event()
    {
        // A bunch of states must be created.
        for( int i = 0; i < 20; ++i )
        {
            auto s = std::make_unique< state_t >( this );
            so_subscribe_self().in( *s ).event< some_signal >( ... );
            m_additional_state.push_back( std::move(s) );
        }
    }
    ...
    void some_another_event()
    {
        // Additional states no more needed.
        for( auto & s : m_additional_states )
        {
            so_drop_subscription< some_signal >(
                so_direct_mbox(),
                *s );
             s.reset();
        }
        m_additional_states.clear();
    }
};
~~~~~

But this approach is very dangerous because raw pointers to state objects are
stored in different places (in agent as a pointer to the current state, in
subscription storage, in history of parent state(s) and so on). If a
pointer to state which must be destroyed manually is not removed from these
places a dangling pointer will be created with corresponding consequences.

