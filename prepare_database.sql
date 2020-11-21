-- Table: public.clicks

-- DROP TABLE public.clicks;

CREATE TABLE public.clicks
(
    new_url character varying(64) COLLATE pg_catalog."default",
    replaced_url character varying(256) COLLATE pg_catalog."default",
    clicked_on timestamp without time zone,
    from_ip character varying(64) COLLATE pg_catalog."default",
    sms_uuid character varying(32) COLLATE pg_catalog."default"
)

TABLESPACE pg_default;

ALTER TABLE public.clicks
    OWNER to postgres;

-- Table: public.mapping

-- DROP TABLE public.mapping;

CREATE TABLE public.mapping
(
    orig_url character varying(256) COLLATE pg_catalog."default",
    new_url character varying(64) COLLATE pg_catalog."default",
    created_on timestamp without time zone,
    expired_on timestamp without time zone,
    sms_uuid character varying(32) COLLATE pg_catalog."default"
)

TABLESPACE pg_default;

ALTER TABLE public.mapping
    OWNER to postgres;

CREATE INDEX "newUrlIndex" ON public.mapping USING btree (new_url COLLATE pg_catalog."default" varchar_ops ASC NULLS LAST) TABLESPACE pg_default;

-- Table: public.url

-- DROP TABLE public.url;

CREATE TABLE public.url
(
    url_id integer NOT NULL,
    url character varying(128) COLLATE pg_catalog."default",
    created_on timestamp without time zone,
    expired_on timestamp without time zone,
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