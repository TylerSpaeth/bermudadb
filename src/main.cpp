#include <bermudadb/common/logger.h>

int main() {
	bermudadb::Logger logger(nullptr);
	logger.Info("Application started");
	return 0;
}
