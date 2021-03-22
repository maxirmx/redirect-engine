#include "RedirectHandler.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <glog/logging.h>

using boost::property_tree::ptree;

using namespace proxygen;

namespace EchoService {

RedirectHandler::RedirectHandler(std::shared_ptr<RedirectProcessor> processor, std::shared_ptr<Reporting<ClickInfo>> clickReporting) 
: processor(processor), clickReporting(clickReporting)
{

}

const std::string ToString(RedirectType type)
{
  switch(type)
  {
    case RedirectType::SUCCESS:
      return "SUCCESS";
    case RedirectType::EXPIRED:
      return "EXPIRED";
    case RedirectType::NOT_FOUND:
      return "NOT_FOUND";
    case RedirectType::NOT_IN_WHITELIST:
      return "NOT_IN_WHITELIST";
    case RedirectType::NO_REFERRER:
      return "NO_REFERRER";
    case RedirectType::NO_USER_AGENT:
      return "NO_USER_AGENT";
  }

  return "UNKNOWN";
}

void RedirectHandler::onRequest(std::unique_ptr<HTTPMessage> req) noexcept 
{
    this->message = std::move(req);
    auto path = message->getPath();
    auto headers = message->getHeaders();
    
    std::map<std::string, std::string> dict;
    std::string host = headers.getSingleOrEmpty("Host");

    ClickInfo clickInfo = {};
    clickInfo.clicked_on = std::chrono::system_clock::now();
    clickInfo.clientIP = message->getClientIP();
    clickInfo.CountryCode = processor->GetCountryFromAddress(clickInfo.clientIP);

    clickInfo.user_agent = headers.getSingleOrEmpty("User-Agent");

    size_t n = clickInfo.user_agent.find_first_of('/');
    if (n == std::string::npos) 
        clickInfo.agent = clickInfo.user_agent;
    else
        clickInfo.agent = clickInfo.user_agent.substr(0, n);
 
    clickInfo.referer = headers.getSingleOrEmpty("Referer");


    if(host.empty())
    {
        LOG(ERROR) << "[REDIRECT] no Host in header: ";
      
        ResponseBuilder(downstream_)
          .status(400, "no Host in header")
          .send();
      return;
    }

    std::string newUrl = "http://" + host + path;
    clickInfo.newUrl = newUrl;
    auto redirectInfo = processor->Load(newUrl);
    std::string redirectedUrl;
    if(!redirectInfo)
    {
      clickInfo.Type = RedirectType::NOT_FOUND;

      auto domainInfo = processor->GetDomain(host);
      LOG(ERROR) << "[REDIRECT] redirect not found for: " + newUrl;
      if(domainInfo == std::nullopt)
      {
        LOG(ERROR) << "[REDIRECT] domain not found: " + host;
        ResponseBuilder(downstream_)
            .status(400, "redirect not found")
            .send();
        return;
      }else
      {
        redirectedUrl = domainInfo->no_url_failover_url;
        if(redirectedUrl.empty())
          redirectedUrl = domainInfo->default_url;
      }
    }else
    {
      clickInfo.sms_uuid = redirectInfo->info.sms_uuid;

      if(redirectInfo->info.expired_on < clickInfo.clicked_on || redirectInfo->domain.expired_on < clickInfo.clicked_on)
      {
        redirectedUrl = redirectInfo->domain.expired_url_failover_url;
        clickInfo.Type = RedirectType::EXPIRED;
      }
      else
      {
        clickInfo.Type = RedirectType::SUCCESS;

        if (!redirectInfo->domain.whiteList.empty()
          && redirectInfo->domain.whiteList.find(clickInfo.CountryCode) == redirectInfo->domain.whiteList.end())
            clickInfo.Type = RedirectType::NOT_IN_WHITELIST;

        if (!redirectInfo->info.whiteList.empty()
          && redirectInfo->info.whiteList.find(clickInfo.CountryCode) == redirectInfo->info.whiteList.end())
          clickInfo.Type = RedirectType::NOT_IN_WHITELIST;

        if (!redirectInfo->domain.referrers.empty()
          && redirectInfo->domain.referrers.find(clickInfo.referer) == redirectInfo->domain.referrers.end())
            clickInfo.Type = RedirectType::NO_REFERRER;

        if (!redirectInfo->info.referrers.empty()
          && redirectInfo->info.referrers.find(clickInfo.referer) == redirectInfo->info.referrers.end())
          clickInfo.Type = RedirectType::NO_REFERRER;

        if (!redirectInfo->domain.agents.empty()
          && redirectInfo->domain.agents.find(clickInfo.agent) == redirectInfo->domain.agents.end())
            clickInfo.Type = RedirectType::NO_USER_AGENT;
            
        if (!redirectInfo->info.agents.empty()
          && redirectInfo->info.agents.find(clickInfo.agent) == redirectInfo->info.agents.end())
          clickInfo.Type = RedirectType::NO_USER_AGENT;


        redirectedUrl = clickInfo.Type == RedirectType::SUCCESS ?
                        redirectInfo->info.orig_url:
                        redirectInfo->domain.out_of_reach_failover_url;          
          
      }
    }

    if(redirectedUrl.empty())
    {
      LOG(ERROR) << "[REDIRECT] redirect default_url or no_url_failover_url or expired_url_failover_url or out_of_reach_failover_url is empty: " + host;

      ResponseBuilder(downstream_)
          .status(400, "redirect not found")
          .send();
      return;          
    }

    clickInfo.replaced_url = redirectedUrl;
    clickReporting->Report(clickInfo);

    
    LOG(INFO) << "[REDIRECT] :"
          + ToString(clickInfo.Type)
          + " newUrl: " + newUrl 
          + " origURL: " + (redirectInfo ? redirectInfo->info.orig_url : "[WRONG]") 
          + " Country: " + clickInfo.CountryCode
          + " IP: " + clickInfo.clientIP
          + " Referer: " + clickInfo.referer
          + " User-Agent: " + clickInfo.user_agent
          + " User Agent TRNC: " + clickInfo.agent;
    

    ResponseBuilder builder(downstream_);
    builder.status(301, "Moved Permanently");
    builder.header("location", redirectedUrl);
    builder.header("cache-control", "private, max-age=90");
    builder.header("content-security-policy", "referrer always");
    builder.header("referrer-policy","unsafe-url");
    
    builder.send();
}

void RedirectHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept 
{}

void RedirectHandler::onEOM() noexcept {
  ResponseBuilder(downstream_).sendWithEOM();
}

void RedirectHandler::onUpgrade(UpgradeProtocol /*protocol*/) noexcept {
  // handler doesn't support upgrades
}

void RedirectHandler::requestComplete() noexcept {
  delete this;
}

void RedirectHandler::onError(ProxygenError /*err*/) noexcept {
  delete this;
}
}
