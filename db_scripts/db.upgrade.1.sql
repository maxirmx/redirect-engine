-- Database upgrade #1
-- maxirmx
-- March 14, 2021
-- Added country_iso, referer and user agent


ALTER TABLE public.clicks
    ADD COLUMN country_iso character varying(2) COLLATE pg_catalog."default";

ALTER TABLE public.clicks
    ADD COLUMN referer character varying(256) COLLATE pg_catalog."default";

ALTER TABLE public.clicks
    ADD COLUMN user_agent character varying(256) COLLATE pg_catalog."default";
