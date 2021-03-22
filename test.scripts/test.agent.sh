# Country-based routing tests
# Yes, bro !
# They are not automated. But they exist !
# ---
# 1.1
# URL, with user agents
echo "---------------- Stage 1 (domains) ----------------"
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data  '{"domain" : "localhost:12000", 
                            "expired_on" : "2022-11-17 12:00:00", 
                            "default_url" : "http://nyt.com", 
                            "no_url_failover_url" : "http://www.washingtonpost.com/", 
                            "expired_url_failover_url" : "http://www.latimes.com/", 
                            "out_of_reach_failover_url" : "http://www.chicagotribune.com/", 
                            "user_agents" : ["curl"] }' \
                  http://localhost:11000/api/update_domain

# 1.2
# URL, with empty user agents list
#curl -v --header "Content-Type: application/json" \
#                  --request POST \
#                  --data '{"domain" : "localhost:12000", 
#                           "expired_on" : "2022-11-17 12:00:00", 
#                           "default_url" : "http://nyt.com", 
#                           "no_url_failover_url" : "http://www.washingtonpost.com/", 
#                           "expired_url_failover_url" : "http://www.latimes.com/",
#                           "out_of_reach_failover_url" : "http://www.chicagotribune.com/", 
#                           "user_agents" : [] }' \
#                  http://localhost:11000/api/update_domain

# 1.3
# URL, no user agents
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
# mapping, with user agents
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"orig_url":"http://www.dallasnews.com/",
                           "created_on":"2020-11-17 17:39:49.546162", 
                           "expired_on" : "2021-11-17 17:39:49.546162", 
                           "sms_uuid":"827dd855fc1c", "domain":"localhost:12000", 
                           "user_agents":["curl"]}' \
                  http://localhost:11000/api/create

curl -v localhost:12000/8732U4

# 2.2
# mapping, with empty user agents list
# curl -v --header "Content-Type: application/json" \
#                  --request POST \
#                  --data '{"orig_url":"http://idahonews.com/",
#                           "created_on":"2020-11-17 17:39:49.546162", 
#                           "expired_on" : "2021-11-17 17:39:49.546162", 
#                           "sms_uuid":"827dd855fc1c", 
#                           "domain":"localhost:12000", 
#                           "referrers":[]}' \
#                  http://localhost:11000/api/create


# 2.3
# mapping, no user agents
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"orig_url": "https://sacobserver.com/",
                  "created_on":"2020-11-17 17:39:49.546162", 
                  "expired_on" : "2021-11-17 17:39:49.546162", 
                  "domain":"localhost:12000"}' \
                  http://localhost:11000/api/create

curl -v localhost:12000/Z8s8u5
