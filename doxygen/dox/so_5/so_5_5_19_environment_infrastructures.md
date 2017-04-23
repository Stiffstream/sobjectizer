# so-5.5.19 Environment Infrastructures {#so_5_5_19__environment_infrastructures}

# Introduction
Every instance of SObjectizer's environment needs some infrastructure for handling several important tasks like:

* completion of cooperation deregistration procedure;
* dealing with timers;
* dispatching events for agents which are bound to the default dispatcher.

Until v.5.5.19 there was only one type of SObjectizer's environment infrastructure: the multithreaded one. It means that SObjectizer environment creates at least three work thread: one for the default dispatcher, one for timer (it called timer thread) and yet another for completion of deregister operations.

These three theads are not a big overhead for complex multithreaded applications where dozens of work threads are used. But there are cases where:

* usage of SObjectizer is valuable, but
* it is better to use just one work thread for the whole application

For example: a small application which works on a tiny computer like Orange PI and connects to MQTT broker, publishes some messages into several topics and then waits for some messages from other topics. For such cases creation of three additional threads for the needs of SObjectizer's environment infrastructure is just an overkill. It is better to have single-threaded SObjectizer environment infrastructures.

Since v.5.5.19 there is a possibilty to specify which kind of environment infrastructure will be used by SObjectizer environment. And there are three implementations of infrastructure:

* default multi threaded infrastructure. It is old one which was the single infrastructure in older SObjectizer's versions;
* thread-safe single threaded infrastructure. It does all SObjectizer-specific actions on single thread. But allows to interact with SObjectizer environment from different threads inside an application;
* not thread-safe single threaded infrastructure. It assumes that there is only one thread in the whole application and does all SObjectizer-related activities on it.

# How to Specify Environment Infrastructure
A classical multi threaded infrastructure is used by default. If it is necessary to use another type of environment infrastructure the required infrastructure must be set via `so_5::environment_params_t` before start of SObjectizer environment. For example, when SObjectizer is started by `so_5::launch`:
~~~~~{.cpp}
so_5::launch([](so_5::environment_t & env) {
        ... // Starting actions.
    },
    [](so_5::environment_params_t & params) {
        // Change environment infrastructure.
        params.infrastructure_factory(
            // Simple thread-safe single threaded infrastructure will be used.
            so_5::env_infrastructures::simple_mtsafe::factory());
    });
~~~~~
Or, in the case of `so_5::wrapped_env_t`:
~~~~~{.cpp}
so_5::wrapped_env_t sobj([](so_5::environment_t & env) {
        ... // Starting actions.
    },
    [](so_5::environment_params_t & params) {
        // Change environment infrastructure.
        params.infrastructure_factory(
            // Simple thread-safe single threaded infrastructure will be used.
            so_5::env_infrastructures::simple_mtsafe::factory());
    });
~~~~~
Please note that environment infrastructure can't be changed after start of SObjectizer environment.
# More About Standard Environment Infrastructures
## default_mt
It is the classical multi threaded environment infrastructure which is used by default. If there is a need to set it up manually then `factory()` function from `so_5::env_infrastructures::default_mt` namespace must be used. For example:
~~~~~{.cpp}
so_5::environment_params_t make_env_params(const app_config & cfg) {
    so_5::environment_params_t result;
    // Single threaded infrastructure is necessary for most cases.
    result.infrastructure_factory(
        so_5::env_infrastructures::simple_mtsafe::factory());
    ... // Some other tuning.
    if(cfg.need_more_threads())
        // In that case default multi threaded environment must be used.
        result.infrastructre_factory(
            so_5::env_infrastructures::default_mt::factory());
    ...
    return result;
}
~~~~~
Please note that in the case of `default_mt` environment infrastructure all values from `environment_params_t` are used for tuning environment behavior. When single threaded infrastructures are used then some of values from `environment_params_t` can be ignored or superseded by params from an infrastructure factory.
## simple_mtsafe
Simple thread safe single threaded environment infrastructure is intended to be used in multi-threaded application where it is necessary to reduce overhead from SObjectizer-related things. For example it can be a simple GUI application in which a single separate thread will be dedicated to all agents and SObjectizer's activities.

This type of environment infrastructure provides full thread safety for SObjectizer's environment. It means that new cooperations can be created or some messages can be sent from other application threads. Something like:
~~~~~{.cpp}
int main() {
    so_5::mbox_t mbox; // Mbox of the main agent will be stored here.
    // Launch SObjectizer on the context of new dedicated thread.
    // This thread is created and destroyed by wrapped_env_t itself.
    so_5::wrapped_env_t sobj([&](so_5::environment_t & env) {
            // Create a coop with main agent inside.
            env.introduce_coop([&](so_5::coop_t & coop) {
                mbox = coop.make_agent<main_agent>(...)->so_direct_mbox();
                ...
            });
        },
        [](so_5::environment_params_t & params) {
            // All SObjectizer-related activities will be performed
            // on the context of the single work thread.
            params.infrastructure_factory(
                so_5::env_infrastructures::simple_mtsafe::factory());
        });
    ...
    // Now we can send messages to the main agent.
    so_5::send<some_message>(mbox, ...);
    ...
    // We can also create new coops.
    sobj.environment().introduce_coop("new_coop", [](so_5::coop_t & coop) {
            coop.make_agent<some_new_agent>(...);
            ...
        });
    ...
    // We can also deregister existing coops.
    sobj.environment().deregister_coop("new_coop", so_5::dereg_reason::normal);
    ...
}
~~~~~
Thread safety of this infrastructure type allows even to launch additional dispatchers (public or private). It is normal because this infrastructure type just reduced SObjectizer-related overhead, but not reduced SObjectizer features. It means that default dispatcher, timers and final deregistration steps will be performed on the context of a single work thread, but additional work threads from additional dispatchers can also be used by application.
### Selecting of Timer Implementation
Because `simple_mtsafe` environment infrastructure doesn't use timer thread then timer thread settings in `environment_params_t` will just ignored. It is possible to set a timer implementation to be used via parameters to `factory()` function. For example:
~~~~~{.cpp}
so_5::launch([](so_5::environment_t & env) {...},
    [](so_5::environment_params_t & params) {
        namespace st_env = so_5::env_infrastructures::simple_mtsafe;
        params.infrastructure_factory(
            st_env::factory(
                // Timer_list implementation will be used.
                st_env::params_t{}.timer_manager(so_5::timer_list_manager_factory())));
    });
~~~~~
### Long-running Init Actions
Init actions for SObjectizer's environment should not be long-running. It is because all SObjectizer-related activities (like timers handling) will be started only after exit from init function. Do not do long-lasting actions like interaction with user in init function for `simple_mtsafe` environment. It will block SObjectizer activities.
## simple_not_mtsafe
Simple not thread safe single threaded environment infrastructure is intended to be used in single threaded applications where the main application thread must be used for everything. For example in a small network unitilities like custom version of ping or traceroute programs.

Usage of `simple_not_mtsafe` infrastructure has several limitations and specific features:

There must not be any attempts to use SObjectizer environment outside of work thread where SObjectizer has been launched. It is because there is no thread safety and attempt to create a new coop or to send a new message from outside can lead to data corruption and other errors.

SObjectizer environment automatically finishes its work when there is no any waiting events or timers. It means that SObjectizer environment will be automatically shut down is the following scenario:
~~~~~{.cpp}
int main() {
    so_5::launch([](so_5::environment_t & env) {
            env.intruduce_coop([](so_5::coop_t & coop) {
                coop.define_agent().on_start([]{
                        std::cout << "Hello, World!" << std::endl;
                    });
            });
        },
        [](so_5::environment_params_t & params) {
            params.infrastructure_factory(
                so_5::env_infrastructures::simple_not_mtsafe::factory());
        });
    // SObjectizer will finish its work just after printing "Hello, World!"
    // to the standard output stream.
}
~~~~~
It is because there is no a way to initiate some activity from outside of SObjectizer environment. 

Because this infrastructure is not thread safe there is no sense to use `so_5::wrapped_env_t` or to create additional dispatchers when `simple_not_mtsafe` is used.
### Selecting of Timer Implementation
Because `simple_not_mtsafe` environment infrastructure doesn't use timer thread then timer thread settings in `environment_params_t` will just ignored. It is possible to set a timer implementation to be used via parameters to `factory()` function. For example:
~~~~~{.cpp}
so_5::launch([](so_5::environment_t & env) {...},
    [](so_5::environment_params_t & params) {
        namespace st_env = so_5::env_infrastructures::simple_not_mtsafe;
        params.infrastructure_factory(
            st_env::factory(
                // Timer_list implementation will be used.
                st_env::params_t{}.timer_manager(so_5::timer_list_manager_factory())));
    });
~~~~~
### Long-running Init Actions
Init actions for SObjectizer's environment should not be long-running. It is because all SObjectizer-related activities (like timers handling) will be started only after exit from init function. Do not do long-lasting actions like interaction with user in init function for `simple_not_mtsafe` environment. It will block SObjectizer activities.

