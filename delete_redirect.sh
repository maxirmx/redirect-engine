#!/bin/bash

for i in {0..0}
  do
        curl --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"newUrl" : "http://localhost:12000/LBItIU" }' \
                  http://localhost:11000/api/delete_redirect
done
