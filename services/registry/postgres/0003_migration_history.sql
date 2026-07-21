-- SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
-- SPDX-License-Identifier: Apache-2.0

BEGIN;

CREATE TABLE registry.schema_migrations
(
  version INTEGER PRIMARY KEY CHECK (version > 0),
  name TEXT NOT NULL UNIQUE,
  applied_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

ALTER TABLE registry.schema_migrations OWNER TO xs_registry_owner;
REVOKE ALL ON registry.schema_migrations FROM PUBLIC;
GRANT SELECT ON registry.schema_migrations TO xsregistry;

INSERT INTO registry.schema_migrations(version, name)
VALUES
  (1, 'initial'),
  (2, 'runtime_privileges'),
  (3, 'migration_history');

COMMIT;
