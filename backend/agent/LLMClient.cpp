#include "LLMClient.h"
#include <drogon/drogon.h>
#include <json/json.h>
#include <cstdlib>
#include <memory>
#include <thread>
#include <chrono>

LLMClient::LLMClient()
{
    // Now backed by Groq (OpenAI-compatible API).
    const char *key = std::getenv("GROQ_API_KEY");
    api_key_ = key ? key : "";
}

std::string LLMClient::ask(const std::string &question)
{
    if (api_key_.empty())
    {
        return "ERROR: GROQ_API_KEY is not set";
    }

    // Build request body in OpenAI chat format:
    // {"model": "...", "messages": [{"role": "user", "content": question}]}
    Json::Value body;
    body["model"] = "llama-3.3-70b-versatile";
    Json::Value message;
    message["role"] = "user";
    message["content"] = question;
    body["messages"].append(message);

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string bodyStr = Json::writeString(writer, body);

    auto client = drogon::HttpClient::newHttpClient(
        "https://api.groq.com",
        nullptr, false, false);

    std::string lastError;

    for (int attempt = 1; attempt <= 4; ++attempt)
    {
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Post);
        req->setPath("/openai/v1/chat/completions");
        req->addHeader("Authorization", "Bearer " + api_key_);
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        req->setBody(bodyStr);

        auto [result, response] = client->sendRequest(req, 30.0);

        if (result != drogon::ReqResult::Ok || !response)
        {
            lastError = "network error";
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            continue;
        }

        std::string respBody(response->getBody());
        int httpStatus = response->getStatusCode();

        if (httpStatus == 503 || httpStatus == 429)
        {
            lastError = "HTTP " + std::to_string(httpStatus) + ": " + respBody;
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
            return "ERROR: could not parse response";
        }

        // OpenAI-format response: choices[0].message.content
        if (parsed.isMember("choices") && parsed["choices"].isArray() && !parsed["choices"].empty())
        {
            const Json::Value &choice = parsed["choices"][0];
            if (choice.isMember("message") && choice["message"].isMember("content"))
            {
                return choice["message"]["content"].asString();
            }
        }

        return "ERROR: unexpected response, body: " + respBody;
    }

    return "ERROR: provider unavailable after retries: " + lastError;
}

// ---- Tool-calling chat completion ----
Json::Value LLMClient::chatWithTools(const Json::Value &messages, const Json::Value &tools)
{
    Json::Value result;

    if (api_key_.empty())
    {
        result["error"] = "GROQ_API_KEY is not set";
        return result;
    }

    Json::Value body;
    body["model"] = "llama-3.3-70b-versatile";
    body["messages"] = messages;
    if (tools.isArray() && !tools.empty())
    {
        body["tools"] = tools;
        body["tool_choice"] = "auto";
    }

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    std::string bodyStr = Json::writeString(writer, body);

    auto client = drogon::HttpClient::newHttpClient(
        "https://api.groq.com", nullptr, false, false);

    for (int attempt = 1; attempt <= 4; ++attempt)
    {
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setMethod(drogon::Post);
        req->setPath("/openai/v1/chat/completions");
        req->addHeader("Authorization", "Bearer " + api_key_);
        req->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        req->setBody(bodyStr);

        auto [res, response] = client->sendRequest(req, 30.0);

        if (res != drogon::ReqResult::Ok || !response)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            continue;
        }

        std::string respBody(response->getBody());
        int httpStatus = response->getStatusCode();

        if (httpStatus == 503 || httpStatus == 429)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 * attempt));
            continue;
        }

        Json::Value parsed;
        Json::CharReaderBuilder reader;
        std::string errs;
        std::unique_ptr<Json::CharReader> jsonReader(reader.newCharReader());

        if (!jsonReader->parse(respBody.c_str(), respBody.c_str() + respBody.size(), &parsed, &errs))
        {
            result["error"] = "could not parse response";
            return result;
        }

        if (httpStatus != 200)
        {
            result["error"] = "HTTP " + std::to_string(httpStatus)+ ": " +respBody;
            result["raw"] = parsed;
            return result;
        }

        return parsed;
    }

    result["error"] = "provider unavailable after retries";
    return result;
}
