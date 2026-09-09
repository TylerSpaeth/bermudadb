#include <bermudadb/common/logger.hpp>

int main() {
	bermudadb::Logger logger(nullptr);
	logger.info("Application started");
	return 0;
}
