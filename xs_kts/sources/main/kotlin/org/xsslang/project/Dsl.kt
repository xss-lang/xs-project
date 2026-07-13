/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

package org.xsslang.project

import java.util.Locale

@DslMarker
annotation class XsProjectDsl

@XsProjectDsl
class SourcesScope internal constructor() {
  internal val includes = mutableListOf<String>()
  internal val excludes = mutableListOf<String>()

  fun include(pattern: String) {
    includes += requireText(pattern, "source include")
  }

  fun exclude(pattern: String) {
    excludes += requireText(pattern, "source exclude")
  }
}

@XsProjectDsl
class DependenciesScope internal constructor() {
  internal val modules = mutableListOf<String>()

  fun addModule(reference: String) {
    modules += requireText(reference, "module reference")
  }
}

@XsProjectDsl
class TargetsScope internal constructor() {
  internal val values = mutableListOf<String>()

  fun target(triple: String) {
    values += requireText(triple, "target triple")
  }
}

@XsProjectDsl
class TestScope internal constructor() {
  internal val includes = mutableListOf<String>()
  internal var framework: String? = null

  fun include(pattern: String) {
    includes += requireText(pattern, "test include")
  }

  fun framework(name: String) {
    framework = requireText(name, "test framework")
  }
}

@XsProjectDsl
class CompilerScope internal constructor(
  private val settings: CompilerSettings,
) {
  fun warnings(level: String) {
    settings.warningLevel =
      try {
        WarningLevel.valueOf(level.uppercase(Locale.ROOT))
      } catch (_: IllegalArgumentException) {
        throw ProjectConfigurationException("unknown warning level '$level'")
      }
  }

  fun werror(enabled: Boolean) {
    settings.warningsAsErrors = enabled
  }

  fun verbose(enabled: Boolean) {
    settings.verbose = enabled
  }
}

class ProjectContext internal constructor(
  val host: Host = detectHost(),
  state: ProjectState? = null,
) {
  private var identity: ProjectIdentity? = state?.identity
  private val variables = (state?.variables ?: mapOf("XS_BACKEND" to "LLVM")).toMutableMap()
  private val authors = state?.authors?.toMutableList() ?: mutableListOf()
  private val modules = state?.modules?.toMutableList() ?: mutableListOf()
  private val targets = state?.targets?.toMutableList() ?: mutableListOf()
  private val sourceIncludes = state?.sourceIncludes?.toMutableList() ?: mutableListOf()
  private val sourceExcludes = state?.sourceExcludes?.toMutableList() ?: mutableListOf()
  private val testIncludes = state?.testIncludes?.toMutableList() ?: mutableListOf()
  private var testFramework: String? = state?.testFramework
  private val compilerSettings = state?.compiler ?: CompilerSettings()

  fun project(
    name: String,
    channel: String,
    version: String,
  ) {
    if (identity != null) throw ProjectConfigurationException("project(...) may be called only once")
    identity =
      ProjectIdentity(
        requireText(name, "project name"),
        requireText(channel, "release channel"),
        requireText(version, "project version"),
      )
  }

  fun set(
    name: String,
    value: String,
  ) {
    variables[requireText(name, "variable name")] = value
  }

  fun authors(vararg entries: Array<String>) {
    entries.forEach { entry ->
      if (entry.size != 2) throw ProjectConfigurationException("each author requires a name and email")
      authors += Author(requireText(entry[0], "author name"), requireText(entry[1], "author email"))
    }
  }

  fun dependencies(block: DependenciesScope.() -> Unit) {
    modules += DependenciesScope().apply(block).modules
  }

  fun targets(block: TargetsScope.() -> Unit) {
    targets += TargetsScope().apply(block).values
  }

  fun sources(block: SourcesScope.() -> Unit) {
    val scope = SourcesScope().apply(block)
    sourceIncludes += scope.includes
    sourceExcludes += scope.excludes
  }

  fun test(block: TestScope.() -> Unit) {
    val scope = TestScope().apply(block)
    testIncludes += scope.includes
    testFramework = scope.framework
  }

  fun compiler(block: CompilerScope.() -> Unit) {
    CompilerScope(compilerSettings).apply(block)
  }

  fun snapshot() =
    ProjectState(
      identity,
      variables.toMap(),
      authors.toList(),
      modules.toList(),
      targets.toList(),
      sourceIncludes.toList(),
      sourceExcludes.toList(),
      testIncludes.toList(),
      testFramework,
      compilerSettings.copy(),
    )

  fun build(): ProjectPlan {
    val project = identity ?: throw ProjectConfigurationException("project(...) is required")
    if (sourceIncludes.isEmpty()) throw ProjectConfigurationException("sources.include(...) is required")
    sourceIncludes.forEach { path ->
      if (path.any { character -> character in "*?[]{}" }) {
        throw ProjectConfigurationException("source includes must name explicit .xs files: $path")
      }
      if (!path.endsWith(".xs")) throw ProjectConfigurationException("source include must end in .xs: $path")
    }
    val mainCount =
      sourceIncludes.count { path ->
        java.nio.file.Path
          .of(path)
          .fileName
          .toString() == "main.xs"
      }
    if (mainCount != 1) throw ProjectConfigurationException("exactly one included file must be named main.xs")
    return ProjectPlan(
      project,
      variables.toSortedMap(),
      authors.toList(),
      modules.distinct(),
      targets.distinct(),
      sourceIncludes.distinct(),
      sourceExcludes.distinct(),
      testIncludes.distinct(),
      testFramework,
      compilerSettings,
    )
  }
}

internal fun requireText(
  value: String,
  field: String,
): String {
  if (value.isBlank()) throw ProjectConfigurationException("$field cannot be empty")
  return value
}

object ProjectRuntime {
  private var context = ProjectContext(state = ProjectStateFile.readConfigured())

  fun reset() {
    context = ProjectContext()
  }

  fun project(
    name: String,
    channel: String,
    version: String,
  ) = context.project(name, channel, version)

  fun set(
    name: String,
    value: String,
  ) = context.set(name, value)

  fun authors(vararg entries: Array<String>) = context.authors(*entries)

  fun dependencies(block: DependenciesScope.() -> Unit) = context.dependencies(block)

  fun targets(block: TargetsScope.() -> Unit) = context.targets(block)

  fun sources(block: SourcesScope.() -> Unit) = context.sources(block)

  fun test(block: TestScope.() -> Unit) = context.test(block)

  fun compiler(block: CompilerScope.() -> Unit) = context.compiler(block)

  fun build() = context.build()

  fun snapshot() = context.snapshot()

  val host get() = context.host
}

val OS get() = ProjectRuntime.host.os
val FAMILY get() = ProjectRuntime.host.family
val ARCH get() = ProjectRuntime.host.architecture
val LINUX get() = OperatingSystem.LINUX
val MACOS get() = OperatingSystem.MACOS
val WINDOWS get() = OperatingSystem.WINDOWS
val FREEBSD get() = OperatingSystem.FREEBSD
val OPENBSD get() = OperatingSystem.OPENBSD
val NETBSD get() = OperatingSystem.NETBSD
val UNIX get() = OperatingSystemFamily.UNIX
val BSD get() = OperatingSystemFamily.BSD
val X86_64 get() = Architecture.X86_64
val AARCH64 get() = Architecture.AARCH64
val ARMV7H get() = Architecture.ARMV7H
val RISCV64 get() = Architecture.RISCV64

fun cfg(condition: Boolean) = condition

fun panic(message: String): Nothing = throw ProjectAbort(message)

fun project(
  name: String,
  channel: String,
  version: String,
) = ProjectRuntime.project(name, channel, version)

fun set(
  name: String,
  value: String,
) = ProjectRuntime.set(name, value)

fun authors(vararg entries: Array<String>) = ProjectRuntime.authors(*entries)

fun dependencies(block: DependenciesScope.() -> Unit) = ProjectRuntime.dependencies(block)

fun targets(block: TargetsScope.() -> Unit) = ProjectRuntime.targets(block)

fun sources(block: SourcesScope.() -> Unit) = ProjectRuntime.sources(block)

fun test(block: TestScope.() -> Unit) = ProjectRuntime.test(block)

fun compiler(block: CompilerScope.() -> Unit) = ProjectRuntime.compiler(block)
