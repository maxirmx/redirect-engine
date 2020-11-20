#pragma once

#include "RedirectInfo.h"
#include "RowsStorage.h"
#include <memory>
#include <boost/regex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <random>
#include <sstream>
#include <glog/logging.h>
#include "GeoIP.h"

struct NewUrlGenerationInfo
{
public:
    std::string newUrl;
    int collisions;
};


typedef std::minstd_rand Generator;

class RedirectProcessor
{
public:
    RedirectProcessor(std::string postgres, GeoIP *geoIP)
    {
        this->postgres = postgres;
        this->geoIP = geoIP;
        this->storage = std::unique_ptr<Storage<std::string, RedirectInfo>>(new Storage<std::string, RedirectInfo>([](std::string k) { return StringKeyCrc32(k); }));\
        this->g.seed(std::time(0));
        this->store_redirects_connection = std::make_shared<pqxx::connection>(postgres);

        LoadCompletePG();
        LoadDomains();
    }

    void AddToWhitelist(std::string url, std::string countrycode)
    {
        std::lock_guard lock(this->store_redirect_mutex);
        pqxx::work txn{*this->store_redirects_connection};

        
        int urlId = CreateDomainIfNotExist(url, txn);
        int count = txn.exec_params1("SELECT count(1) FROM url_whitelist where url_id = $1 and country_iso = $2", urlId, countrycode)[0].as<int>();
        if(count == 0)
        {
            txn.exec_params("INSERT INTO public.url_whitelist(url_id, country_iso)	VALUES ($1, $2)",
                    urlId, countrycode);
        }
        
        txn.commit();
    }

    std::string GetCountryFromAddress(const std::string &address)
    {
        auto res = GeoIP_country_code_by_addr(geoIP, address.c_str());
        if(res == 0)
            return std::string();

        return std::string(res);
    }

    NewUrlGenerationInfo StoreRedirection(std::string targetDomain, RedirectInfo info)
    {
        std::lock_guard lock(this->store_redirect_mutex);
        pqxx::work txn{*this->store_redirects_connection};

        CreateDomainIfNotExist(targetDomain, txn);
        int currentDigits = 6;

        int collisions = 0;
        auto newUrl = GenerateRandomURL(targetDomain, currentDigits);
        while(true)
        {
            auto already_exist = storage->Load(newUrl);
            if(!already_exist)
            {
                int count = txn.exec_params1("SELECT count(1) FROM mapping WHERE new_url = $1;", newUrl)[0].as<int>();
                if(count == 0)
                     break;
            }

            newUrl = GenerateRandomURL(targetDomain, currentDigits);
            collisions++;
            if(collisions > 100)
                currentDigits++;
        }

        txn.exec_params("INSERT INTO mapping(orig_url, new_url, created_on, expired_on, sms_uuid) VALUES ($1, $2, $3, $4, $5);",
                    info.orig_url, newUrl, ToTimeStampString(info.created_on), ToTimeStampString(info.expired_on), info.sms_uuid );
        txn.commit();
        storage->StoreInfo(newUrl, info);

        NewUrlGenerationInfo res = {};
        res.newUrl = newUrl;
        res.collisions = collisions;
        return res;
    }

    void LoadDomains()
    {
        std::chrono::steady_clock::time_point load_begin = std::chrono::steady_clock::now();

        pqxx::connection C(postgres);
        pqxx::work W(C);
        pqxx::result records { W.exec("SELECT url_id, url, created_on, expired_on, default_url, no_url_failover_url, expired_url_failover_url, out_of_reach_failover_url FROM url") };

        for (auto row: records)
        {
            DomainInfo info = {};
            info.url_id = row[0].as<int>();
            info.url = row[1].as<std::string>();
            if(!row[2].is_null())
                info.created_on = TimePointFromString(row[2].as<std::string>());

            if(!row[3].is_null())
                info.expired_on = TimePointFromString(row[3].as<std::string>());

            if(!row[4].is_null())
                info.default_url = row[4].as<std::string>();

            if(!row[5].is_null())
                info.no_url_failover_url = row[5].as<std::string>();

            if(!row[6].is_null())
                info.expired_url_failover_url = row[6].as<std::string>();

            if(!row[7].is_null())
                info.out_of_reach_failover_url = row[7].as<std::string>();

            pqxx::result wl_records { W.exec_params("SELECT country_iso FROM url_whitelist where url_id = $1", info.url_id) };
            for(auto wl : wl_records)
                info.whitelist.insert(wl[0].as<std::string>());

            domains[info.url] = info;
        }

        std::chrono::steady_clock::time_point load_end = std::chrono::steady_clock::now();

        std::cout << "Loading domains: " << std::chrono::duration_cast<std::chrono::milliseconds>(load_end - load_begin).count() << "[ms]" << std::endl;
        std::cout << "Count: " << records.size() << '\n';
    }

    std::string GenerateRandomURL(std::string targetDomain, int length)
    {
        typedef std::uniform_int_distribution<> D;
        std::string res = "http://" + targetDomain + "/";
        
        D n_type(0, 2);
        D a_type('A', 'Z');
        D b_type('a', 'z');
        D c_type('0', '9');

        for(int i = 0; i < length; ++i)
        {
            switch (n_type(g))
            {
            case 0:
                res += (char)a_type(g);
                break;
            case 1:
                res += (char)b_type(g);
                break;
            case 2:
                res += (char)c_type(g);
                break;
            default:
                throw std::runtime_error("rand function failed");
            }
        }

        return res;
    }

    std::optional<CompleteRedirectInfo> Load(std::string key)
    {
        auto info = storage->Load(key);
        if(!info)
            return std::nullopt;

        boost::regex ex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
        boost::cmatch what;
        if(!regex_match(key.c_str(), what, ex))
            throw std::runtime_error("cannot extract domain");
        
        
        auto s_domain = std::string(what[2].first, what[2].second);
        auto port = std::string(what[3].first, what[3].second);
        auto domain = s_domain;
        if(!port.empty())
            domain += ":" + std::string(port);

        auto f_d_i = this->domains.find(domain);
        if(f_d_i == this->domains.end())
            throw std::runtime_error("domain: " + domain + " not loaded or doesn't exist in url table, please restart service");

        CompleteRedirectInfo res;
        res.info = (*info);
        res.domain = f_d_i->second;
        return res;
    }

    void LoadCompletePG()
    {
        std::chrono::steady_clock::time_point load_begin = std::chrono::steady_clock::now();

        pqxx::connection C(postgres);
        pqxx::work W(C);
        pqxx::result records { W.exec("SELECT orig_url, new_url, created_on, expired_on, sms_uuid FROM mapping") };

        for (auto row: records)
        {
            RedirectInfo info;

            auto new_url = row[1].as<std::string>();

            info.orig_url = row[0].as<std::string>();
            info.created_on = TimePointFromString(row[2].as<std::string>());
            info.expired_on = TimePointFromString(row[3].as<std::string>());
            info.sms_uuid = row[4].as<std::string>();

            storage->StoreInfo(new_url, info);
        }

        std::chrono::steady_clock::time_point load_end = std::chrono::steady_clock::now();

        std::cout << "Loading tree from db = " << std::chrono::duration_cast<std::chrono::milliseconds>(load_end - load_begin).count() << "[ms]" << std::endl;
        std::cout << "Count: " << records.size() << '\n';
    }

    inline static std::string ToTimeStampString(std::chrono::system_clock::time_point point)
    {
        const std::time_t time = std::chrono::system_clock::to_time_t(point);
        auto info = localtime(&time);

        char buffer[80];
        strftime(buffer, sizeof(buffer),"%Y-%m-%d %H:%M:%S",info);
        return std::string(buffer);
    }

    std::chrono::system_clock::time_point TimePointFromString(const std::string& s)
    {
        using namespace boost::posix_time;
        using namespace std::chrono;

        const ptime ts = time_from_string(s);
        auto seconds = to_time_t(ts);
        time_duration td = ts.time_of_day();
        auto microseconds = td.fractional_seconds();
        auto d = std::chrono::seconds{seconds} + std::chrono::microseconds{microseconds};
        system_clock::time_point tp{duration_cast<system_clock::duration>(d)};
        return tp;
    }

    std::optional<DomainInfo> GetDomain(std::string domain)
    {
        auto r = this->domains.find(domain);
        if(r == this->domains.end())
            return std::nullopt;
        return r->second;
    }
private:
    int CreateDomainIfNotExist(std::string domain, pqxx::work &txn)
    {
        int count = txn.exec_params1("SELECT count(1) FROM url WHERE url = $1;", domain)[0].as<int>();
        if(count > 0)
            return txn.exec_params1("SELECT url_id FROM url WHERE url = $1;", domain)[0].as<int>();

        int nextIndex = 0;
        auto max_urlid_fdb = txn.exec("SELECT MAX( url_id ) FROM url")[0][0];
        if(!max_urlid_fdb.is_null())
            nextIndex = max_urlid_fdb.as<int>() + 1;

        auto now = std::chrono::system_clock::now();
        auto t_str = ToTimeStampString(now);
        txn.exec_params("INSERT INTO url(url_id, url, created_on) VALUES ($1, $2, $3)", nextIndex, domain,  t_str);

        LOG(INFO) << "[API] Created new DOMAIN" 
                    << " url: " << domain
                    << " id: " << nextIndex;

        return nextIndex;
    }

    std::string postgres;
    std::unique_ptr<Storage<std::string, RedirectInfo>> storage;

    std::map<std::string, DomainInfo> domains;

    std::shared_ptr<pqxx::connection> store_redirects_connection;
    std::mutex store_redirect_mutex;
    GeoIP *geoIP;
    Generator g;
};