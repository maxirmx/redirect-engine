# SMS_UUID tests
# Yes, bro !
# They are not automated. But they exist !
# ---
# Pre
# Domain setup
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"domain" : "localhost:12000", "expired_on" : "2022-11-17 12:00:00", "default_url" : "http://nyt.com", "no_url_failover_url" : "http://www.washingtonpost.com/", "expired_url_failover_url" : "http://www.latimes.com/", "out_of_reach_failover_url" : "http://www.chicagotribune.com/", "whitelist" : ["RU", "US"] }' \
                  http://localhost:11000/api/update_domain
# 1.1
# With SMS_UIID, OK
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"orig_url":"http://www.seattletimes.com/", "created_on" : "2020-11-17 12:00:00", "expired_on" : "2021-11-17 17:39:49.546162", "sms_uuid":"827dd855fc1c", "domain":"localhost:12000", "whitelist":["RU", "US"]}' \
                  http://localhost:11000/api/create
# 1.2
# No SMS_UIID, still OK
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"orig_url":"http://www.dallasnews.com/", "created_on" : "2020-11-17 12:00:00", "expired_on" : "2021-11-17 17:39:49.546162", "domain":"localhost:12000", "whitelist":["RU", "US"]}' \
                  http://localhost:11000/api/create
# 1.3
# Long SMS_UIID, failure
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"orig_url":"http://www.dallasnews.com/", "created_on" : "2020-11-17 12:00:00", "expired_on" : "2021-11-17 17:39:49.546162", "sms_uuid":"7a423f4a-4725-45dc-9dde-4dd99d9a628b", "domain":"localhost:12000", "whitelist":["RU", "US"]}' \
                  http://localhost:11000/api/create

# Update
# 2.1
# ****************
# To set manually:
R_KEY="UjTu9u"
# ****************
# With SMS_UIID, OK
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"newUrl" : "http://localhost:12000/'${R_KEY}'", "orig_url" : "http://www.houstonchronicle.com/", "expired_on" : "2022-11-17 12:00:00", "sms_uuid" : "8b170c4327fa", "whitelist" : ["", "RU", "AU"]}' \
                  http://localhost:11000/api/update_redirect
# 2.2
# No SMS_UIID, OK
# INteresting quesion. COnsidering that SMS_UUID is updated what shall happen in this case ?
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"newUrl" : "http://localhost:12000/'${R_KEY}'", "orig_url" : "http://www.houstonchronicle.com/", "expired_on" : "2022-11-17 12:00:00", "sms_uuid" : "8b170c4327fa", "whitelist" : ["", "RU", "TR"]}' \
                  http://localhost:11000/api/update_redirect
# 2.3
# Long SMS_UIID, failure
curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"newUrl" : "http://localhost:12000/'${R_KEY}'", "orig_url" : "http://www.houstonchronicle.com/", "expired_on" : "2022-11-17 12:00:00", "sms_uuid" : "83628969-d5c4-409f-9b37-8bbbd41ec4fc", "whitelist" : ["", "RU", "US"]}' \
                  http://localhost:11000/api/update_redirect
