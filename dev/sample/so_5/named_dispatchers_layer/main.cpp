/*
 * An example of usage of SObjectizer's layer for holding a dictionary of named
 * dispatchers.
 */
#include <iostream>

#include <so_5/all.hpp>

// Implemetation of SObjectizer's layer that plays a role of
// a dictionary of named one_thread dispatchers.
class disp_binder_dictionary_layer_t : public so_5::layer_t
{
	// Type of dictionary for holding dispatcher binders.
	using map_t = std::map<std::string, so_5::disp_binder_shptr_t, std::less<>>;

	// Because layer can be used from different worker threads it should
	// be protected.
	std::mutex m_lock;

	// The dictionary.
	map_t m_dict;

public:
	// There is no need to override start()/shutdown()/wait() methods
	// because the basic implementation does nothing.

	// An interface of dictionary.
	
	// Add a new binder if there is no such name in the dictionary.
	void add(
		const std::string & name,
		so_5::disp_binder_shptr_t binder)
	{
		std::lock_guard<std::mutex> l{m_lock};
		auto [it, inserted] = m_dict.emplace(name, std::move(binder));
		(void)it; // To suppress warnings about unused variables.
		if(!inserted)
			throw std::runtime_error(name + ": is not unique name");
	}

	// Try find binder in the dictionary.
	// Absence of a binder is not an error.
	std::optional<so_5::disp_binder_shptr_t>
	try_get(std::string_view name) noexcept
	{
		std::lock_guard<std::mutex> l{m_lock};
		if(const auto it = m_dict.find(name); it != m_dict.end())
			return { it->second };
		else
			return std::nullopt;
	}

	// Try get binder from the dictinary.
	// Absence of a binder is reported by an exception.
	so_5::disp_binder_shptr_t
	get(std::string_view name)
	{
		auto r = try_get(name);
		if(!r) throw std::runtime_error("binder is not found");
		return std::move(*r);
	}
};

// Type of demo agent to be used in the example.
//
// This agent tells the parent about its start and then deregisters itself.
//
class a_child_t final : public so_5::agent_t
{
public:
	// Type of signal to be sent to the parent.
	struct i_am_completed final : public so_5::signal_t {};

	a_child_t(context_t ctx, so_5::mbox_t parent)
		:	so_5::agent_t{std::move(ctx)}
		,	m_parent{std::move(parent)}
	{}

	void so_evt_start() override
	{
		so_5::send<i_am_completed>(m_parent);

		so_deregister_agent_coop_normally();
	}

private:
	const so_5::mbox_t m_parent;
};

// Another type of demo agent to be used in the example.
class a_parent_t final : public so_5::agent_t
{
public:
	a_parent_t(context_t ctx, std::string disp_name)
		:	so_5::agent_t{std::move(ctx)}
		,	m_disp_name{std::move(disp_name)}
	{}

	void so_define_agent() override
	{
		so_subscribe_self().event(
			[this](mhood_t<a_child_t::i_am_completed>) {
				// We can finish our work too.
				std::cout << "child from '" << m_disp_name << "' completed"
						<< std::endl;
				so_deregister_agent_coop_normally();
			});
	}

	void so_evt_start() override
	{
		// Create a new child on the specified dispatcher.
		so_5::introduce_child_coop( *this,
			[this](so_5::coop_t & coop) {
				auto binder = so_environment().query_layer<
						disp_binder_dictionary_layer_t
					>()->get(m_disp_name);
				coop.make_agent_with_binder<a_child_t>(binder, so_direct_mbox());
			});
	}

private:
	const std::string m_disp_name;
};

int main()
{
	so_5::launch(
		[](so_5::environment_t & env) {
			// Create several dispatchers and give them names.
			auto * layer = env.query_layer<disp_binder_dictionary_layer_t>();

			layer->add("first",
					so_5::disp::one_thread::make_dispatcher(env).binder());
			layer->add("second",
					so_5::disp::one_thread::make_dispatcher(env).binder());
			layer->add("third",
					so_5::disp::one_thread::make_dispatcher(env).binder());

			// Create several parent agents. All of them will work on
			// the default dispatcher.
			env.register_agent_as_coop(env.make_agent<a_parent_t>("first"));
			env.register_agent_as_coop(env.make_agent<a_parent_t>("second"));
			env.register_agent_as_coop(env.make_agent<a_parent_t>("third"));
			env.register_agent_as_coop(env.make_agent<a_parent_t>("second"));
			env.register_agent_as_coop(env.make_agent<a_parent_t>("first"));
			env.register_agent_as_coop(env.make_agent<a_parent_t>("third"));
			env.register_agent_as_coop(env.make_agent<a_parent_t>("third"));
			env.register_agent_as_coop(env.make_agent<a_parent_t>("third"));
			env.register_agent_as_coop(env.make_agent<a_parent_t>("third"));

			// Wait while all parents finish their work.
		},
		[](so_5::environment_params_t & params) {
			// Our layer should be created and stored in params.
			params.add_layer(std::make_unique<disp_binder_dictionary_layer_t>());
		});

	return 0;
}

