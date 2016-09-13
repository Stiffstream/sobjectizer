# so-5.5.18: Global lock_factories {#so_5_5_18__global_lock_factories}

Message queues in SObjectizer dispatchers use synchronization objects to
protect the contents of the queue when working with it from several threads.
Synchronization objects are created with the help of special factories
(so-called lock_factory). The developer can point which particular lock_factory
should be used when creating specific dispatcher.

This feature is available since 5.5.10 and 5.5.11 versions. Also since then two
types of lock_factories were implemented:

* `combined_lock_factory` creates combined synchronization objects. Such object
  consists of two parts - spin-lock and simple mutex. The capture of such
  object is performed in a tricky way: first there are several attempts to
  capture spin-lock with call to `std::this_thread::yield` between unsuccessful
  attempts. If spin-lock is not captured then there is an attempt to capture
  mutex. Doing this during prolonged waiting the thread, which tries to capture
  the synchronization object, provides operation system an opportunity to fall
  asleep until the resource is available. Such combined synchronization objects
  show good results in heavy loads when there is an intensive message exchange
  and no one waits long to get some work;
* `simple_lock_factory` creates synchronization objects based on simple mutex.
  That is the thread, which attempts to capture the resource obtained by
  someone, will simply  fall asleep waiting for mutex to be released. When the
  mutex is released the operation system will wake the sleeping thread and push
  it to execution. Such synchronization objects are more expensive but the
  application consumes almost no resources in case message chains are empty
  (nobody is using cores for nothing on spin-locks).

By default `combined_lock_factory` is used. This can be changed in message
queue properties to `simple_lock_factory` if needed.

Before version 5.5.18 it was possible to replace `combined_lock_factory` with
`simple_lock_factory` only for one separate dispatcher. That is when creating
dispatcher in its queue_traits `simple_lock_factory` is selected. Which was
uncomfortable if the application created more than one dispatcher (or even more
than ten): one had to remember to configure the queue_traits for each of them.

Even worse if the application uses ready-made library of agents inside of which
its own dispatcher instances are created. Likely this library doesn't provide
an opportunity to configure queue_traits for its own dispatchers but uses the
default parameters of queue_traits, i.e. `combined_lock_factory` will be used.

In version 5.5.18 this defect was fixed by adding the feature to set the
default lock_factories for the whole SObjectizer Environment. That is if all
application's dispatchers in their queues should use only `simple_lock_factory`
by default this can be configured via `environment_params_t` since version
5.5.18. For example:

~~~~~{.cpp}
so_5::launch( []( so_5::environment_t & env ) {
      ... // Some initial actions.
   },
   []( so_5::environment_params_t & params ) {
      // Use simple_lock_factory for event queues by default.
      params.queue_locks_defaults_manager(
            so_5::make_defaults_manager_for_simple_locks() );
      ...
   } );
~~~~~

After that for dispatcher creation the following scenario is used:

1. If the developer in queue_traits set the specific lock_factory for
	dispatcher then when creating queues of this dispatcher the specific
	lock_factory will be used. Global parameters of SObjectizer Environment are
	ignored.
2. Ohterwise when creating queues of this dispatcher the lock_factory will be
	used from SObjectizer Environment parameters. In the example above it is
	`simple_lock_factory`.  

Herewith the application doesn't care where particular the dispatcher is
created: in the application code or in the third-party library. If the
lock_factory  is not set explicitly when creating dispatcher then the
lock_factory will be used by default.

If the developer didn't set queue_locks_defaults_manager explicitly during SObjectizer Environment start then `combined_lock_factory` will be used by default. So the default behavior of dispatchers and queues in SObjectizer in version 5.5.18 is not changed.
