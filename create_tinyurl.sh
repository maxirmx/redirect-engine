#!/bin/bash

for i in {0..100}
  do
        curl --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"orig_url":"http://yandex.ru","created_on":"2020-11-17 17:39:49.546162", "expired_on" : "2021-11-17 17:39:49.546162", "sms_uuid":"knockknock", "domain":"localhost:12000"}' \
                  http://localhost:11000/api/create
done
