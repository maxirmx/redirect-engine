#include <iostream>
#include <pqxx/pqxx>
#include "sys/time.h"

#include <chrono>
#include "../../lib/RedirectProcessor.h"
#include "../../lib/Reporting.h"
#include "../../lib/PBULKBackendClicks.h"
#include <chrono>


int main(int argc, char* argv[]) 
{
    auto gi = GeoIP_open("/mnt/c/Development/GeoIPDataBases/GeoIP.dat", GEOIP_MEMORY_CACHE); 
    if(gi == 0)
        throw std::runtime_error("can't load geoip database");

    gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

    LOG(INFO) << "Started";

    std::string connectionInfo = "user=postgres host=localhost port=5432 dbname=urls_remap password=changeme";
    Reporting<ClickInfo> clicksReporting(std::make_unique<PBULKBackendClicks>(connectionInfo), 500);

    for(int i = 0 ; i < 200; ++i)
    {
        ClickInfo sampleClick = {};
        sampleClick.clicked_on = std::chrono::system_clock::now();
        sampleClick.clientIP = "127.0.0.1";
        sampleClick.newUrl = "http://127.0.0.1:12000/xxxxxx";
        sampleClick.replaced_url = "http://google.com";
        sampleClick.sms_uuid = "123";

        clicksReporting.Report(sampleClick);
    }

    RedirectProcessor p(connectionInfo, gi);

    DomainInfo domain;
    domain.created_on = std::chrono::system_clock::now();
    domain.expired_on = domain.created_on + std::chrono::hours(24 * 365);
    domain.default_url = "http://boost.org";
    domain.expired_url_failover_url = "http://rsdn.ru";
    domain.no_url_failover_url = "http://microsoft.com";
    domain.out_of_reach_failover_url = "http://google.com";
    domain.url = "localhost:12000";
    
    domain.whitelist = std::unordered_set<std::string> { "RU", "TW " };


    p.UpdateDomain(domain);

    auto c1 = p.GetCountryFromAddress("127.0.0.1");
    auto c2 = p.GetCountryFromAddress("77.88.55.117");
    
    auto res = p.Load("http://localhost:12000/aAAAAA");
    if(res)
        std::cout << res->info.orig_url;

    auto now = std::chrono::system_clock::now();
    
    RedirectInfo sampleInfo;
    sampleInfo.orig_url = "https://www.boost.org:423/doc/libs/1_74_0/libs/regex/doc/html/index.html";
    sampleInfo.sms_uuid = "123";
    sampleInfo.created_on = now;
    sampleInfo.expired_on = now;

    auto stored = p.StoreRedirection("localhost:12000", sampleInfo);
    auto redInfo = p.Load(stored.newUrl);
    p.UpdateRedirectionInfo(*redInfo);
    p.UpdateDomain(redInfo->domain);
    std::cout << stored.newUrl << std::endl;

    p.DeleteDomain("localhost:12000");
    GeoIP_delete(gi);
}
