# Country-based routing tests
# Yes, bro !
# They are not automated. But they exist !
# ---
# 1.1
# URL, with whitelist
echo "---------------- Stage 1 (domains) ----------------"
#curl -v --header "Content-Type: application/json" \
#                  --request POST \
#                  --data  '{"domain" : "localhost:12000", 
#                            "expired_on" : "2022-11-17 12:00:00", 
#                            "default_url" : "http://nyt.com", 
#                            "no_url_failover_url" : "http://www.washingtonpost.com/", 
#                            "expired_url_failover_url" : "http://www.latimes.com/", 
#                            "out_of_reach_failover_url" : "http://www.chicagotribune.com/", 
#                            "referrers" : ["facebook.com", "twitter.com"] }' \
#                  http://localhost:11000/api/update_domain

# 1.2
# URL, with empty whitelist
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"domain" : "localhost:12000", 
                           "expired_on" : "2022-11-17 12:00:00", 
                           "default_url" : "http://nyt.com", 
                           "no_url_failover_url" : "http://www.washingtonpost.com/", 
                           "expired_url_failover_url" : "http://www.latimes.com/",
                           "out_of_reach_failover_url" : "http://www.chicagotribune.com/", 
                           "referrers" : [] }' \
                  http://localhost:11000/api/update_domain

# 1.3
# URL, no whitelist
#curl -v --header "Content-Type: application/json" \
#                  --request POST \
#                  --data '{"domain" : "localhost:12000", 
#                           "expired_on" : "2022-11-17 12:00:00", 
#                           "default_url" : "http://nyt.com", 
#                           "no_url_failover_url" : "http://www.washingtonpost.com/", 
#                           "expired_url_failover_url" : "http://www.latimes.com/", 
#                           "out_of_reach_failover_url" : "http://www.chicagotribune.com/" }' \
#                  http://localhost:11000/api/update_domain

echo "---------------- Stage 2 (mappings) ----------------"
# 2.1
# mapping, with whitelist
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"orig_url":"http://www.dallasnews.com/",
                           "created_on":"2020-11-17 17:39:49.546162", 
                           "expired_on" : "2021-11-17 17:39:49.546162", 
                           "sms_uuid":"827dd855fc1c", "domain":"localhost:12000", 
                           "referrers":["pinterest.com", "telegram.com"]}' \
                  http://localhost:11000/api/create

curl -v localhost:12000/8pun5V
curl -v --referer pinterest.com localhost:12000/8pun5V

# 2.2
# mapping, with empty whitelist
#curl -v --header "Content-Type: application/json" \
#                  --request POST \
#                  --data '{"orig_url":"http://idahonews.com/",
#                           "created_on":"2020-11-17 17:39:49.546162", 
#                           "expired_on" : "2021-11-17 17:39:49.546162", 
#                           "sms_uuid":"827dd855fc1c", 
#                           "domain":"localhost:12000", 
#                           "referrers":[]}' \
#                  http://localhost:11000/api/create

#curl -v localhost:12000/JeQc2l

# 2.3
# mapping, no whitelist
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"orig_url": "https://sacobserver.com/",
                  "created_on":"2020-11-17 17:39:49.546162", 
                  "expired_on" : "2021-11-17 17:39:49.546162", 
                  "domain":"localhost:12000"}' \
                  http://localhost:11000/api/create

curl -v localhost:12000/NJW1hf
