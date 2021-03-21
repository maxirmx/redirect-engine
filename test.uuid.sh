curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"domain" : "localhost:12000", "expired_on" : "2022-11-17 12:00:00", "default_url" : "http://nyt.com", "no_url_failover_url" : "http://https://www.washingtonpost.com/", "expired_url_failover_url" : "http://https://www.latimes.com/", "out_of_reach_failover_url" : "http://https://www.chicagotribune.com/", "whitelist" : ["RU", "US"] }' \
                  http://localhost:11000/api/update_domain