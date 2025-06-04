#pragma once

#include "Utils/TaskCoroutine.hpp"
#include "System/Net/Http/HttpClient.hpp"
#include "System/Net/Http/HttpMethod.hpp"
#include "System/Net/Http/HttpRequestMessage.hpp"
#include "System/Net/Http/HttpResponseMessage.hpp"
#include "System/Net/Http/Headers/HttpHeaders.hpp"
#include "System/Net/Http/Headers/HeaderInfo.hpp"
#include "System/Net/Http/StringContent.hpp"
#include "System/Uri.hpp"
#include "System/Text/Encoding.hpp"
#include "System/Collections/Generic/IEnumerable_1.hpp"
#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include "System/Threading/CancellationTokenSource.hpp"
#include "rapidjson-macros/shared/auto.hpp"
#include "rapidjson-macros/shared/serialization.hpp"
#include "System/Net/Http/ByteArrayContent.hpp"
#include "UnityEngine/Sprite.hpp"
#include "bsml/shared/Helpers/utilities.hpp"

#include "metacore/shared/il2cpp.hpp"

template<typename T>
concept JsonContent = JSONStruct<T> || JSONBasicType<T>;

template<typename T>
struct isJsonContainer { static constexpr bool value = false; };
template<JsonContent T>
struct isJsonContainer<StringKeyedMap<T>> { static constexpr bool value = true; };
template<JsonContent T>
struct isJsonContainer<std::vector<T>> { static constexpr bool value = true; };

template<typename T>
concept JsonContainer = isJsonContainer<T>::value;

template<typename T>
concept ResponseContent = JSONStruct<T> || std::is_same_v<std::decay_t<T>, std::vector<uint8_t>> || std::is_same_v<std::decay_t<T>, std::string> || JsonContainer<std::decay_t<T>>;

template<typename T>
concept PostContent = JSONStruct<T> || std::is_same_v<std::decay_t<T>, std::vector<uint8_t>> || std::is_same_v<std::decay_t<T>, std::string>;

template<typename T = std::string, typename U = std::string>
struct HttpResponse {
    bool success;
    int responseCode;
    std::unordered_map<std::string, std::string> headers;
    T content;
    U errorContent;
};

struct HttpService {
    using HttpRequestMessage = System::Net::Http::HttpRequestMessage;
    using HttpResponseMessage = System::Net::Http::HttpResponseMessage;
    using HeaderMap = std::unordered_map<std::string, std::string>;

    static System::Net::Http::HttpClient* get_httpClient() {
        static SafePtr<System::Net::Http::HttpClient> httpClient;
        if (!httpClient) {
            httpClient = System::Net::Http::HttpClient::New_ctor();
            httpClient->Timeout = System::TimeSpan::FromMilliseconds(-1);
        }
        return httpClient.ptr();
    }

    template<ResponseContent T = std::string, ResponseContent U = std::string>
    static task_coroutine<HttpResponse<T, U>> GetAsync(const std::string& url, const HeaderMap& headers = {}, int timeout = 10) {
        HttpResponseMessage* response = co_await SendAsync(BuildMessage(url, false, headers), timeout);
        co_return HandleResponse<T, U>(response);
    }

    template<ResponseContent T = std::string, ResponseContent U = std::string, PostContent S = std::string>
    static task_coroutine<HttpResponse<T, U>> PostAsync(const std::string& url, const S& body, const HeaderMap& headers = {}, int timeout = 10) {
        HttpResponseMessage* response = co_await SendAsync(BuildMessage(url, true, headers, body), timeout);
        co_return HandleResponse<T, U>(response);
    }

    static task_coroutine<UnityEngine::Sprite*> GetSpriteAsync(const std::string& url, const HeaderMap& headers = {}, int timeout = 10) {
        HttpResponseMessage* response = co_await SendAsync(BuildMessage(url, false, headers), timeout);
        if (!response || !response->IsSuccessStatusCode) co_return nullptr;
        co_await response->Content->LoadIntoBufferAsync();
        co_await YieldMainThread();
        ArrayW<uint8_t> byteArray = response->Content->buffer->ToArray();
        auto* texture = BSML::Utilities::LoadTextureRaw(byteArray);
        texture->hideFlags = UnityEngine::HideFlags::DontSave;
        auto* sprite = BSML::Utilities::LoadSpriteFromTexture(texture);
        sprite->hideFlags = UnityEngine::HideFlags::DontSave;
        response->Dispose();
        co_return sprite;
    }

    private:

    static void AddHeaders(HttpRequestMessage* req, const HeaderMap& headers) {
        for (auto [key, value] : headers) {
            auto array = ArrayW<StringW>({StringW(value)});
            System::Net::Http::Headers::HeaderInfo* leagalHeader;
            try { leagalHeader = req->Headers->CheckName(key); }
            catch (...) { continue; }
            req->Headers->AddInternal(key, reinterpret_cast<System::Collections::Generic::IEnumerable_1<StringW>*>(array.convert()), leagalHeader, false);
        }
    }

    template<PostContent S>
    static void AddPostContent(HttpRequestMessage* req, const S& body, const HeaderMap& headers) {
        if constexpr(std::is_same_v<std::decay_t<S>, std::vector<uint8_t>>) {
            auto array = Array<uint8_t>::NewLength(body.size());
            memcpy(array->_values, body.data(), sizeof(uint8_t)*body.size());
            req->Content = System::Net::Http::ByteArrayContent::New_ctor(ArrayW<uint8_t>(array));
            return;
        }
        auto content_type = headers.find("Content-Type");
        std::string content;
        if constexpr(JSONStruct<S>) content = WriteToString(body);
        else if constexpr(std::is_same_v<std::decay_t<S>, std::string>) content = body;
        req->Content = System::Net::Http::StringContent::New_ctor(content, System::Text::Encoding::get_UTF8(), content_type != headers.end() ? content_type->second : "");
    }

    template<PostContent S = std::string>
    static HttpRequestMessage* BuildMessage(const std::string& url, bool isPost, const HeaderMap& headers, const S& body = {}) {
        auto method = isPost ? System::Net::Http::HttpMethod::get_Post() : System::Net::Http::HttpMethod::get_Get();
        HttpRequestMessage* req = HttpRequestMessage::New_ctor(method, System::Uri::New_ctor(url));
        AddHeaders(req, headers);
        if (isPost) AddPostContent(req, body, headers);
        return req;
    }

    static task_coroutine<HttpResponseMessage*> SendAsync(HttpRequestMessage* req, int timeout = 10) {
        try {
            auto* cts = System::Threading::CancellationTokenSource::New_ctor();
            auto task = get_httpClient()->SendAsync(req, cts->Token);
            cts->CancelAfter(System::TimeSpan::FromSeconds(timeout));
            auto response = co_await task;
            req->Dispose();
            co_return response;
        }
        catch (...) {
            co_return nullptr; 
        }
    }

    template<ResponseContent T, ResponseContent U>
    static HttpResponse<T, U> HandleResponse(HttpResponseMessage* response) {
        if (!response) return HttpResponse<T, U>{false, 408, {}, {}, {}};
        HttpResponse<T, U> responseObj;

        if (!response->IsSuccessStatusCode) responseObj = {false, response->StatusCode.value__, {}, {}, ExtractType<U>(response)};
        else responseObj = {true, response->StatusCode.value__, {}, ExtractType<T>(response), {}};
        
        for (auto [header, value] : DictionaryW(response->Headers->___headers)) {
            responseObj.headers.insert({header, fmt::to_string(fmt::join(ListW<StringW>(value->Values), ","))});
        }
        for (auto [header, value] : DictionaryW(response->Content->Headers->___headers)) {
            responseObj.headers.insert({header, fmt::to_string(fmt::join(ListW<StringW>(value->Values), ","))});
        }

        response->Dispose();
        return responseObj;
    }

    template<ResponseContent T>
    static T ExtractType(HttpResponseMessage* response) {
        response->Content->LoadIntoBufferAsync()->Wait();
        ArrayW<uint8_t> byteArray = response->Content->buffer->ToArray();
        if constexpr(std::is_same_v<T, std::vector<uint8_t>>) {
            std::vector<uint8_t> data;
            data.resize(byteArray.size());
            byteArray.copy_to(data, 0);
            return data;
        }
        std::string string((char*)byteArray->begin(), byteArray.size());
        if constexpr(std::is_same_v<std::decay_t<T>, std::string>) return string;
        else if constexpr(JSONStruct<T>) {
            try { return ReadFromString<T>(string); } 
            catch(JSONException e) { return {}; }
        }
        else if constexpr(JsonContainer<T>) {
            std::string toDeserialise = "{ \"key\": " + string + "}";
            rapidjson::Document document;
            document.Parse(toDeserialise.data());
            if(document.HasParseError()) return {};
            T t;
            rapidjson_macros_auto::Deserialize(t, "key", document);
            return t;
        }
    }
};
