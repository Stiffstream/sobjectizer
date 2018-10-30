# so-5.5.23 Enveloped Messages {#so_5_5_23__enveloped_msg}

[TOC]

A new feature was introduced in v.5.5.23: envelopes with messages inside. This feature allows to send a message or signal that enveloped in a special object called *envelope*. An envelope can carry some additional information and can perform some actions when message/signal is delivered to a receiver.
# An Important Note
This new feature is a low-level feature that intended to be used by SObjectizer's developers and those developers who want to extend SObjectizer functionality. It seems that ordinary users will be protected from envelope-related details by some high-level code (something like a special tools from so_5_extra).

Because of that the sense of reading the following text exists only if you want to write your own type of an envelope.
# What Is An Envelope
An envelope is a special case of a message. An envelope carries a message or signal (or another envelope) inside. An envelope is implemented as class derived from `so_5::enveloped_msg::envelope_t`.
# Delivery Process For An Envelope
Because an envelope is a message the delivery of an envelope is almost the same. For example, dispatchers have no knowledge about envelopes at all -- they see just instances of `so_5::message_t` type.

Key differences in delivery process can be found in two places:

* passing of an envelope to an mbox or mchain for a delivery;
* extraction of envelope's payload for calling an event handler or for transformation of payload (for example in the case of `limit_then_transform`).

## Passing An Envelope To A Mbox

A new method has been added to `so_5::abstract_message_box_t` type: `do_deliver_enveloped_msg`. This method has the same semantic as `do_deliver_message` and `do_deliver_service_request`, but intended to be used with enveloped messages only.

If you want to send an envelope you have to pass your envelope to `do_deliver_enveloped_msg` method. For example:
```{.cpp}
class my_envelope final : public so_5::enveloped_msg::envelope_t {
  const so_5::message_ref_t payload_;
public:
  my_envelope(so_5::message_ref_t payload) : payload_{std::move(payload)} {...}
  ...
};

void make_and_send_envelope(
   // The destination for an envelope.
   const so_5::mbox_t & to,
   // Type of message to be delivered inside an envelope.
   const std::type_index & msg_type,
   // Message to send inside an envelope.
   so_5::message_ref_t what)
{
   to->do_deliver_enveloped_msg(
      msg_type,
      // Envelope instance.
      std::make_unique<my_envelope>(std::move(what)),
      1u);
}
```
## msg_type For An Envelope
New method `do_deliver_enveloped_msg` requires `msg_type` argument. This is a type of envelope's payload, not a type of envelope itself. For example:
```{.cpp}
struct my_message final : public so_5::message_t {...};
class my_envelope final : public so_5::enveloped_msg::envelope_t {...};

// Create a payload of type my_message.
auto payload = std::make_unique<my_message>(...);
// Create an envelope with payload inside.
auto envelope = std::make_unique<my_envelope>(std::move(payload));

// Send envelope with my_message inside.
mbox->do_deliver_enveloped_msg(
   so_5::message_payload_type<my_message>::subscription_type_index(),
   std::move(envelope),
   1u);
```
## Extraction Of Envelope's Payload
There are several points in message delivery process where the payload should be extracted from an envelope:

* delivery of an envelope to a receiver. In this case payload is extracted to be passed to a receiver for normal processing of a message;
* transformation of message. For example, if `limit_then_transform` is used for overload control and count of messages of some type is exceeded the specified limit then the payload of envelope should be extracted and transformed to another message or signal;
* inspection of message content during delivery procedure. For example, a delivery filter need to analyze the payload's content to enable or disable message delivery to a particular receiver.

To extract the payload the corresponding evelope's hook is called.

An envelope can decide should the payload be extracted or not. It is possible that an envelope won't provide access to the payload for some reason. In that case the payload will be ignored. It means that if an envelope is delivered to a receiver but don't provide access to the payload then an event handler for the receiver won't be called (as if message was lost). The same is for transformation case: if an envelope don't provide access to the payload for `limit_then_transform` then message won't be transformed and will be simple thrown out.

This is the main feature of an envelope: the possibility to "hide" the payload from a receiver (or transformation procedure).
# Envelope's Hook
The `access_hook()` is called when SObjectizer need to access the payload of an enveloped message.

`access_hook()` can be called in different cases: when a message is delivered to a receiver, when overload reaction must be performed, when a delivery filter has to analyze the message and so on. The context of `access_hook()` call is identified by `context` argument of `access_hook()`. That argument can have the following values:

* `access_context_t::handler_found`. It means that an enveloped message is delivered to a receiver and tha payload is necessary to call an event handler;
* `access_context_t::transformation`. It means that the payload of an enveloped message should be transformed to another representation. For example in the case of `limit_then_transform` overload reaction;
* `access_context_t::inspection`. It means that the payload of an enveloped message should be analyzed. For example, by a delivery filter.

If the envelope decided that the access to the payload can be provided it should call `invoke()` method for `invoker` object (the `invoker` object is passed as argument to `access_hook()`):
```{.cpp}
class my_envelope final : public so_5::enveloped_msg::envelope_t {
  so_5::message_ref_t payload_;
  ...
public:
  void access_hook(access_context_t context, handler_invoker_t & invoker) const noexcept {
    if(access_can_be_granted(context))
      invoker.invoke(payload_info_t{payload_});
  }
  ...
};
```
Please note that `access_hook()` is marked as `noexcept`. It must not allow exceptions to leave the hook.

## Multiple Invocations Of Hooks
If an enveloped is passed to MPMC-mbox then hook methods can be called more than once. It is because here can be several receivers of the message inside the envelope.
# Envelope And Multithreading
Because envelopes can be sent to MPMC-mboxes there is a possibility that envelope's hooks will be called at the same time on different threads.

It means that envelope classes should be thread safe and should allow invocation of hooks on several threads simultaneously.
# Envelope And Message Limits
Message limits know almost nothing about envelopes. Strictly speaking only `limit_then_transform` knows that envelopes exist and knows how to extract of envelope's payload from an envelope. But no more.

It means that behaviour of an envelope have no influence on message limits.

For example let's consider an envelope that allows to revoke a message. Something like:
```{.cpp}
// A special envelope with atomic revocation flag.
class revocable_envelope_t final : so_5::enveloped_msg::envelope_t {
   friend class revocable_msg_id_t;
   so_5::message_ref_t payload_;
   std::atomic_bool revoked_;
public:
   ...
   void access_hook(access_context_t, handler_invoker_t & invoker) const noexcept {
      if(!revoked_.load(std::memory_order_acquire))
         invoker.invoke(payload_info_t{payload_});
   }
   ...
};

// A special handler that allows to revoke envelope with a message.
class revocable_msg_id_t {
   so_5::intrusive_ptr_t<revocable_envelope_t> envelope_;
public:
   ... // Constructor/destructor and other stuff.
   void revoke() {
      envelope_->revoked_.store(true, std::memory_order_release);
   }
};

// A special helper function to make and send a message inside revocable message.
template<typename Msg, typename... Args>
revocable_msg_id_t make_and_send(
   const so_5::mbox_t & destination,
   Args && ...args)
{
   so_5::intrusive_ptr_t<revocable_envelope_t> envelope{
      std::make_unique<revocable_envelope_t>(
         std::make_unique<Msg>(std::forward<Args>(args)...)));
   revocable_msg_id_t result{envelope};
   destination->do_deliver_enveloped_msg(
      so_5::message_payload_type<Msg>::subscription_type_index(),
      envelope,
      1u);
   return result;
}

// Send a revocable message.
auto id = make_and_send<my_message>(to, ...);
...
// Revoke a message.
id.revoke();
```
If `revoke()` method is called when message is waiting in receiver's queue then the message won't be delivered to the receiver (because envelope won't provide access to the payload). But the calling to `revoke()` do not decrement the quantity of messages on type `my_message` in receiver's queue. Quantity of messages will be decremented only after extraction of an envelope from receiver's queue.

It means that `revoke()` have no influence on message limits.
# Nested Envelopes
It is possible to create envelopes that holds another envelopes.

There is no need to know a type of the payload. For example this simple envelope can hold envelopes as well as messages or signals:
```{.cpp}
class demo_envelope final : public so_5::enveloped_msg::envelope_t {
   so_5::message_ref_t payload_;
public:
   demo_envelope(so_5::message_ref_t payload) : payload_{std::move(payload)} {}

   void access_hook(access_context_t, handler_invoker_t & invoker) const noexcept {
      invoker.invoke(payload_info_t{payload_});
   }
};
```
This is because the standard implementations of `handler_invoker_t` check the kind of payload. And if a payload is another envelope these implementation call appropriate hook for that envelope recursively.

An user should take care of payload kind only if he or she implements own `handler_invoker_t` class. In that case `so_5::message_t::kind_t` and `so_5::message_kind()` should be used.
# Ready To Use Envelopes
SObjectizer v.5.5.23 doesn't provide ready to use envelopes. SObjectizer core only implements basic functionality for envelopes.

Ready to use envelopes can be found in so_5_extra library that is built on top of SObjectizer.
