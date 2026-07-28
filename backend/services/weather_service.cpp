#include "weather_service.h"
#include "domain_error.h"
#include "repositories/weather_repository.h"
WeatherService::WeatherService() : WeatherService({WeatherRepository::getRecentWeather,
    WeatherRepository::getLatestWeather, WeatherRepository::createWeather}) {}
WeatherService::WeatherService(Dependencies dependencies) : dependencies_(std::move(dependencies)) {}
Json::Value WeatherService::getRecent() const { return dependencies_.recent(); }
Json::Value WeatherService::getLatest() const { return dependencies_.latest(); }
Json::Value WeatherService::create(const std::string &condition, float visibility, float wind, float temperature) const {
    if (condition.empty() || visibility < 0.0f || wind < 0.0f)
        throw DomainError(DomainErrorKind::Validation, "invalid_weather", "Invalid weather payload");
    return dependencies_.create(condition, visibility, wind, temperature);
}
