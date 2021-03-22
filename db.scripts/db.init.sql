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
    user_agent character varying(256) COLLATE pg_catalog."default"
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