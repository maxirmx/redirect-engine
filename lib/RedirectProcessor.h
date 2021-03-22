// Ph2 completed

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
#include "RWLock.h"

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
    bool use_async_store = false;

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

    void DeleteDomain(std::string domain)
    {
        std::lock_guard lock(this->store_redirect_mutex);
        pqxx::work txn{*this->store_redirects_connection};
        int urlID = 0;
        pqxx::result res { txn.exec_params("SELECT url_id FROM url WHERE url = $1;", domain ) };
        if(res.empty())
            throw std::runtime_error("Can't find domain:" + domain);
        urlID = res[0][0].as<int>();

        txn.exec_params("DELETE FROM mapping WHERE new_url LIKE $1", "http://" + domain + "%" );
        txn.exec_params("DELETE FROM mappingwl WHERE new_url LIKE $1", "http://" + domain + "%" );
        txn.exec_params("DELETE FROM mappingre WHERE new_url LIKE $1", "http://" + domain + "%" );
        txn.exec_params("DELETE FROM mappingag WHERE new_url LIKE $1", "http://" + domain + "%" );
        txn.exec_params("DELETE FROM url_whitelist WHERE url_id = $1;", urlID );
        txn.exec_params("DELETE FROM url_referrers WHERE url_id = $1;", urlID );
        txn.exec_params("DELETE FROM url_agents WHERE url_id = $1;", urlID );
        txn.exec_params("DELETE FROM url WHERE url = $1;", domain );
        

        txn.commit();

        LoadCompletePG();
        LoadDomains();
    }

    void UpdateDomain(DomainInfo domainInfo)
    {
        std::lock_guard lock(this->store_redirect_mutex);
        pqxx::work txn{*this->store_redirects_connection};
        
        int urlID = 0;
        pqxx::result res { txn.exec_params("SELECT url_id FROM url WHERE url = $1;", domainInfo.url ) };
        if(res.empty())
        {
            auto max_urlid_fdb = txn.exec("SELECT MAX( url_id ) FROM url")[0][0];
            if(!max_urlid_fdb.is_null())
                urlID = max_urlid_fdb.as<int>() + 1;

            txn.exec_params("INSERT INTO url(url_id, url, created_on, expired_on, default_url, no_url_failover_url, expired_url_failover_url, out_of_reach_failover_url) VALUES ($1, $2, $3, $4, $5, $6, $7, $8)", 
                        urlID, domainInfo.url,  ToTimeStampString(std::chrono::system_clock::now()), ToTimeStampString(domainInfo.expired_on),
                        domainInfo.default_url, domainInfo.no_url_failover_url, domainInfo.expired_url_failover_url, domainInfo.out_of_reach_failover_url);

            LOG(INFO) << "[API] Created new DOMAIN" 
                        << " url: " << domainInfo.url
                        << " id: " << urlID;
        }
        else
        {
            urlID = res[0][0].as<int>();
            txn.exec_params("UPDATE url	SET expired_on=$2, default_url=$3, no_url_failover_url=$4, expired_url_failover_url=$5, out_of_reach_failover_url=$6 WHERE url_id = $1",
                urlID, ToTimeStampString(domainInfo.expired_on), domainInfo.default_url, domainInfo.no_url_failover_url, domainInfo.expired_url_failover_url, domainInfo.out_of_reach_failover_url);
        }
        

        UpdateDomainWhiteList(txn, urlID, domainInfo.whiteList);
        UpdateDomainReferrers(txn, urlID, domainInfo.refererList);
        UpdateDomainAgents(txn, urlID, domainInfo.agentList);
        txn.commit();

        LoadDomains();
    }
    

    std::string GetCountryFromAddress(const std::string &address)
    {
        auto res = GeoIP_country_code_by_addr(geoIP, address.c_str());
        if(res == 0)
            return std::string();

        return std::string(res);
    }

    bool DeleteRedirectionInfo(std::string newUrl)
    {
        std::lock_guard lock(this->store_redirect_mutex);
        pqxx::work txn { *this->store_redirects_connection };
        txn.exec_params("DELETE FROM mappingwl WHERE new_url = $1", newUrl);
        txn.exec_params("DELETE FROM mappingre WHERE new_url = $1", newUrl);
        txn.exec_params("DELETE FROM mappingag WHERE new_url = $1", newUrl);
        txn.exec_params("DELETE FROM mapping WHERE new_url = $1", newUrl);
        txn.commit();
        return storage->Remove(newUrl);
    }

    void UpdateRedirectionInfo(const CompleteRedirectInfo cri)
    {
        std::lock_guard lock(this->store_redirect_mutex);
        pqxx::work txn { *this->store_redirects_connection };
        txn.exec_params("UPDATE mapping SET orig_url=$2, expired_on=$3, sms_uuid=$4	WHERE new_url=$1", 
                cri.newUrl, cri.info.orig_url, ToTimeStampString(cri.info.expired_on), cri.info.sms_uuid );
        
        txn.exec_params("DELETE FROM mappingwl WHERE new_url = $1", cri.newUrl);
        for(auto wl : cri.info.whiteList)
            txn.exec_params("INSERT INTO mappingwl(new_url, country_iso) VALUES ($1, $2)", cri.newUrl, wl);

        txn.exec_params("DELETE FROM mappingre WHERE new_url = $1", cri.newUrl);
        for(auto re : cri.info.refererList)
            txn.exec_params("INSERT INTO mappingre(new_url, referrer) VALUES ($1, $2)", cri.newUrl, re);

        txn.exec_params("DELETE FROM mappingag WHERE new_url = $1", cri.newUrl);
        for(auto ag : cri.info.agentList)
            txn.exec_params("INSERT INTO mappingag(new_url, user_agent) VALUES ($1, $2)", cri.newUrl, ag);

        txn.commit();
        storage->StoreInfo(cri.newUrl, cri.info);
    }

    NewUrlGenerationInfo StoreRedirection(const std::string targetDomain, const RedirectInfo info)
    {
        std::lock_guard lock(this->store_redirect_mutex);
        pqxx::work txn{*this->store_redirects_connection};

        if(use_async_store)
            txn.exec("BEGIN; SET synchronous_commit TO OFF;");

        pqxx::result url_records { txn.exec_params("SELECT url_id FROM url WHERE url = $1;", targetDomain ) };
        if(url_records.empty())
            throw std::runtime_error("URL not found: " + targetDomain );

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

        for(auto wl : info.whiteList)
            txn.exec_params("INSERT INTO mappingwl(new_url, country_iso) VALUES ($1, $2)", newUrl, wl);

        for(auto re : info.refererList)
            txn.exec_params("INSERT INTO mappingre(new_url, referrer) VALUES ($1, $2)", newUrl, re);

        for(auto ag : info.agentList)
            txn.exec_params("INSERT INTO mappingag(new_url, user_agent) VALUES ($1, $2)", newUrl, ag);

        txn.commit();
        storage->StoreInfo(newUrl, info);

        NewUrlGenerationInfo res = {};
        res.newUrl = newUrl;
        res.collisions = collisions;
        return res;
    }    

    std::optional<CompleteRedirectInfo> Load(std::string key)
    {
        RWReadGuard domainsReadGuard(&multipleReadersOneWriterLock);

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
        res.newUrl = key;
        return res;
    }

    inline static std::string ToTimeStampString(std::chrono::system_clock::time_point point)
    {
        const std::time_t time = std::chrono::system_clock::to_time_t(point);
        auto info = localtime(&time);

        char buffer[80];
        strftime(buffer, sizeof(buffer),"%Y-%m-%d %H:%M:%S",info);
        return std::string(buffer);
    }

    inline static std::chrono::system_clock::time_point TimePointFromString(const std::string& s)
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
        RWReadGuard domainsLock(&multipleReadersOneWriterLock);

        auto r = this->domains.find(domain);
        if(r == this->domains.end())
            return std::nullopt;
        return r->second;
    }
private:

    void LoadCompletePG()
    {
        RWWriteGuard _lock(&this->multipleReadersOneWriterLock);
        storage->Clear();
        std::chrono::steady_clock::time_point load_begin = std::chrono::steady_clock::now();

        pqxx::connection C(postgres);

        std::lock_guard lock(this->store_redirect_mutex);
        pqxx::work W(C);
        pqxx::result records { W.exec("SELECT orig_url, new_url, created_on, expired_on, sms_uuid FROM mapping") };

        for (auto row: records)
        {
            RedirectInfo info = {};

            auto new_url = row[1].as<std::string>();

            info.orig_url = row[0].as<std::string>();
            info.created_on = TimePointFromString(row[2].as<std::string>());
            info.expired_on = TimePointFromString(row[3].as<std::string>());
            info.sms_uuid = row[4].as<std::string>();

            storage->StoreInfo(new_url, info);
        }

        std::chrono::steady_clock::time_point load_end = std::chrono::steady_clock::now();

        LoadInitialURLWhiteList(W);
        LoadInitialURLReferrers(W);
        LoadInitialURLAgents(W);

        std::cout << "Loading tree from db = " << std::chrono::duration_cast<std::chrono::milliseconds>(load_end - load_begin).count() << "[ms]" << std::endl;
        std::cout << "Count: " << records.size() << '\n';
    }

    void LoadInitialURLWhiteList(pqxx::work &trx)
    {
        pqxx::result map_wl { trx.exec("SELECT new_url, country_iso from mappingwl order by new_url") };
        std::optional<std::string> curURL = std::nullopt;
        std::unordered_set<std::string> curWL;
        for(auto r : map_wl)
        {
            auto newUrl = r[0].as<std::string>();
            auto wl = r[1].as<std::string>();

            if(curURL && *curURL != newUrl)
            {
                auto ri = this->storage->Load(*curURL);
                if(ri)
                {
                    ri->whiteList = curWL;
                    this->storage->StoreInfo(*curURL, *ri);
                }

                curWL.clear();
            }

            curURL = newUrl;
            curWL.insert(wl);
        }

        if(curURL && !curWL.empty())
        {
            auto ri = this->storage->Load(*curURL);
            if(ri)
            {
                ri->whiteList = curWL;
                this->storage->StoreInfo(*curURL, *ri);
                curWL.clear();
            }
        }
    }

    void LoadInitialURLReferrers(pqxx::work &trx)
    {
        pqxx::result map_re { trx.exec("SELECT new_url, referrer from mappingre order by new_url") };
        std::optional<std::string> curURL = std::nullopt;
        std::unordered_set<std::string> curRE;
        for(auto r : map_re)
        {
            auto newUrl = r[0].as<std::string>();
            auto re = r[1].as<std::string>();

            if(curURL && *curURL != newUrl)
            {
                auto ri = this->storage->Load(*curURL);
                if(ri)
                {
                    ri->refererList = curRE;
                    this->storage->StoreInfo(*curURL, *ri);
                }

                curRE.clear();
            }

            curURL = newUrl;
            curRE.insert(re);
        }

        if(curURL && !curRE.empty())
        {
            auto ri = this->storage->Load(*curURL);
            if(ri)
            {
                ri->refererList = curRE;
                this->storage->StoreInfo(*curURL, *ri);
                curRE.clear();
            }
        }
    }

    void LoadInitialURLAgents(pqxx::work &trx)
    {
        pqxx::result map_ag { trx.exec("SELECT new_url, user_agent from mappingag order by new_url") };
        std::optional<std::string> curURL = std::nullopt;
        std::unordered_set<std::string> curAG;
        for(auto r : map_ag)
        {
            auto newUrl = r[0].as<std::string>();
            auto ag = r[1].as<std::string>();

            if(curURL && *curURL != newUrl)
            {
                auto ri = this->storage->Load(*curURL);
                if(ri)
                {
                    ri->agentList = curAG;
                    this->storage->StoreInfo(*curURL, *ri);
                }

                curAG.clear();
            }

            curURL = newUrl;
            curAG.insert(ag);
        }

        if(curURL && !curAG.empty())
        {
            auto ri = this->storage->Load(*curURL);
            if(ri)
            {
                ri->agentList = curAG;
                this->storage->StoreInfo(*curURL, *ri);
                curAG.clear();
            }
        }
    }

    void UpdateDomainWhiteList(pqxx::work &txn, int urlId, const std::unordered_set<std::string> &countrycodes)
    {   
        txn.exec_params("delete FROM url_whitelist WHERE url_id = $1", urlId);
        for(auto c : countrycodes)
            txn.exec_params("INSERT INTO url_whitelist(url_id, country_iso)	VALUES ($1, $2)", urlId, c);
    }

    void UpdateDomainReferrers(pqxx::work &txn, int urlId, const std::unordered_set<std::string> &referrers)
    {   
        txn.exec_params("delete FROM url_referrers WHERE url_id = $1", urlId);
        for(auto c : referrers)
            txn.exec_params("INSERT INTO url_referrers(url_id, referrer) VALUES ($1, $2)", urlId, c);
    }

    void UpdateDomainAgents(pqxx::work &txn, int urlId, const std::unordered_set<std::string> &agents)
    {   
        txn.exec_params("delete FROM url_agents WHERE url_id = $1", urlId);
        for(auto c : agents)
            txn.exec_params("INSERT INTO url_agents(url_id, user_agent) VALUES ($1, $2)", urlId, c);
    }

    void LoadDomains()
    {
        RWWriteGuard domainsReadGuard(&multipleReadersOneWriterLock);

        domains.clear();

        std::chrono::steady_clock::time_point load_begin = std::chrono::steady_clock::now();

        pqxx::connection C(postgres);

        std::lock_guard lock(this->store_redirect_mutex);
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
                info.whiteList.insert(wl[0].as<std::string>());

            pqxx::result re_records { W.exec_params("SELECT referrer FROM url_referrers where url_id = $1", info.url_id) };
            for(auto re : re_records)
                info.refererList.insert(re[0].as<std::string>());

            pqxx::result ag_records { W.exec_params("SELECT user_agent FROM url_agents where url_id = $1", info.url_id) };
            for(auto ag : ag_records)
                info.agentList.insert(ag[0].as<std::string>());

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

    std::string postgres;
    std::unique_ptr<Storage<std::string, RedirectInfo>> storage;

    std::map<std::string, DomainInfo> domains;

    std::shared_ptr<pqxx::connection> store_redirects_connection;
    std::recursive_mutex store_redirect_mutex;
    GeoIP *geoIP;
    Generator g;
    RWLock multipleReadersOneWriterLock;
};