# so-5.5.19 Stop Guards {#so_5_5_19__stop_guards}

[TOC]

# The Problem
Sometimes an agent can want to prevent the shutdown of SObjectizer for some time. For example if the agent is caching some data in memory and it is necessary to store that data to the disk when the application finishes its work. But there is a problem: if someone calls `so_5::environment_t::stop()` method then there was no possibility to start some rather long operation like data saving.

There was just one solution for that problem in versions prior to 5.5.19.2: an application-wise policy to not to call so_5::environment_t::stop() directly. Some shutdown-coordinator can be used instead. For example: an agent-coordinator which receives shutdown-requests, makes shutdown-announces, collects shutdown-acks and performs actual shutdown.

This scheme is rather simple but is has a major drawback: every agent in an application must follow this scheme. It is very hard to integrate several different agents libraries into one application if they uses different shutdown prevention schemes.
# The Solution
Since v.5.5.19.2 SObjectizer provides the basic building blocks for uniform shutdown prevention scheme: stop_guards.

Stop_guard is an object which implements `so_5::stop_guard_t` interface. A stop_guard must be set into SObjectizer Environment by method `so_5::environment_t::setup_stop_guard()`. When someone starts the shutdown by calling `so_5::environment_t().stop()` SObjectizer Environment calls `so_5::stop_guard_t::stop()` method for every installed stop_guard and waits until all stop_guards will be removed.

An user then must remove all installed stop_guards. When all installed stop_guards are removed SObjectizer Environment starts the shutdown operation. At this point it is impossible to prevent the shutdown (just like in previous versions of SObjectizer).
## An Example
The is a very simple example of preventing SObjectizer's shutdown until all cached data will be stored:
~~~~~{.cpp}
// An agent which caches data and needs some form of shutdown prevention.
class cache_agent : public so_5::agent_t
{
    // A stop guard type for that agent.
    class shutdown_guard final : public so_5::stop_guard_t
    {
    public:
        // A signal to be sent when shutdown is requested.
        struct shutdown_requested final : public so_5::signal_t {};
        // Constructor receives direct mbox of cache_agent.
        shutdown_agent(so_5::mbox_t cache) : m_cache(std::move(cache)) {}
        virtual void stop() noexcept override
        {
            // Inform cache_agent about shutdown request.
            so_5::send<shutdown_requested>(m_cache);
        }
    private:
        const so_5::mbox_t m_cache;
    };
    
    // A pointer to cache's stop_guard.
    const so_5::stop_guard_shptr_t m_shutdown_guard;
    ...
public:
    cache(context_t ctx, ...)
        : so_5::agent_t(std::move(ctx))
        , m_shutdown_guard(std::make_shared<shutdown_guard>(so_direct_mbox()))
        ...
    {
        // Shutdown guard must be installed into SObjectizer Environment.
        so_environment().setup_stop_guard(m_shutdown_guard);
        ...
    }
    virtual void so_define_agent() override
    {
        // Cache agent must be subscribed to shutdown request.
        so_subscribe_self().event(&cache_agent::on_shutdown_request);
        ...
    }
private:
    void on_shutdown_request(mhood_t<shutdown_guard::shutdown_requested>)
    {
        // Shutdown has been requested. Cache data must be stored to disk.
        ... // Initiate a data storing process.
    }
    void on_data_store_completed(mhood_t<store_completed>)
    {
        // All data stored. It is time to enable the shutdown operation.
        // Just remove out stop_guard.
        so_environment().remove_stop_guard(m_shutdown_guard);
    }
    ...
};
~~~~~
## Some Technical Notes
### Stop_guards Can't Be Installed If stop() Is Already Called
It is possible to install stop_guards only before the first call to `so_5::environment_t::stop()` method. If `so_5::environment_t::stop()` is already called all subsequent calls to `so_5::environment_t::setup_stop_guard()` will fail.

By default method `setup_stop_guard()` throws an exception if `stop()` is already called. But if such policy is not appropriate it can be changed by using second argument to `setup_stop_guard()`:
~~~~~{.cpp}
const auto result = so_environment().setup_stop_guard(
        my_guard,
        so_5::stop_guard_t::what_if_stop_in_progress_t::return_negative_result );
if(so_5::stop_guard_t::setup_result_t::ok != result)
    ... // Handle setup error here.
~~~~~

### Why stop_guard_t::stop() is noexcept?
Method `so_5::stop_guard_t::stop()` shouldn't throw. It is because there is no way to rollback shutdown-specific actions which were done before an exception. It means that shutdown preparation procedure can be broken without a chance to recover.

This is the main reason why `so_5::stop_guard_t::stop()` is marked as `noexcept`: it shouldn't throw. If there can be some exceptions inside user's stop_guard `stop()` method which do not prevent shutdown operation they must be caught and handled inside that `stop()` method. Otherwise the only way is to terminate the current application.
