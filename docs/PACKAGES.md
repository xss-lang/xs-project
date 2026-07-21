<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: MPL-2.0
-->

# Packages and registry commands

The package registry is an early infrastructure track and is not yet available for production use. The compiler remains
usable without a registry.

The planned command contract is:

```text
xs login
xs publish
xs update
xs yank <module>@<version>
```

`xs login` prompts without echo for a GitHub personal access token. The service uses it once to establish the GitHub
identity, does not store it, and returns a revocable X# registry token. `xs publish` requires the evaluated project to set
`PUBLISH` to `true`; versions are immutable after publication. `xs update` resolves dependencies and replaces
`xs.lock.sqlite3` atomically only after the complete solution succeeds. `xs yank` excludes a version from new solutions
without breaking projects that already pin it in a lock file.

Registry transport, artifact signing, namespace policy, and public service availability are still under development.
