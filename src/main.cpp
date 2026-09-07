#include <bermudadb/common/logger.h>

int main() {
	bermudadb::Logger logger(nullptr);
	logger.info("Application started");
	return 0;
}
