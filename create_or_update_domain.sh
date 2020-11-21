#!/bin/bash

for i in {0..0}
  do
        curl --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"domain" : "localhost:12000", "expired_on" : "2022-11-17 12:00:00", "default_url" : "http://rsdn.ru", "no_url_failover_url" : "http://google.com", "expired_url_failover_url" : "http://boost.org", "out_of_reach_failover_url" : "http://me.ru", "whitelist" : ["RU", "US"] }' \
                  http://localhost:11000/api/update_domain
done
