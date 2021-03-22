#pragma once

#include <folly/Memory.h>
#include <proxygen/httpserver/RequestHandler.h>
#include "lib/RedirectProcessor.h"
#include "lib/Reporting.h"

namespace proxygen {
class ResponseHandler;
}

namespace EchoService {

class RedirectHandler : public proxygen::RequestHandler {
 public:
  explicit RedirectHandler(std::shared_ptr<RedirectProcessor> processor,
                std::shared_ptr<Reporting<ClickInfo>> clickReporting);

  void onRequest(
      std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;

  void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;
  void onEOM() noexcept override;
  void onUpgrade(proxygen::UpgradeProtocol proto) noexcept override;
  void requestComplete() noexcept override;
  void onError(proxygen::ProxygenError err) noexcept override;

 private:
  std::shared_ptr<RedirectProcessor> processor;
  std::unique_ptr<proxygen::HTTPMessage> message;
  std::shared_ptr<Reporting<ClickInfo>> clickReporting;
};

} // namespace EchoService
