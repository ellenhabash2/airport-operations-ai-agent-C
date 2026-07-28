#include "runway_service.h"
#include "repositories/runway_repository.h"
RunwayService::RunwayService() : RunwayService(RunwayRepository::getAllRunways) {}
RunwayService::RunwayService(Loader loader) : loader_(std::move(loader)) {}
Json::Value RunwayService::getStatus() const { return loader_(); }
