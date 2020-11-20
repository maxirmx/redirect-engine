#pragma once

#include <string>
#include "RedirectInfo.h"
#include "RedirectProcessor.h"

class PBULKBackendClicks : public ReportingBackend<ClickInfo>
{
public:
    PBULKBackendClicks(std::string connectionString)
    {
        this->p_connection = std::make_unique<pqxx::connection>(connectionString);
    }

protected:
    void Report(const std::vector<ClickInfo> &messages) override
    {
        pqxx::work txn{*this->p_connection};

        for(auto click : messages)
        {
            txn.exec_params("INSERT INTO clicks(new_url, replaced_url, clicked_on, from_ip, sms_uuid) VALUES ($1, $2, $3, $4, $5);",
                        click.newUrl, click.replaced_url, RedirectProcessor::ToTimeStampString(click.clicked_on), click.clientIP, click.sms_uuid );
        }

        txn.commit();
    }
private:

    std::unique_ptr<pqxx::connection> p_connection;
};