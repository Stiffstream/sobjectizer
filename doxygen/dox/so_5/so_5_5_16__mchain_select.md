# so-5.5.16: New select functions for receive messages from several mchains {#so_5_5_16__mchain_select}

Since v.5.5.16 there are two new `so_5::select` functions which allow to receive and handle messages from several mchains.

The first variant of `select` takes a timeout and a list of mchains and message handlers for them:

~~~~~{.cpp}
so_5::mchain_t ch1 = env.create_mchain(...);
so_5::mchain_t ch2 = env.create_mchain(...);
so_5::mchain_t ch3 = env.create_mchain(...);
...
so_5::select(std::chrono::milliseconds(500),
    case_(ch1, [](const reply1 & r) {...}),
    case_(ch2, [](const reply2 & r) {...}),
	case_(ch2, [](const reply3 & r) {...}, [](const reply4 & r) {...}));
~~~~~

It checks the mchains and wait for no more than 500ms if all mchains are empty. If some mchain is not empty it extracts just one message from that mchain and tries to find an appropriate handler for it. If handler is found it is called. If a handler is not found then the message extracted will be thrown out without any processing.

If all mchains are empty even after the specified timeout then `select` will do nothing.

Simple form of `select` extract just one message from a mchain. Which non-empty mchain will be used for message extraction is not determined.

Please note that if there is just one non-empty mchain then simple `select` will return after extraction on just one message. This message could not be processed if there is no appropriate handler for it.

As with `receive` there are two special values which can be used as timeout:

* value `so_5::no_wait` specifies zero timeout. It means that receive will not wait if the mchain is empty;
* value `so_5::infinite_wait` specifies unlimited waiting. The return from receive will be on arrival of any message. Or if the mchain is closed explicitly.

There is also more advanced version of `select` which can receive and handle more than one message from mchains. It receives a `so_5::mchain_select_params_t` objects with list of conditions and returns control if any of those conditions becomes true. For example:

~~~~~{.cpp}
using namespace so_5;
mchain_t ch1 = env.create_mchain(...);
mchain_t ch2 = env.create_mchain(...);
// Receive and handle 3 messages.
// It could be 3 messages from ch1. Or 2 messages from ch1 and 1 message
// from ch2. Or 1 message from ch1 and 2 messages from ch2. And so on...
//
// If there is no 3 messages in mchains the select will wait infinitely.
// A return from select will be after handling of 3 messages or
// if all mchains are closed explicitely.
select( from_all().handle_n( 3 ),
  case_( ch1,
      []( const first_message_type & msg ) { ... },
      []( const second_message_type & msg ) { ... } ),
  case_( ch2,
      []( const third_message_type & msg ) { ... },
      handler< some_signal_type >( []{ ... ] ),
      ... ) );
// Receive and handle 3 messages.
// If there is no 3 messages in chains the select will wait
// no more that 200ms.
// A return from select will be after handling of 3 messages or
// if all mchains are closed explicitely, or if there is no messages
// for more than 200ms.
select( from_all().handle_n( 3 ).empty_timeout( milliseconds(200) ),
  case_( ch1,
      []( const first_message_type & msg ) { ... },
      []( const second_message_type & msg ) { ... } ),
  case_( ch2,
      []( const third_message_type & msg ) { ... },
      handler< some_signal_type >( []{ ... ] ),
      ... ) );
// Receive all messages from mchains.
// If there is no message in any of mchains then wait no more than 500ms.
// A return from select will be after explicit close of all mchains
// or if there is no messages for more than 500ms.
select( from_all().empty_timeout( milliseconds(500) ),
  case_( ch1,
      []( const first_message_type & msg ) { ... },
      []( const second_message_type & msg ) { ... } ),
  case_( ch2,
      []( const third_message_type & msg ) { ... },
      handler< some_signal_type >( []{ ... ] ),
      ... ) );
// Receve any number of messages from mchains but do waiting and
// handling for no more than 2s.
select( from_all().total_time( seconds(2) ),
  case_( ch1,
      []( const first_message_type & msg ) { ... },
      []( const second_message_type & msg ) { ... } ),
  case_( ch2,
      []( const third_message_type & msg ) { ... },
      handler< some_signal_type >( []{ ... ] ),
      ... ) );
// Receve 1000 messages from chains but do waiting and
// handling for no more than 2s.
select( from_all().extract_n( 1000 ).total_time( seconds(2) ),
  case_( ch1,
      []( const first_message_type & msg ) { ... },
      []( const second_message_type & msg ) { ... } ),
  case_( ch2,
      []( const third_message_type & msg ) { ... },
      handler< some_signal_type >( []{ ... ] ),
      ... ) );
~~~~~

Both `select` versions return an object of `so_5::mchain_receive_result_t` type. Methods of that object allow to get number of extracted and handled messages, and also the status of `select` operation.

It is possible to use a mchain in several `select` on different threads at the same time.

**NOTE!** A mchain can be used inside just one `case_` statement in `select`. If the same mchain is used in several `case_` statements then the behaviour of `select` is undefined. Please note also that `select` do not checks uniqueness of mchains in `case_` statements (for performance reasons).

