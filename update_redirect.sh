#!/bin/bash

curl -v --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"newUrl" : "http://localhost:12000/LBItIU", "orig_url" : "http://lamoda.ru", "expired_on" : "2022-11-17 12:00:00", "sms_uuid" : "ywtwy", "whitelist" : ["", "RU", "US"]}' \
                  http://localhost:11000/api/update_redirect

