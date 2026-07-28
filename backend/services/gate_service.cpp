#include "gate_service.h"
#include "repositories/gate_repository.h"
GateService::GateService() : GateService({GateRepository::getAllGates, GateRepository::getAvailableGates}) {}
GateService::GateService(Dependencies dependencies) : dependencies_(std::move(dependencies)) {}
Json::Value GateService::getAll() const { return dependencies_.all(); }
Json::Value GateService::getAvailable() const { return dependencies_.available(); }
