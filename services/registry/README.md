<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# X# package registry foundation

This directory contains the reproducible database foundation for the future X# package registry. It does not contain a
network service yet and is not part of the compiler build.

The intended CLI surface is:

- `xs login`: securely prompt for a GitHub personal access token, validate the GitHub identity over TLS, discard the PAT,
  and store an X# registry token in the user's platform credential store;
- `xs publish`: publish a new immutable module version from a project with `PUBLISH` enabled;
- `xs update`: resolve newer permitted dependency versions and atomically update `xs.lock.sqlite3`;
- `xs yank <module>@<version>`: hide a compromised or unusable version from new resolution without deleting its artifact.

Published version contents are immutable. Yanking is reversible and existing lock files remain resolvable. A package
service must never log or persist a GitHub PAT. Registry tokens are random opaque credentials and only their SHA-256
digests are stored.

## PostgreSQL boundary

The initial deployment uses PostgreSQL 16 on the registry host. PostgreSQL listens only on the local Unix socket; it is
not exposed through the host firewall. The `xs_registry_owner` role owns the schema but cannot log in. The
`xsregistry` operating-system account connects through peer authentication and receives only data access privileges.

Apply migrations as the PostgreSQL administrator in numeric order. `postgres/0001_initial.sql` creates identity,
ownership, token, module-version, artifact-metadata, and yank-audit records. The second migration narrows runtime writes
so published version content cannot be replaced; only its yank fields may change. Package blobs are deliberately not
stored in PostgreSQL; `storage_key` identifies content in the future artifact store.

The deployment templates keep SSH key-only and install a daily atomic PostgreSQL custom-format backup. The on-host backup
is a recovery convenience, not a substitute for copying encrypted backups to an independent system before production.
