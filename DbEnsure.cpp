#include <pqxx/pqxx>
#include <string>
#include <ctime>

#include <glog/logging.h>
#include <DbEnsure.h>

void DbEnsure::Ensure(void) {
    try {
        LOG(INFO) << "Initializing database at 0.2.0";
        Ensure_0_2_0();
    } catch (const std::exception& ex) {
        LOG(ERROR) << "Error initializing database: " << ex.what();
    }
}

std::string DbEnsure::PuVersionUpdateQuery(const std::string& version) {
    std::time_t t = std::time(nullptr);
    char date[100];
    std::strftime(date, sizeof(date), "%Y-%m-%d", std::localtime(&t));
    return "START TRANSACTION; INSERT INTO versions (version, date) VALUES ('" + version + "', '" + date + "'); COMMIT;";
}

std::string DbEnsure::VQuery(const std::string& version) {
    return "SELECT COUNT(*) FROM versions WHERE version = '" + version + "';";
}

bool DbEnsure::VCheck(const std::string& version) {
    pqxx::work txn(connection);
    pqxx::result result = txn.exec(VQuery(version));
    txn.commit();
    return !result.empty() && result[0][0].as<long>() != 0;
}

int DbEnsure::Ensure_0_2_0(void) {
    pqxx::work txn(connection);
    pqxx::result result = txn.exec("SELECT COUNT(*) FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = 'versions';");

    int res = 0;
    if (!result.empty() && result[0][0].as<long>() != 0) {
        result = txn.exec("SELECT COUNT(*) FROM versions WHERE version = '0.1.4';");
    }

    if (result.empty() || result[0][0].as<long>() == 0) {
        txn.exec(sqlScript_0_2_0);
        res = 1;
    }

    txn.commit();
    return res;
}

void DbEnsure::PuVersionUpdate(const std::string& version) {
    if (!VCheck(version)) {
        pqxx::work txn(connection);
        txn.exec(PuVersionUpdateQuery(version));
        txn.commit();
    }
}

void DbEnsure::EnsureVersion(const std::string& version, const std::string& statement) {
    if (!VCheck(version)) {
        pqxx::work txn(connection);
        txn.exec(statement);
        txn.commit();
    }
}

const std::string DbEnsure::sqlScript_0_2_0 = R"(
-- Table: public.clicks
-- DROP TABLE public.clicks;

CREATE TABLE public.clicks
(
    new_url character varying(64) COLLATE pg_catalog."default" NOT NULL,
    replaced_url character varying(256) COLLATE pg_catalog."default",
    clicked_on timestamp without time zone,
    from_ip character varying(64) COLLATE pg_catalog."default",
    sms_uuid character varying(32) COLLATE pg_catalog."default",
    country_iso character varying(2) COLLATE pg_catalog."default",
    referer character varying(256) COLLATE pg_catalog."default",
    user_agent character varying(256) COLLATE pg_catalog."default",
    country_iso character varying(2) COLLATE pg_catalog."default",
    referer character varying(256) COLLATE pg_catalog."default",
    user_agent character varying(256) COLLATE pg_catalog."default";

)
TABLESPACE pg_default;

ALTER TABLE public.clicks
    OWNER to postgres;


-- Table: public.mapping
-- DROP TABLE public.mapping;

CREATE TABLE public.mapping
(
    orig_url character varying(256) COLLATE pg_catalog."default" NOT NULL,
    new_url character varying(64) COLLATE pg_catalog."default" NOT NULL,
    created_on timestamp without time zone NOT NULL,
    expired_on timestamp without time zone NOT NULL,
    sms_uuid character varying(32) COLLATE pg_catalog."default"
)
TABLESPACE pg_default;

ALTER TABLE public.mapping
    OWNER to postgres;

-- Index: newUrlIndex
-- DROP INDEX public."newUrlIndex";

CREATE INDEX "newUrlIndex"
    ON public.mapping USING btree
    (new_url COLLATE pg_catalog."default" varchar_ops ASC NULLS LAST)
    TABLESPACE pg_default;

-- Table: public.mappingwl
-- DROP TABLE public.mappingwl;

CREATE TABLE public.mappingwl
(
    new_url character varying(64) COLLATE pg_catalog."default" NOT NULL,
    country_iso character varying(8) COLLATE pg_catalog."default" NOT NULL
)
TABLESPACE pg_default;

ALTER TABLE public.mappingwl
    OWNER to postgres;

-- Index: newURLWLIndex
-- DROP INDEX public."newURLWLIndex";

CREATE INDEX "newURLWLIndex"
    ON public.mappingwl USING btree
    (new_url COLLATE pg_catalog."default" varchar_ops ASC NULLS LAST)
    TABLESPACE pg_default;

-- Table: public.mappingre
-- DROP TABLE public.mappingre;

CREATE TABLE public.mappingre
(
    new_url character varying(64) COLLATE pg_catalog."default" NOT NULL,
    referrer character varying(256) COLLATE pg_catalog."default" NOT NULL
)
TABLESPACE pg_default;

ALTER TABLE public.mappingre
    OWNER to postgres;

-- Index: newURLREIndex
-- DROP INDEX public."newURLREIndex";

CREATE INDEX "newURLREIndex"
    ON public.mappingre USING btree
    (new_url COLLATE pg_catalog."default" varchar_ops ASC NULLS LAST)
    TABLESPACE pg_default;

-- Table: public.mappingag
-- DROP TABLE public.mappingag;

CREATE TABLE public.mappingag
(
    new_url character varying(64) COLLATE pg_catalog."default" NOT NULL,
    user_agent character varying(256) COLLATE pg_catalog."default" NOT NULL
)
TABLESPACE pg_default;

ALTER TABLE public.mappingag
    OWNER to postgres;

-- Index: newURLAGIndex
-- DROP INDEX public."newURLAGIndex";
CREATE INDEX "newURLAGIndex"
    ON public.mappingag USING btree
    (new_url COLLATE pg_catalog."default" varchar_ops ASC NULLS LAST)
    TABLESPACE pg_default;


-- Table: public.url
-- DROP TABLE public.url;
CREATE TABLE public.url
(
    url_id integer NOT NULL,
    url character varying(128) COLLATE pg_catalog."default" NOT NULL,
    created_on timestamp without time zone NOT NULL,
    expired_on timestamp without time zone NOT NULL,
    default_url character varying(128) COLLATE pg_catalog."default",
    no_url_failover_url character varying(128) COLLATE pg_catalog."default",
    expired_url_failover_url character varying(128) COLLATE pg_catalog."default",
    out_of_reach_failover_url character varying(128) COLLATE pg_catalog."default",
    CONSTRAINT remap_pkey PRIMARY KEY (url_id)
)
TABLESPACE pg_default;

ALTER TABLE public.url
    OWNER to postgres;

-- Table: public.url_whitelist
-- DROP TABLE public.url_whitelist;

CREATE TABLE public.url_whitelist
(
    url_id integer NOT NULL,
    country_iso character varying(8) COLLATE pg_catalog."default" NOT NULL
)
TABLESPACE pg_default;

ALTER TABLE public.url_whitelist
    OWNER to postgres;

-- Table: public.url_agents
-- DROP TABLE public.url_agents;

CREATE TABLE public.url_agents
(
    url_id integer NOT NULL,
    user_agent character varying(256) COLLATE pg_catalog."default" NOT NULL
)
TABLESPACE pg_default;

ALTER TABLE public.url_agents
    OWNER to postgres;

-- Table: public.url_referrers
-- DROP TABLE public.url_referrers;

CREATE TABLE public.url_referrers
(
    url_id integer NOT NULL,
    referrer character varying(256) COLLATE pg_catalog."default" NOT NULL
)
TABLESPACE pg_default;

ALTER TABLE public.url_referrers
    OWNER to postgres;

-- Table: public.mappingre
-- DROP TABLE public.mappingre;

CREATE TABLE public.mappingre
(
    new_url character varying(64) COLLATE pg_catalog."default" NOT NULL,
    referrer character varying(256) COLLATE pg_catalog."default" NOT NULL
)
TABLESPACE pg_default;

ALTER TABLE public.mappingre
    OWNER to postgres;

-- Index: newURLREIndex
-- DROP INDEX public."newURLREIndex";

CREATE INDEX "newURLREIndex"
    ON public.mappingre USING btree
    (new_url COLLATE pg_catalog."default" varchar_ops ASC NULLS LAST)
    TABLESPACE pg_default;

-- Table: public.mappingag
-- DROP TABLE public.mappingag;

CREATE TABLE public.mappingag
(
    new_url character varying(64) COLLATE pg_catalog."default" NOT NULL,
    user_agent character varying(256) COLLATE pg_catalog."default" NOT NULL
)
TABLESPACE pg_default;

ALTER TABLE public.mappingag
    OWNER to postgres;

-- Index: newURLAGIndex
-- DROP INDEX public."newURLAGIndex";

CREATE INDEX "newURLAGIndex"
    ON public.mappingag USING btree
    (new_url COLLATE pg_catalog."default" varchar_ops ASC NULLS LAST)
    TABLESPACE pg_default;


-- Table: public.url_agents
-- DROP TABLE public.url_agents;

CREATE TABLE public.url_agents
(
    url_id integer NOT NULL,
    user_agent character varying(256) COLLATE pg_catalog."default" NOT NULL
)
TABLESPACE pg_default;

ALTER TABLE public.url_agents
    OWNER to postgres;

-- Table: public.url_referrers
-- DROP TABLE public.url_referrers;

CREATE TABLE public.url_referrers
(
    url_id integer NOT NULL,
    referrer character varying(256) COLLATE pg_catalog."default" NOT NULL
)
TABLESPACE pg_default;

ALTER TABLE public.url_referrers
    OWNER to postgres;

CREATE TABLE "versions" (
      "id"      SERIAL PRIMARY KEY,
      "version" VARCHAR(16) NOT NULL,
      "date"    DATE NOT NULL
    );

CREATE UNIQUE INDEX "idx_versions_version" ON "versions" ("version");

INSERT INTO "versions" ("version", "date") VALUES('0.2.0', '2024-11-11');
)";
