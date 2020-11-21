#include "ApiHandler.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <proxygen/httpserver/RequestHandler.h>
#include <proxygen/httpserver/ResponseBuilder.h>

#include <glog/logging.h>

using boost::property_tree::ptree;


using namespace proxygen;

namespace EchoService {

ApiHandler::ApiHandler(std::shared_ptr<RedirectProcessor> processor) 
: processor(processor)
{

}

void ApiHandler::UpdateMapping(std::string body)
{
  LOG(INFO) << "[API] update mapping request body: " << body << '\n';

  ptree pt;
  std::istringstream is(body);
  boost::property_tree::read_json(is, pt);

  auto info = processor->Load(pt.get<std::string>("newUrl"));
  if(!info)
  {
    ResponseBuilder(downstream_)
      .status(400, "mapping not found")
      .send();
      return;
  }

  {
    auto exp_prop = pt.get_optional<std::string>("expired_on");
    if(exp_prop)
      info->info.expired_on = RedirectProcessor::TimePointFromString(*exp_prop);
  }

  {
    auto orig_url = pt.get_optional<std::string>("orig_url");
    if(orig_url)
      info->info.orig_url = *orig_url;
  }

  {
    auto sms_uuid = pt.get_optional<std::string>("sms_uuid");
    if(sms_uuid)
      info->info.sms_uuid = *sms_uuid;
  }

  info->info.whiteList.clear();
  for(auto f: pt.get_child("whitelist"))
    info->info.whiteList.insert(f.second.get_value<std::string>());
  
  processor->UpdateRedirectionInfo(*info);
  
  ResponseBuilder(downstream_)
    .status(200, "OK")
    .send();
}

void ApiHandler::DeleteMapping(std::string body)
{
  LOG(INFO) << "[API] delete mapping request body: " << body << '\n';

  ptree pt;
  std::istringstream is(body);
  boost::property_tree::read_json(is, pt);
  bool deleted = processor->DeleteRedirectionInfo(pt.get<std::string>("newUrl"));
  
  LOG(INFO) << "[API] mapping deleted: " << deleted << '\n';

  if(deleted)
    ResponseBuilder(downstream_)
      .status(200, "OK")
      .send();
  else
    ResponseBuilder(downstream_)
      .status(400, "mapping not found")
      .send();
}

void ApiHandler::DeleteDomain(std::string body)
{
  LOG(INFO) << "[API] delete domain request body: " << body << '\n';

  ptree pt;
  std::istringstream is(body);
  boost::property_tree::read_json(is, pt);
  processor->DeleteDomain(pt.get<std::string>("domain"));

  LOG(INFO) << "[API] domain deleted: " << body << '\n';

  ResponseBuilder(downstream_)
    .status(200, "OK")
    .send();
}

void ApiHandler::UpdateDomain(std::string body)
{
  ptree pt;
  std::istringstream is(body);
  boost::property_tree::read_json(is, pt);

  LOG(INFO) << "[API] update domain request body: " << body << '\n';

  DomainInfo info = {};
  info.url = pt.get<std::string>("domain");
  info.expired_on = RedirectProcessor::TimePointFromString(pt.get<std::string>("expired_on"));
  info.default_url = pt.get<std::string>("default_url");
  info.no_url_failover_url = pt.get<std::string>("no_url_failover_url");
  info.expired_url_failover_url = pt.get<std::string>("expired_url_failover_url");
  info.out_of_reach_failover_url = pt.get<std::string>("out_of_reach_failover_url");
  
  for(auto f: pt.get_child("whitelist"))
    info.whitelist.insert(f.second.get_value<std::string>());

  processor->UpdateDomain(info);

  LOG(INFO) << "[API] update domain request complete: ";  

  ResponseBuilder(downstream_)
    .status(200, "OK")
    .send();
}

void ApiHandler::Create(std::string body)
{
  ptree pt;
  std::istringstream is(body);
  boost::property_tree::read_json(is, pt);

  auto domain = pt.get<std::string>("domain");
  RedirectInfo info = {};
  info.created_on = processor->TimePointFromString(pt.get<std::string>("created_on"));
  info.expired_on = processor->TimePointFromString(pt.get<std::string>("expired_on"));
  info.orig_url = pt.get<std::string>("orig_url");
  info.sms_uuid = pt.get<std::string>("sms_uuid");

  for(auto f: pt.get_child("whitelist"))
    info.whiteList.insert(f.second.get_value<std::string>());
  
  LOG(INFO) << "[API] Create request body: " << body << '\n';
  auto newUrl = processor->StoreRedirection(domain, info);
  LOG(INFO) << "[API] new url created:" << newUrl.newUrl 
              << " original: " << info.orig_url 
              << " collisions: " << newUrl.collisions 
              << '\n';

  if(newUrl.collisions > 100)
    LOG(WARNING) << "[NEWURL] collisions for domain: " << domain
                 << " : " << newUrl.collisions
                 << " newUrl: " << newUrl.newUrl;

  ptree resJson;
  resJson.put("newUrl", newUrl.newUrl);
  resJson.put("collisions", newUrl.collisions);
  std::stringstream ss;
  boost::property_tree::json_parser::write_json(ss, resJson);

  ResponseBuilder(downstream_)
    .status(200, "OK")
    .body(ss.str())
    .send();
}

void ApiHandler::Remap()
{
  auto url = message->getQueryParam("url");
  if(url.empty())
  {
    ResponseBuilder builder(downstream_);
    builder.status(405, "no url parameter defined");
    builder.send();
    return;
  }else
  {
    ResponseBuilder builder(downstream_);
    
    auto redirect_info = processor->Load(url);
    if(!redirect_info)
      throw std::runtime_error("url not found");

    builder.status(200, "OK");

    ptree pt;
    pt.put("orig_url", redirect_info->info.orig_url);
    pt.put("created_on", RedirectProcessor::ToTimeStampString(redirect_info->info.created_on));
    pt.put("expired_on", RedirectProcessor::ToTimeStampString(redirect_info->info.expired_on));
    pt.put("sms_uuid", redirect_info->info.sms_uuid);

    ptree whitelist;
    for(auto wl : redirect_info->info.whiteList)
    {
      ptree arrayElement;
      arrayElement.put_value(wl);
      whitelist.push_back(std::make_pair("", arrayElement));
    }
    
    pt.put_child("whitelist", whitelist);


    std::stringstream ss;
    boost::property_tree::json_parser::write_json(ss, pt);

    builder.body(ss.str());
    builder.send();
  }
}

void ApiHandler::onRequest(std::unique_ptr<HTTPMessage> req) noexcept 
{
  try
  {
    this->message = std::move(req);

    auto path = message->getPath();
    VLOG(1) << "[API]" << path;
    if(path == "/api/remap")
      this->Remap();
    else if(path == "/api/create")
    {
      waiting_post = true;
      return; // POST processing
    }
    else if(path == "/api/update_domain")
    {
      waiting_post = true;
      return; // POST processing
    }
    else if(path == "/api/delete_domain")
    {
      waiting_post = true;
      return; // POST processing
    }
    else if(path == "/api/update_redirect")
    {
      waiting_post = true;
      return; // POST processing
    }
    else if(path == "/api/delete_redirect")
    {
      waiting_post = true;
      return; // POST processing
    }
    else
        throw std::runtime_error("unknown api path: " + path);
  }
  catch(const std::exception& e)
  {
    LOG(ERROR) << e.what() << '\n';

    ResponseBuilder builder(downstream_);
    builder.status(400, e.what());
    builder.send();
  }
}

void ApiHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept 
{
  auto post_body = body->moveToFbString().toStdString();
  auto path = message->getPath();
  waiting_post = false;

  try
  {
    if(path == "/api/create")
      Create(post_body);
    else if(path == "/api/update_domain")
      UpdateDomain(post_body);
    else if(path == "/api/delete_domain")
      DeleteDomain(post_body);
    else if(path == "/api/update_redirect")
      UpdateMapping(post_body);
    else if(path == "/api/delete_redirect")
      DeleteMapping(post_body);
    else
      throw std::runtime_error("unknown api:" + path);
  }
  catch(const std::exception& e)
  {
    LOG(ERROR) << e.what() << '\n';
    ResponseBuilder(downstream_)
      .status(400, "Wrong url")
      .send();
  }
}

void ApiHandler::onEOM() noexcept 
{
  ResponseBuilder builder(downstream_);
  if(waiting_post)
  {
    builder.status(400, "not processed, need body");
    builder.send();
  }
  
  ResponseBuilder(downstream_)
      .sendWithEOM();
}

void ApiHandler::onUpgrade(UpgradeProtocol /*protocol*/) noexcept {
  // handler doesn't support upgrades
}

void ApiHandler::requestComplete() noexcept {
  delete this;
}

void ApiHandler::onError(ProxygenError /*err*/) noexcept {
  delete this;
}
} // namespace EchoService
