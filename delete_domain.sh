#!/bin/bash

for i in {0..0}
  do
        curl --header "Content-Type: application/json" \
                  --request POST \
                  --data '{"domain" : "localhost:12000" }' \
                  http://localhost:11000/api/delete_domain
done
