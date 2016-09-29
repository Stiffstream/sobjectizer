# so-5.5.18: Work thread activity tracking {#so_5_5_18__work_thread_activity_tracking}

Amount of monitoring information from dispatchers has been expanded in
v.5.5.18. It is now possible to receive information about time spent inside
event-handlers and time spent during waiting for next event.

Collecting of such information must be turned on explicitely. This information
is not collected by default to avoid negative impact on dispatchers'
performance. Work thread activity tracking can be turned on for an individual
dispatcher:

~~~~~{.cpp}
using namespace so_5::disp::active_obj;
auto disp = create_private_disp( env, "my_disp",
      // Collecting of work thread activity is turned on
      // in the dispatcher's parameters.
      disp_params_t{}.turn_work_thread_activity_tracking_on() ); 
~~~~~

It can be also done for the whole SObjectizer Environment:

~~~~~{.cpp}
so_5::launch(
   []( so_5::environment_t & env ) {
      ... // some intitial actions.
   }
   []( so_5::environment_params_t & params ) {
      // Turn work thread activity statistics collection on explicitely.
      params.turn_work_thread_activity_tracking_on();
      ...
   } ); 
~~~~~

In that case collecting of information of work thread activity will be turned
on for all dispatchers inside SObjectizer Environment. If it is not appropriate
for some dispatcher then thread activity tracking can be turned off in
parameters of that dispatcher:

~~~~~{.cpp}
so_5::launch(
   []( so_5::environment_t & env ) {
      ... // Some initial actions.

      // Create dispatcher and turn off work thread activity tracking for it.
      auto my_disp = so_5::disp::one_thread::create_private_disp(
            env, "my_disp",
            so_5::disp::one_thread::disp_params_t{}
                  .turn_work_thread_activity_tracking_off() );
      ...
   []( so_5::environment_params_t & params ) {
      // Work thread activity tracking will be turned on for the whole environment. 
      params.turn_work_thread_activity_tracking_on();
      ...
   } );
~~~~~

Distribution of information about work thread activity is done the usual way --
by messages which are sent to message box of stats_collector. These messages
have type `work_thread_activity` which is defined in `so_5::stats::messages`
namespace.

~~~~~{.cpp}
class activity_listener final : public so_5::agent_t
   {
   public :
      activity_listener( context_t ctx )
         :  so_5::agent_t( ctx )
         {
            so_default_state().event(
                  so_environment().stats_controller().mbox(),
                  &activity_listener::evt_monitor_activity );
         }

   private :
      void
      evt_monitor_activity(
         const so_5::stats::messages::work_thread_activity & evt )
         {
            namespace stats = so_5::stats;

            std::cout << evt.m_prefix << evt.m_suffix
                  << " [" << evt.m_thread_id << "] ->\n"
                  << "  working: " << evt.m_stats.m_working_stats << "\n"
                  << "  waiting: " << evt.m_stats.m_waiting_stats << std::endl;
            ...
         }
   }; 
~~~~~

There ara well known fields `m_prefix` and `m_suffix` inside
`work_thread_activity`. Also there are additional fields:

* `m_thread_id` with ID of working thread (activity stats information is
  related to thread with this ID);
* `m_working_stats` with info about time spent in event-handlers;
* `m_waiting_stats` with info about time spent on waiting for new events.

Fields `m_working_stats` and `m_waiting_stats` have type
`so_5::stats::activity_stats_t`:

~~~~~{.cpp}
struct activity_stats_t
   {
      // Count of events in that period of time.
      std::uint_fast64_t m_count{};

      // Total time spent for events in that period of time.
      duration_t m_total_time{};

      // Average time for one event.
      duration_t m_avg_time{};
   }; 
~~~~~

Where `duration_t` is `std::chrono::high_resolution_clock::duration` or
`std::chrono::steady_lock::duration`.

Message `work_thread_activity` allows to know how many events have been handled
by a work thread (from the very beginning of its work), how much time have been
spent inside event-handlers (from the very beginning of its work), average time
of event-handler's work. Similary for waiting of new events (e.g. count of
waitings, total time spent in waiting, average waiting time).

This is a small example of how that information can looks like:

~~~~~
disp/atp/0x2b7fb70/threads.count: 4
disp/atp/0x2b7fb70/agent.count: 5
disp/atp/0x2b7fb70/wt-14/thread.activity [14] ->
  working: [count=5651;total=3.86549ms;avg=0.000655ms]
  waiting: [count=5652;total=492.831ms;avg=3.60857ms]
disp/atp/0x2b7fb70/wt-15/thread.activity [15] ->
  working: [count=62394;total=40.4094ms;avg=0.000644ms]
  waiting: [count=62395;total=363.346ms;avg=0.201897ms]
disp/atp/0x2b7fb70/wt-16/thread.activity [16] ->
  working: [count=69073;total=46.2095ms;avg=0.000637ms]
  waiting: [count=69073;total=361.668ms;avg=0.000813ms]
disp/atp/0x2b7fb70/wt-17/thread.activity [17] ->
  working: [count=80587;total=52.904ms;avg=0.000656ms]
  waiting: [count=80588;total=325.136ms;avg=0.003536ms]
disp/atp/0x2b7fb70/cq/__so5_au...e_2__/agent.count: 5
disp/atp/0x2b7fb70/cq/__so5_au...e_2__/demands.count: 4
~~~~~

This is a dump of monitoring info for adv_thread_pool-dispatcher with four work
threads. A single coop with five agents inside works on that dispatcher. It is
possible to see info for every work thread (there is thread ID in square
brackets). For this particular case it is possible to see that work threads
spend more time during waiting for new events that for event-handling.

Some words about performance impact. It highly depends of dispatcher's type and
load profile. For one_thread-dispatcher and heavy message-stream this impact is
negligible. But for active_obj-dispatcher with occasional messages performance
can degraded for 3-4 times. Becase of that it impossible to provide some
numbers of cummulative performance loss: there is too big variety in results.

In any cases this mechanism is regarded as auxilary. It indended to debugging
and profiling. Because of that it is necessary to check performance impact of
work thread activity tracking on your application. And only then the decision
about turning that tracking on in production evironment must be taken.

