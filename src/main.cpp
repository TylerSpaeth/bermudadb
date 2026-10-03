#include <bermudadb/common/Logger.hpp>

int main() {
	const bermudadb::Logger logger(nullptr);
	logger.info("Application started");
	return 0;
}
