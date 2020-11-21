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
      }else
      {
        bool blockByCountry = false;

        if(!redirectInfo->domain.whitelist.empty()
          && redirectInfo->domain.whitelist.find(clickInfo.CountryCode) == redirectInfo->domain.whitelist.end())
            blockByCountry = true;

        if(!redirectInfo->info.whiteList.empty()
          && redirectInfo->info.whiteList.find(clickInfo.CountryCode) == redirectInfo->info.whiteList.end())
          blockByCountry = true;

        if(blockByCountry)
        {
          redirectedUrl = redirectInfo->domain.out_of_reach_failover_url;          
          clickInfo.Type = RedirectType::NOT_IN_WHITELIST;
        }else
        {
          redirectedUrl = redirectInfo->info.orig_url;          
          clickInfo.Type = RedirectType::SUCCESS;
        }
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
        + " origURL: " + redirectInfo->info.orig_url
        + " Country: " + clickInfo.CountryCode
        + " IP: " + clickInfo.clientIP;

    ResponseBuilder builder(downstream_);
    builder.status(307, "Moved Permanently");
    builder.header("Location", redirectedUrl);
    builder.header("sft", "urlfh");
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
