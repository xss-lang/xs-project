-- SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
-- SPDX-License-Identifier: Apache-2.0

SELECT 'CREATE ROLE xs_registry_owner NOLOGIN NOSUPERUSER NOCREATEDB NOCREATEROLE'
WHERE NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'xs_registry_owner')
\gexec

SELECT 'CREATE ROLE xsregistry LOGIN NOSUPERUSER NOCREATEDB NOCREATEROLE'
WHERE NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'xsregistry')
\gexec

SELECT 'CREATE DATABASE xs_registry OWNER xs_registry_owner ENCODING ''UTF8'' TEMPLATE template0'
WHERE NOT EXISTS (SELECT FROM pg_database WHERE datname = 'xs_registry')
\gexec

REVOKE ALL ON DATABASE xs_registry FROM PUBLIC;
GRANT CONNECT ON DATABASE xs_registry TO xsregistry;
