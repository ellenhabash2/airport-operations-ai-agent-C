#pragma once
#include <functional>
#include <json/json.h>
#include <string>

class GateService {
public:
    struct Dependencies {
        std::function<Json::Value()> all, available;
        std::function<Json::Value(const std::string &)> byNumber;
        std::function<Json::Value(int)> byId, byTerminal;
        std::function<bool(int, bool)> updateAvailability;
    };
    GateService(); explicit GateService(Dependencies dependencies);
    Json::Value getAllGates() const; Json::Value getAvailableGates() const;
    Json::Value getGateById(int gateId) const;
    Json::Value getGateByNumber(const std::string &gateNumber) const;
    Json::Value getGatesByTerminal(int terminalId) const;
    bool setGateAvailability(int gateId, bool available) const;

    // Compatibility names retained for Phase 1/2 callers.
    Json::Value getAll() const { return getAllGates(); }
    Json::Value getAvailable() const { return getAvailableGates(); }
    Json::Value getByNumber(const std::string &gateNumber) const;
    static bool isGateAvailable(const Json::Value &gate);
    static bool isGateOperational(const Json::Value &gate);
private: Dependencies dependencies_;
};
