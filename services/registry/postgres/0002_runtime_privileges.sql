-- SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
-- SPDX-License-Identifier: Apache-2.0

BEGIN;

REVOKE INSERT, UPDATE, DELETE ON ALL TABLES IN SCHEMA registry FROM xsregistry;

GRANT INSERT, UPDATE ON registry.accounts TO xsregistry;
GRANT INSERT, UPDATE ON registry.access_tokens TO xsregistry;
GRANT INSERT ON registry.modules TO xsregistry;
GRANT INSERT, UPDATE ON registry.module_owners TO xsregistry;
GRANT INSERT ON registry.versions TO xsregistry;
GRANT UPDATE (yanked_at, yanked_by, yank_reason) ON registry.versions TO xsregistry;
GRANT INSERT ON registry.yank_events TO xsregistry;

COMMIT;
