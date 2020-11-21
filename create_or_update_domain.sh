#!/bin/bash


curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"domain" : "localhost:12000", "expired_on" : "2022-11-17 12:00:00", "default_url" : "http://rsdn.ru", "no_url_failover_url" : "http://google.com", "expired_url_failover_url" : "http://boost.org", "out_of_reach_failover_url" : "http://ori.org", "whitelist" : ["", "RU", "US"] }' \
                  http://localhost:11000/api/update_domain


curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"domain" : "localhost:13000", "expired_on" : "2022-11-17 12:00:00", "default_url" : "http://rsdn.ru", "no_url_failover_url" : "http://google.com", "expired_url_failover_url" : "http://boost.org", "out_of_reach_failover_url" : "http://ori.org", "whitelist" : ["RU", "US"] }' \
                  http://localhost:11000/api/update_domain

curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"domain" : "localhost:14000", "expired_on" : "2022-11-17 12:00:00", "default_url" : "http://rsdn.ru", "no_url_failover_url" : "http://google.com", "expired_url_failover_url" : "http://boost.org", "out_of_reach_failover_url" : "http://ori.org", "whitelist" : ["RU", "US"] }' \
                  http://localhost:11000/api/update_domain

curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"domain" : "192.99.10.113:12000", "expired_on" : "2022-11-17 12:00:00", "default_url" : "http://rsdn.ru", "no_url_failover_url" : "http://google.com", "expired_url_failover_url" : "http://boost.org", "out_of_reach_failover_url" : "http://ori.org", "whitelist" : ["RU", "US"] }' \
                  http://localhost:11000/api/update_domain
