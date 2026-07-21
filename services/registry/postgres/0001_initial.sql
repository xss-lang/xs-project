-- SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
-- SPDX-License-Identifier: Apache-2.0

BEGIN;

CREATE SCHEMA IF NOT EXISTS registry AUTHORIZATION xs_registry_owner;

CREATE TABLE registry.accounts
(
  id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  github_id BIGINT NOT NULL UNIQUE CHECK (github_id > 0),
  github_login TEXT NOT NULL,
  created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE UNIQUE INDEX accounts_github_login_ci ON registry.accounts (lower(github_login));

CREATE TABLE registry.access_tokens
(
  id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  account_id BIGINT NOT NULL REFERENCES registry.accounts(id),
  token_digest BYTEA NOT NULL UNIQUE CHECK (octet_length(token_digest) = 32),
  created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
  expires_at TIMESTAMPTZ NOT NULL,
  last_used_at TIMESTAMPTZ,
  revoked_at TIMESTAMPTZ,
  CHECK (expires_at > created_at)
);

CREATE TABLE registry.modules
(
  id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  name TEXT NOT NULL UNIQUE,
  created_by BIGINT NOT NULL REFERENCES registry.accounts(id),
  created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE registry.module_owners
(
  module_id BIGINT NOT NULL REFERENCES registry.modules(id),
  account_id BIGINT NOT NULL REFERENCES registry.accounts(id),
  can_publish BOOLEAN NOT NULL DEFAULT TRUE,
  can_yank BOOLEAN NOT NULL DEFAULT TRUE,
  PRIMARY KEY (module_id, account_id)
);

CREATE TABLE registry.versions
(
  id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  module_id BIGINT NOT NULL REFERENCES registry.modules(id),
  version TEXT NOT NULL,
  stability TEXT NOT NULL,
  manifest JSONB NOT NULL,
  storage_key TEXT NOT NULL UNIQUE,
  sha256 BYTEA NOT NULL CHECK (octet_length(sha256) = 32),
  size_bytes BIGINT NOT NULL CHECK (size_bytes >= 0),
  published_by BIGINT NOT NULL REFERENCES registry.accounts(id),
  published_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
  yanked_at TIMESTAMPTZ,
  yanked_by BIGINT REFERENCES registry.accounts(id),
  yank_reason TEXT,
  UNIQUE (module_id, version),
  CHECK ((yanked_at IS NULL) = (yanked_by IS NULL))
);

CREATE TABLE registry.yank_events
(
  id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
  version_id BIGINT NOT NULL REFERENCES registry.versions(id),
  account_id BIGINT NOT NULL REFERENCES registry.accounts(id),
  yanked BOOLEAN NOT NULL,
  reason TEXT,
  created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

ALTER TABLE registry.accounts OWNER TO xs_registry_owner;
ALTER TABLE registry.access_tokens OWNER TO xs_registry_owner;
ALTER TABLE registry.modules OWNER TO xs_registry_owner;
ALTER TABLE registry.module_owners OWNER TO xs_registry_owner;
ALTER TABLE registry.versions OWNER TO xs_registry_owner;
ALTER TABLE registry.yank_events OWNER TO xs_registry_owner;

REVOKE ALL ON SCHEMA registry FROM PUBLIC;
REVOKE ALL ON ALL TABLES IN SCHEMA registry FROM PUBLIC;
REVOKE ALL ON ALL SEQUENCES IN SCHEMA registry FROM PUBLIC;
GRANT USAGE ON SCHEMA registry TO xsregistry;
GRANT SELECT, INSERT, UPDATE ON ALL TABLES IN SCHEMA registry TO xsregistry;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA registry TO xsregistry;

COMMIT;
