#include "GeminiClient.h"
#include <drogon/drogon.h>
#include <json/json.h>
#include <cstdlib>
#include <memory>
#include <thread>
#include <chrono>

GeminiClient::GeminiClient()
{
    const char *key = std::getenv("GEMINI_API_KEY");
    api_key_ = key ? key : "";
}

std::string GeminiClient::ask(const std::string &question)
{
    if (api_key_.empty())
    {
        return "ERROR: GEMINI_API_KEY is not set";
    }

    Json::Value body;
    Json::Value part;
    part["text"] = question;
    Json::Value content;
    content["parts"].append(part);
    body["contents"].append(content);

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string bodyStr = Json::writeString(writer, body);

    auto client = drogon::HttpClient::newHttpClient(
        "https://generativelanguage.googleapis.com",
        nullptr, false, false);

    std::string lastError;

    // Retry up to 4 times on transient 503 (high demand) errors.
    for (int attempt = 1; attempt <= 4; ++attempt)
    {
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Post);
        req->setPath("/v1beta/models/gemini-2.0-flash-lite:generateContent");
        req->setParameter("key", api_key_);
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        req->setBody(bodyStr);
        req->setPathEncode(false);

        auto [result, response] = client->sendRequest(req, 30.0);

        if (result != drogon::ReqResult::Ok || !response)
        {
            lastError = "request failed, result code = " + std::to_string(static_cast<int>(result));
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            continue;
        }

        std::string respBody(response->getBody());
        int httpStatus = response->getStatusCode();

        // 503 = temporary high demand: wait and retry.
        if (httpStatus == 503)
        {
            lastError = "HTTP 503 (high demand), body: " + respBody;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 * attempt));
            continue;
        }

        if (httpStatus != 200)
        {
            return "ERROR: HTTP status = " + std::to_string(httpStatus) + ", body: " + respBody;
        }

        Json::Value parsed;
        Json::CharReaderBuilder reader;
        std::string errs;
        std::unique_ptr<Json::CharReader> jsonReader(reader.newCharReader());

        if (!jsonReader->parse(respBody.c_str(), respBody.c_str() + respBody.size(), &parsed, &errs))
        {
            return "ERROR: parse failed, body: " + respBody;
        }

        if (parsed.isMember("candidates") && parsed["candidates"].isArray() && !parsed["candidates"].empty())
        {
            const Json::Value &cand = parsed["candidates"][0];
            if (cand.isMember("content") && cand["content"].isMember("parts") &&
                cand["content"]["parts"].isArray() && !cand["content"]["parts"].empty())
            {
                return cand["content"]["parts"][0]["text"].asString();
            }
        }

        return "ERROR: unexpected response, body: " + respBody;
    }

    return "ERROR: all retries failed. Last error: " + lastError;
}

