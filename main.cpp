/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <folly/Memory.h>
#include <folly/io/async/EventBaseManager.h>
#include <folly/portability/GFlags.h>
#include <folly/portability/Unistd.h>
#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>

#include "ApiHandler.h"
#include "RedirectHandler.h"

#include "lib/RedirectProcessor.h"
#include "lib/PBULKBackendClicks.h"

using namespace EchoService;
using namespace proxygen;

using folly::SocketAddress;

using Protocol = HTTPServer::Protocol;

DEFINE_int32(api_http_port, 11000, "Port to listen API on with HTTP protocol");
DEFINE_int32(redirect_http_port, 12000, "Port to listen API on with HTTP protocol");
// DEFINE_int32(spdy_port, 11001, "Port to listen on with SPDY protocol");
// DEFINE_int32(h2_port, 11002, "Port to listen on with HTTP/2 protocol");
DEFINE_string(ip, "localhost", "IP/Hostname to bind to");
DEFINE_string(postgres, "", "PostgresSql connection string (\"user=postgres host=localhost port=5432 dbname=urls_remap password=changeme\")");
DEFINE_string(geoip, "", "Path to geoip database");
DEFINE_int32(threads,
             0,
             "Number of threads to listen on. Numbers <= 0 "
             "will use the number of cores on this machine.");

DEFINE_int32(clicks_bulk, 10000, "Number of messages in queue throttle for reporting");

class RedirectHandlerFactory : public RequestHandlerFactory {
 public:
  RedirectHandlerFactory(std::shared_ptr<RedirectProcessor> processor,
              std::shared_ptr<Reporting<ClickInfo>> clickReporting)
              : processor(processor), clickReporting(clickReporting)
  {}

  void onServerStart(folly::EventBase* /*evb*/) noexcept override {}
  void onServerStop() noexcept override {}

  RequestHandler* onRequest(RequestHandler* rh, HTTPMessage* mess) noexcept override {
    return new RedirectHandler(processor, clickReporting);
  }

 private:
  std::shared_ptr<RedirectProcessor> processor;
  std::shared_ptr<Reporting<ClickInfo>> clickReporting;
};

class ApiHandlerFactory : public RequestHandlerFactory {
 public:
  ApiHandlerFactory(std::shared_ptr<RedirectProcessor> processor)
  {
    this->processor = processor;
  }

  void onServerStart(folly::EventBase* /*evb*/) noexcept override {}
  void onServerStop() noexcept override {}

  RequestHandler* onRequest(RequestHandler*, HTTPMessage*) noexcept override {
    return new ApiHandler(processor);
  }

 private:
  //folly::ThreadLocalPtr<EchoStats> stats_;
  std::shared_ptr<RedirectProcessor> processor;
};

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  LOG(INFO) << "api http_port: " << FLAGS_api_http_port << std::endl;
  LOG(INFO) << "redirect http_port: " << FLAGS_redirect_http_port << std::endl;
  LOG(INFO) << "ip: " << FLAGS_ip  << std::endl;
  LOG(INFO) << "threads: " << FLAGS_threads << std::endl;
  //LOG(INFO) << "postgres: " << FLAGS_postgres << std::endl;

  if(FLAGS_postgres.empty())
  {
    std::cerr << "--postgres \"\" parameter connection string can't be empty" << std::endl;
    return 1;
  }

  if(FLAGS_geoip.empty())
  {
    std::cerr << "--geoip \"\" parameter can't be empty and must be file to GeoIP database file" << std::endl;
    return 1;
  }

  auto gi = GeoIP_open(FLAGS_geoip.c_str(), GEOIP_MEMORY_CACHE); 
  if(gi == 0)
      throw std::runtime_error("can't load geoip database");

  if (FLAGS_threads <= 0) {
    FLAGS_threads = sysconf(_SC_NPROCESSORS_ONLN);
    CHECK(FLAGS_threads > 0);
  }

  auto processor = std::make_shared<RedirectProcessor>(FLAGS_postgres, gi);
  auto clickReporter = std::make_shared<Reporting<ClickInfo>>(std::make_unique<PBULKBackendClicks>(FLAGS_postgres), FLAGS_clicks_bulk);

// ============================ API =============================== //
  std::vector<HTTPServer::IPConfig> api_IPs = {
      {SocketAddress(FLAGS_ip, FLAGS_api_http_port, true), Protocol::HTTP}
  };

  HTTPServerOptions api_options;
  api_options.threads = 1;
  api_options.idleTimeout = std::chrono::milliseconds(60000);
  api_options.shutdownOn = {SIGINT, SIGTERM};
  api_options.enableContentCompression = false;
  api_options.handlerFactories =
      RequestHandlerChain()
      .addThen<ApiHandlerFactory>(processor)
      .build();
  // Increase the default flow control to 1MB/10MB
  api_options.initialReceiveWindow = uint32_t(1 << 20);
  api_options.receiveStreamWindowSize = uint32_t(1 << 20);
  api_options.receiveSessionWindowSize = 10 * (1 << 20);
  api_options.h2cEnabled = true;

  HTTPServer server(std::move(api_options));
  server.bind(api_IPs);
  std::thread t_api([&]() { server.start(); });

// ****************************************************************** //


// ============================ REDIRECT =============================== //
  std::vector<HTTPServer::IPConfig> redirect_IPs = {
      {SocketAddress(FLAGS_ip, FLAGS_redirect_http_port, true), Protocol::HTTP}
  };

  HTTPServerOptions redirect_options;
  redirect_options.threads = static_cast<size_t>(FLAGS_threads);
  redirect_options.idleTimeout = std::chrono::milliseconds(60000);
  //redirect_options.shutdownOn = {SIGINT, SIGTERM};
  redirect_options.enableContentCompression = false;
  redirect_options.handlerFactories =
      RequestHandlerChain()
      .addThen<RedirectHandlerFactory>(processor, clickReporter)
      .build();
  // Increase the default flow control to 1MB/10MB
  redirect_options.initialReceiveWindow = uint32_t(1 << 20);
  redirect_options.receiveStreamWindowSize = uint32_t(1 << 20);
  redirect_options.receiveSessionWindowSize = 10 * (1 << 20);
  redirect_options.h2cEnabled = true;

  HTTPServer server_redirect(std::move(redirect_options));
  server_redirect.bind(redirect_IPs);
  std::thread t_redirect([&]() { server_redirect.start();  });

// ****************************************************************** //

  
  t_api.join();
  server_redirect.stop();
  t_redirect.join();

  GeoIP_delete(gi);
  return 0;
}
