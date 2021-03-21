-- Database upgrade #2
-- maxirmx
-- March 21, 2021
-- Added referrer and user agent  -- based routing

-- Table: public.mappingre
-- DROP TABLE public.mappingre;

TABLESPACE pg_default;

CREATE TABLE public.mappingre
(
    new_url character varying(64) COLLATE pg_catalog."default" NOT NULL,
    referrer character varying(256) COLLATE pg_catalog."default" NOT NULL
)

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

TABLESPACE pg_default;

CREATE TABLE public.mappingag
(
    new_url character varying(64) COLLATE pg_catalog."default" NOT NULL,
    user_agent character varying(256) COLLATE pg_catalog."default" NOT NULL
)

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

TABLESPACE pg_default;

CREATE TABLE public.url_agents
(
    url_id integer NOT NULL,
    user_agent character varying(256) COLLATE pg_catalog."default" NOT NULL
)

ALTER TABLE public.url_agents
    OWNER to postgres;

-- Table: public.url_referrers
-- DROP TABLE public.url_referrers;

TABLESPACE pg_default;

CREATE TABLE public.url_referrers
(
    url_id integer NOT NULL,
    referrer character varying(256) COLLATE pg_catalog."default" NOT NULL
)


ALTER TABLE public.url_referrers
    OWNER to postgres;
