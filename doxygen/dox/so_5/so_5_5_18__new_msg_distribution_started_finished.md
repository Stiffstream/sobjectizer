# so-5.5.18: New messages distribution_started and distribution_finished {#so_5_5_18__new_msg_distrib_started_finished}

[The feature of getting monitoring
information](https://sourceforge.net/p/sobjectizer/wiki/so-5.5%20In-depth%20-%20Run-Time%20Monitoring/)
about the events inside SObjectizer Environment was implemented long ago since
version 5.5.4. The mechanism works on clock cycle: user is able to set the rate
of receiving the monitoring information. For example, once in 250ms. This means
that every 250ms SObjectizer will send message pack with current values to
special mbox.

The minor defect was found out in this mechanism with time: for the series of
processing monitoring information scenarios it was very desirable to know that
the next cycle of data distribution  started or finished. In version 5.5.18
this defect was fixed. New types of messages: `distribution_started` and
`distribution_finished` were added to the  `so_5::stats::messages` namespace.
The message `distribution_started` is sent in the beginning of each cycle of
monitoring information distribution. Actually it’s the first message being sent
by stats_controller in the cycle begin. Then follow messages from data sources
inside SObjectizer Environment. The cycle is closed by `distribution_finished`
message. This is the last message and there will not be any more messages
related to the closed cycle.

These two messages simplify the scenarios processing which require information
about the moments of cycle begin/end. For example to update information on the
charts or to fixate the transaction with new data in DB.

Below is the example of agent that initiates the distribution of monitoring
information 3 times per second and then processes only messages with
information about request queues length on worker threads running in
SObjectizer Environment dispatchers (to filter the required messages the
[delivery_filters](https://sourceforge.net/p/sobjectizer/wiki/so-5.5.5%20Message%20delivery%20filters/)
mechanism is used).

~~~~~{.cpp}
// Agent for receiving run-time monitoring information.
class a_stats_listener_t : public so_5::agent_t
{
public :
   a_stats_listener_t(
      // Environment to work in.
      context_t ctx,
      // Address of logger.
      so_5::mbox_t logger )
      :  so_5::agent_t( ctx )
      ,  m_logger( std::move( logger ) )
   {}

   virtual void so_define_agent() override
   {
      using namespace so_5::stats;

      auto & controller = so_environment().stats_controller();

      // Set up a filter for messages with run-time monitoring information.
      so_set_delivery_filter(
         // Message box to which delivery filter must be set.
         controller.mbox(),
         // Delivery predicate.
         []( const messages::quantity< std::size_t > & msg ) {
            // Process only messages related to dispatcher's queue sizes.
            return suffixes::work_thread_queue_size() == msg.m_suffix;
         } );

      // We must receive messages from run-time monitor.
      so_default_state()
         .event(
            // This is mbox to that run-time statistic will be sent.
            controller.mbox(),
            &a_stats_listener_t::evt_quantity )
         .event( controller.mbox(),
            [this]( const messages::distribution_started & ) {
               so_5::send< log_message >( m_logger, "--- DISTRIBUTION STARTED ---" );
            } )
         .event( controller.mbox(),
            [this]( const messages::distribution_finished & ) {
               so_5::send< log_message >( m_logger, "--- DISTRIBUTION FINISHED ---" );
            } );
   }

   virtual void so_evt_start() override
   {
      // Change the speed of run-time monitor updates.
      so_environment().stats_controller().set_distribution_period(
            std::chrono::milliseconds( 330 ) );
      // Turn the run-timer monitoring on.
      so_environment().stats_controller().turn_on();
   }

private :
   const so_5::mbox_t m_logger;

   void evt_quantity(
      const so_5::stats::messages::quantity< std::size_t > & evt )
   {
      std::ostringstream ss;

      ss << "stats: '" << evt.m_prefix << evt.m_suffix << "': " << evt.m_value;

      so_5::send< log_message >( m_logger, ss.str() );
   }
}; 
~~~~~

