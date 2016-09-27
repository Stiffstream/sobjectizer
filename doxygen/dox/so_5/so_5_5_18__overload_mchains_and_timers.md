# so-5.5.18: Overloaded mchains and delayed/periodic messages delivery {#so_5_5_18__overloaded_mchains_and_timers}

Since v.5.5.18 there is a new behavior in delivery of delayed and periodic
messages to mchains if these mchains are overloaded:

- if mchains is configured for waiting if is it full then this waiting is
  not preformed for delayed/periodic messages. It is because the sending of
  such messages is performed on context of timer thread. All operations on
  this context must be done as fast as possible. It means impossibility
  to wait for some time on overloaded mchain. It means that the overflow
  reaction for that mchain will be performed immediately;
- the so_5::mchain_props::overflow_reaction_t::throw_exception is
  replaced to so_5::mchain_props::overflow_reaction::drop_newest.
  It means a slient drop of delayed/peridic message without raising any
  exception. It is because the context of timer thread is a special context:
  no exceptions should be thrown here.

