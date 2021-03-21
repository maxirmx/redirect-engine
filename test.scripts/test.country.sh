# Country-based routing tests
# Yes, bro !
# They are not automated. But they exist !
# ---
# 1.1
# 
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"domain" : "localhost:12000", "expired_on" : "2022-11-17 12:00:00", "default_url" : "http://nyt.com", "no_url_failover_url" : "http://www.washingtonpost.com/", "expired_url_failover_url" : "http://www.latimes.com/", "out_of_reach_failover_url" : "http://www.chicagotribune.com/", "whitelist" : ["RU", "US"] }' \
                  http://localhost:11000/api/update_domain

curl -v localhost:12000
# 1.2
# 
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"domain" : "localhost:12000", "expired_on" : "2022-11-17 12:00:00", "default_url" : "http://nyt.com", "no_url_failover_url" : "http://www.washingtonpost.com/", "expired_url_failover_url" : "http://www.latimes.com/", "out_of_reach_failover_url" : "http://www.chicagotribune.com/", "whitelist" : [] }' \
                  http://localhost:11000/api/update_domain

curl -v localhost:12000
# 1.3
# 
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"domain" : "localhost:12000", "expired_on" : "2022-11-17 12:00:00", "default_url" : "http://nyt.com", "no_url_failover_url" : "http://www.washingtonpost.com/", "expired_url_failover_url" : "http://www.latimes.com/", "out_of_reach_failover_url" : "http://www.chicagotribune.com/" }' \
                  http://localhost:11000/api/update_domain

curl -v localhost:12000