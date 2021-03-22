#pragma once

#include <folly/Memory.h>
#include <proxygen/httpserver/RequestHandler.h>
#include "lib/RedirectProcessor.h"

namespace proxygen {
class ResponseHandler;
}

namespace EchoService {

class ApiHandler : public proxygen::RequestHandler {
 public:
  explicit ApiHandler(std::shared_ptr<RedirectProcessor> processor);
  void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;
  void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
  void onEOM() noexcept override;
  void onUpgrade(proxygen::UpgradeProtocol proto) noexcept override;
  void requestComplete() noexcept override;
  void onError(proxygen::ProxygenError err) noexcept override;

protected:
  void Remap();
  void Create(std::string body);
  void UpdateDomain(std::string body);
  void DeleteDomain(std::string body);
  void UpdateMapping(std::string body);
  void DeleteMapping(std::string body);

 private:
  std::shared_ptr<RedirectProcessor> processor;
  std::unique_ptr<proxygen::HTTPMessage> message;

  bool waiting_post = false;
};

} // namespace EchoService
