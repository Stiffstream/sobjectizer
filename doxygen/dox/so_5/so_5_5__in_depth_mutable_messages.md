# so-5.5 In Depth - Mutable Messages {#so_5_5_in_depth__mutable_messages}

# Introduction
Since the very beginning there were only immutable messages in SObjectizer-5.

Immutable message is a very simple and safe approach to implement an interaction in a concurrent application:

* a message instance can be received by any number of receivers at the same time;
* a message can be redirected to any number of new receivers;
* a message can be stored to be processed later...

Because of that immutable messages are very useful in 1:N or N:M interactions. And because Publish-Subscribe Model was the first model supported by SObjectizer-5 the interaction via immutable messages is used by default.

But there can be cases when immutable message is not a good choice in 1:1 interaction...

## Example 1: Big Messages
Let's consider a pipeline of agents which need to modify a big binary object...

A message like this:
~~~~~{.cpp}
struct raw_image_fragment final : public so_5::message_t {
   std::array<std::uint8_t, 10*1024*1024> raw_data_;
   ...
};
~~~~~
That need to be processed by a pipeline like that:
~~~~~{.cpp}
class first_modificator final : public so_5::agent_t {
   void on_fragment(mhood_t<raw_image_fragment> cmd) {
      ... // Some modification of cmd's contents.
      next_stage_->deliver_message(cmd.make_reference()); // Send to the next.
   }
   ...
};
class second_modificator final : public so_5::agent_t {
   void on_fragment(mhood_t<raw_image_fragment> cmd) {
      ... // Some modification of cmd's contents.
      next_stage_->deliver_message(cmd.make_reference()); // Send to the next.
   }
   ...
};
~~~~~
But... It won't be compiled!
~~~~~{.cpp}
class first_modificator final : public so_5::agent_t {
   void on_fragment(mhood_t<raw_image_fragment> cmd) {
      cmd->raw_data_[0] = ...; // WON'T COMPILE! cmd->raw_data_ is const!
      ...
      next_stage_->deliver_message(cmd.make_reference()); // Send to the next.
   }
   ...
};
class second_modificator final : public so_5::agent_t {
   void on_fragment(mhood_t<raw_image_fragment> cmd) {
      cmd->raw_data_[1] = ...; // WON'T COMPILE! cmd->raw_data_ is const!
      ...
      next_stage_->deliver_message(cmd.make_reference()); // Send to the next.
   }
   ...
};
~~~~~
A safe way is copy, modify and send modified copy...
~~~~~{.cpp}
class first_modificator final : public so_5::agent_t {
   void on_fragment(mhood_t<raw_image_fragment> cmd) {
      auto cp = std::make_unique<raw_image_fragment>(*cmd);
      cp->raw_data_[0] = ...;
      ...
      next_stage_->deliver_message(std::move(cp)); // Send to the next.
   }
   ...
};
class second_modificator final : public so_5::agent_t {
   void on_fragment(mhood_t<raw_image_fragment> cmd) {
      auto cp = std::make_unique<raw_image_fragment>(*cmd);
      cp->raw_data_[1] = ...;
      ...
      next_stage_->deliver_message(std::move(cp)); // Send to the next.
   }
   ...
};
~~~~~
It's obvious that the safe way is a very, very inefficient...
## Example 2: Messages With Moveable Data Inside
Let's consider a case where agent Alice opens a file and then transfers opened file to agent Bob:
~~~~~{.cpp}
struct process_file final : public so_5::message_t { // A message to transfer opened file.
   std::ifstream file_;
   process_file(std::ifstream file) : file_(std::move(file)) {}
};
class Alice final : public so_5::agent_t {
   ...
   void on_handle_file(mhood_t<start_file_processing> cmd) {
      std::ifstream file(cmd->file_name()); // Try to open...
      if(file) so_5::send<process_file>(bob_mbox, std::move(file)); // Transfer file to Bob.
   }
};
class Bob final : public so_5::agent_t {
   ...
   void on_process_file(mhood_t<process_file> cmd) {
      ... // Some file processing code.
   }
};
~~~~~
But if we try to do something like that:
~~~~~{.cpp}
class Bob final : public so_5::agent_t {
   ...
   void on_process_file(mhood_t<process_file> cmd) {
      std::ifstream file(std::move(cmd->file_)); // (1)
      ... // Processing file content.
   }
};
~~~~~
We will get a compile-time error at point (1) because cmd->file\_ is const and can't be moved anywhere...
## There Are Some Workarounds Of Course...
You can declare fields of your messages as mutable:
~~~~~{.cpp}
struct raw_image_fragment final : public so_5::message_t {
   mutable std::array<std::uint8_t, 10*1024*1024> raw_data_;
   ...
};
class first_modificator final : public so_5::agent_t {
   void on_fragment(mhood_t<raw_image_fragment> cmd) {
      cmd->raw_data_[0] = ...; // Now it works.
      ...
      next_stage_->deliver_message(cmd.make_reference()); // Send to the next.
   }
   ...
};
~~~~~
But what if your message is received by two agents at the same time? There is no any guarantee that message will be delivered only to the single receiver...

Or you can use shared_ptr instead of object itself:
~~~~~{.cpp}
struct process_file final : public so_5::message_t { // A message to transfer opened file.
   std::shared_ptr<std::ifstream> file_;
   process_file(std::shared_ptr<std::ifstream> file) : file_(std::move(file)) {}
};
class Alice final : public so_5::agent_t {
   ...
   void on_handle_file(mhood_t<start_file_processing> cmd) {
      auto file = std::make_shared<std::ifstream>(cmd->file_name()); // Try to open...
      if(*file) so_5::send<process_file>(bob_mbox, std::move(file)); // Transfer file to Bob.
   }
};
~~~~~
But there is additional memory allocation and additional level of data indirection. Overhead can be significant if you need to transfer small objects like mutexes.
# The Real Solution: Mutable Messages
Since v.5.5.19 a message of type Msg can be sent either as immutable one:
~~~~~{.cpp}
so_5::send<Msg>(dest, ... /* Msg's constructor args */);
so_5::send_delayed<Msg>(dest, pause, ... /* Msg's constructor args */);
~~~~~
and as mutable one:
~~~~~{.cpp}
so_5::send<so_5::mutable_msg<Msg>>(dest, ... /* Msg's constructor args */);
so_5::send_delayed<so_5::mutable_msg<Msg>>(dest, pause, ... /* Msg's constructor args */);
~~~~~
To receive and handle a mutable message an event handler must have on of the following formats:
~~~~~{.cpp}
result_type handler(so_5::mhood_t<so_5::mutable_msg<Msg>>);
result_type handler(so_5::mutable_mhood_t<Msg>);
~~~~~
Note, that `mutable_mhood_t<M>` is just a shorthand for `mhood_t<mutable_msg<M>>`. Usage of `mutable_mhood_t<M>` makes code more compact and concise. But `mhood_t<mutable_msg<M>>` can be used in templates:
~~~~~{.cpp}
template<typename M> // Can be Msg or mutable_msg<Msg>
class demo : public so_5::agent_t {
   ...
   void on_message(mhood_t<M> cmd) {
      ...
   }
};
~~~~~
With mutable messages the examples above can be rewritten that way:

**An example with big messages:**
~~~~~{.cpp}
struct raw_image_fragment final : public so_5::message_t {
   std::array<std::uint8_t, 10*1024*1024> raw_data_;
   ...
};
class first_modificator final : public so_5::agent_t {
   void on_fragment(mutable_mhood_t<raw_image_fragment> cmd) {
      cmd->raw_data_[0] = ...; // Now it works.
      ...
      so_5::send(next_stage_, std::move(cmd)); // Send to the next.
   }
   ...
};
class second_modificator final : public so_5::agent_t {
   void on_fragment(mutable_mhood_t<raw_image_fragment> cmd) {
      cmd->raw_data_[1] = ...; // Now it works.
      ...
      so_5::send(next_stage_, std::move(cmd)); // Send to the next.
   }
   ...
};
~~~~~
**An example with moveable object inside:**
~~~~~{.cpp}
struct process_file final : public so_5::message_t { // A message to transfer opened file.
   std::ifstream file_;
   process_file(std::ifstream file) : file_(std::move(file)) {}
};
class Alice final : public so_5::agent_t {
   ...
   void on_handle_file(mhood_t<start_file_processing> cmd) {
      std::ifstream file(cmd->file_name()); // Try to open...
      if(file) so_5::send<so_5::mutable_msg<process_file>>(bob_mbox, std::move(file)); // Transfer file.
   }
};
class Bob final : public so_5::agent_t {
   ...
   void on_process_file(mutable_mhood_t<process_file> cmd) {
      std::ifstream file(std::move(cmd->file_)); // Now it works because cmd->file_ is not const.
      ... // Some file processing code.
   }
};
~~~~~
# Safety Of Mutable Messages
But why sending of a mutable message is safer that sending an immutable message with mutable fields inside? Are there some guarantees from SObjectizer?

A mutable message can be sent only to MPSC mbox or mchain. It means that there can be at most one receiver of the message. An attempt to send mutable message to MPMC mbox will lead to an exception at run-time.

A `mutable_mhood_t<M>` works just like `std::unique_ptr`: *when you redirect your mutable message to someone else your mutable_mhood_t becomes nullptr.*

It means that you lost your access to mutable message after redirection:
~~~~~{.cpp}
void on_fragment(mutable_mhood_t<raw_image_fragment> cmd) {
   cmd->raw_data_[0] = ...; // Now it works.
   ...
   so_5::send(next_stage_, std::move(cmd)); // cmd is a nullptr now!

   cmd->raw_data_[0] = ...; // It will lead to access violation or something similar.
}
~~~~~
These all mean that only one receiver can have access to mutable message instance at some time.

This property can't be satisfied for immutable message.

And this makes usage of mutable messages safe.
# Immutable And Mutable Message Are Different
Mutable message of type `M` has different type than immutable message of type `M`. It means that an agent can have different event handlers for mutable and immutable `M`:
~~~~~{.cpp}
class two_handlers final : public so_5::agent_t {
    struct M final {};
public :
    two_handlers(context_t ctx) : so_5::agent_t(std::move(ctx)) {
        so_subscribe_self()
            .event(&two_handlers::on_immutable_M)
            .event(&two_handlers::on_mutable_M);
    }
    virtual void so_evt_start() override {
        so_5::send<M>(*this); // Immutable message is sent.
        so_5::send<so_5::mutable_msg<M>>(*this); // Mutable message is sent.
    }
private :
    void on_immutable_M(mhood_t<M>) { std::cout << "on immutable" << std::endl; }
    void on_mutable_M(mhood_t<so_5::mutable_msg<M>>) { std::cout << "on mutable" << std::endl; }
};
~~~~~
# Mutable Messages In Synchronous Interaction
A mutable message can be used for service requests (e.g. for synchronous interactions):
~~~~~{.cpp}
class service_provider final : public so_5::agent_t {
public :
    service_provider(context_t ctx) : so_5::agent_t(std::move(ctx)) {
       // Service request handler.
        so_subscribe_self().event([](mutable_mhood_t<std::string> cmd) {
            *cmd = "<" + *cmd + ">"; // Modify incoming message.
            return std::move(*cmd); // Return modified value.
        });
    }
    ...
};
...
so_5::mbox_t provider_mbox = ...; // Must be MPSC mbox or mchain.
auto r = so_5::request_value<std::string, so_5::mutable_msg<std::string>>( // Initiate request.
        provider_mbox, so_5::infinite_wait, "hello");
~~~~~
But note: mutable service request can be sent only into MPSC-mbox or mchain.
# Conversion Into An Immutable Message
When a mutable message is received via `mutable_mhood_t` and then redirected via `send` or `request_value/future` then redirected message will also be a mutable message. It means that redirected message can be sent only to one subscriber and can be handled only via `mutable_mhood_t`.

Sometimes it is necessary to remove mutability of a message and send the message as immutable one. It can be done via `to_immutable` helper function.

Helper function `to_immutable` converts its argument from `mutable_mhood_t<M>` into `mhood_t<M>` and returns message hood to immutable message. This new message hood can be used as parameter for `send`, `request_value` or `receive_future`. Old mutable message hood becomes a nullptr and can't be used anymore.
~~~~~{.cpp}
void some_agent::on_some_message(mutable_mhood_t<some_message> cmd) {
    ... // Some actions with the content of cmd.
    // Now the mutable message will be resend as immutable one.
    so_5::send(another_mbox, so_5::to_immutable(std::move(cmd)));
    // NOTE: cmd is a nullptr now. It can't be used anymore.
    ...
}
~~~~~
Note: a mutable message can be converted to immutable message only once. An immutable message can't be converted into mutable one.
# Mutable Messages And Timers
Mutable messages can be sent by send_delayed functions:
~~~~~{.cpp}
// It is a valid call:
so_5::send_delayed<so_5::mutable_msg<some_message>>(
    so_environment(), dest_mbox,
    std::chrono::milliseconds(200), // Delay before message appearance.
    ... // some_message's constructor args.
    );
~~~~~
Mutable messages can't be sent as periodic messages. It means that send_periodic can be used with mutable_msg only if a period parameter is zero:
~~~~~{.cpp}
// It is a valid call:
auto timer = so_5::send_periodic<so_5::mutable_msg<some_message>>(
    so_environment(), dest_mbox,
    std::chrono::milliseconds(200), // Delay before message appearance.
    std::chrono::milliseconds::zero(), // Period is zero.
    ...);

// It isn't a valid call. An exception will be thrown at run-time.
auto timer = so_5::send_periodic<so_5::mutable_msg<some_message>>(
    so_environment(), dest_mbox,
    std::chrono::milliseconds(200), // Delay before message appearance.
    std::chrono::milliseconds(150), // Period is not zero.
    ...);
~~~~~
# Signals Can't Be Mutable
Signals do not carry any information inside. Because of that there is no sense in `mutable_msg<S>` where S is a signal type.

Because of that an attempt to use `mutable_msg<S>` in code will lead to compile-time error if S is a signal.

