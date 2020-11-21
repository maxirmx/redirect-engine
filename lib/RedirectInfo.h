#pragma once

#include <string>
#include <pqxx/pqxx>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <chrono>
#include <unordered_set>


struct RedirectInfo
{
public:
    std::string orig_url;
    std::chrono::system_clock::time_point created_on;
    std::chrono::system_clock::time_point expired_on;
    std::string sms_uuid;
    std::unordered_set<std::string> whiteList;
};

struct DomainInfo
{
public:
    std::string url;
    int url_id;
    std::chrono::system_clock::time_point created_on;
    std::chrono::system_clock::time_point expired_on;

    std::string default_url;
    std::string no_url_failover_url;
    std::string expired_url_failover_url;
    std::string out_of_reach_failover_url;

    std::unordered_set<std::string> whitelist;
};

struct CompleteRedirectInfo
{
public:
    RedirectInfo info;
    DomainInfo domain;
    std::string newUrl;
};

enum RedirectType
{
    SUCCESS = 0,
    EXPIRED = 1,
    NOT_FOUND = 2,
    NOT_IN_WHITELIST = 3,
};

struct ClickInfo
{
public:
    std::string newUrl;
    std::string replaced_url;
    std::chrono::system_clock::time_point clicked_on;
    std::string clientIP;
    std::string sms_uuid;
    std::string CountryCode;
    RedirectType Type;
};