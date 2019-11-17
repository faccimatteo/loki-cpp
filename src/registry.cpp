#include "registry.hpp"
#include "agent.hpp"

namespace loki
{

Registry::Registry(const std::map<std::string, std::string> &labels,
				   int flush_interval,
				   int max_buffer,
				   Agent::LogLevels log_level)
	: labels_{labels}
	, flush_interval_{flush_interval}
	, max_buffer_{max_buffer}
	, log_level_{log_level}
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
	thread_ = std::thread([this]() {
		while (!close_request_.load()) {
			//if (std::chrono::duration(std::chrono::system_clock::now() - last_flush_).count() > flush_interval_) {
				for (auto &agent : agents_)
					agent->Flush();
				//last_flush_ = std::chrono::system_clock::now();
			//}
		}
	});
}

Registry::~Registry()
{
	close_request_.store(true);
	if (thread_.joinable()) {
		thread_.join();
	}
	curl_global_cleanup();
}

Agent &Registry::Add(std::map<std::string, std::string> labels)
{
	std::lock_guard<std::mutex> lock{mutex_};
	for (const auto &p : labels_)
		labels.emplace(p);
	auto agent = std::make_unique<Agent>(labels, flush_interval_, max_buffer_, log_level_);
	auto &ref = *agent;
	agents_.push_back(std::move(agent));
	return ref;
}

} // namespace loki