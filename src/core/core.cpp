#include "../../include/core/core.h"


unsigned int fluid::Core::refresh_rate = 30;
unsigned int fluid::Core::message_queue_size = 1000;

std::shared_ptr<fluid::StateGraph> fluid::Core::graph_p_;
std::shared_ptr<fluid::StatusPublisher> fluid::Core::status_publisher_p_;

std::shared_ptr<fluid::StateGraph> fluid::Core::getGraphPtr() {
	if (!graph_p_) {
		graph_p_ = std::make_shared<fluid::StateGraph>();
	}

	return graph_p_;
}

std::shared_ptr<fluid::StatusPublisher> fluid::Core::getStatusPublisherPtr() {
	if (!status_publisher_p_) {
		status_publisher_p_ = std::make_shared<fluid::StatusPublisher>();
	}

	return status_publisher_p_;
}