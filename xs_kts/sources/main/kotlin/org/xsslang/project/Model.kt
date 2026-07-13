/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

package org.xsslang.project

import java.io.Serializable

enum class OperatingSystem { LINUX, MACOS, WINDOWS, FREEBSD, OPENBSD, NETBSD, UNKNOWN }

enum class OperatingSystemFamily { UNIX, BSD, WINDOWS, UNKNOWN }

enum class Architecture { X86_64, AARCH64, ARMV7H, RISCV64, UNKNOWN }

enum class WarningLevel { ALL, MEDIUM, LOW, NONE }

data class Host(
  val os: OperatingSystem,
  val family: OperatingSystemFamily,
  val architecture: Architecture,
)

data class ProjectIdentity(
  val name: String,
  val channel: String,
  val version: String,
) : Serializable

data class Author(
  val name: String,
  val email: String,
) : Serializable

data class CompilerSettings(
  var warningLevel: WarningLevel = WarningLevel.MEDIUM,
  var warningsAsErrors: Boolean = false,
  var verbose: Boolean = false,
) : Serializable

data class ProjectState(
  val identity: ProjectIdentity?,
  val variables: Map<String, String>,
  val authors: List<Author>,
  val modules: List<String>,
  val targets: List<String>,
  val sourceIncludes: List<String>,
  val sourceExcludes: List<String>,
  val testIncludes: List<String>,
  val testFramework: String?,
  val compiler: CompilerSettings,
) : Serializable

data class ProjectPlan(
  val identity: ProjectIdentity,
  val variables: Map<String, String>,
  val authors: List<Author>,
  val modules: List<String>,
  val targets: List<String>,
  val sourceIncludes: List<String>,
  val sourceExcludes: List<String>,
  val testIncludes: List<String>,
  val testFramework: String?,
  val compiler: CompilerSettings,
)

class ProjectConfigurationException(
  message: String,
) : IllegalArgumentException(message)

class ProjectAbort(
  message: String,
) : RuntimeException(message)

internal fun detectHost(): Host {
  val osName = System.getProperty("os.name", "").lowercase()
  val architectureName = System.getProperty("os.arch", "").lowercase()
  val os =
    when {
      osName.contains("linux") -> OperatingSystem.LINUX
      osName.contains("mac") || osName.contains("darwin") -> OperatingSystem.MACOS
      osName.contains("windows") -> OperatingSystem.WINDOWS
      osName.contains("freebsd") -> OperatingSystem.FREEBSD
      osName.contains("openbsd") -> OperatingSystem.OPENBSD
      osName.contains("netbsd") -> OperatingSystem.NETBSD
      else -> OperatingSystem.UNKNOWN
    }
  val family =
    when (os) {
      OperatingSystem.FREEBSD, OperatingSystem.OPENBSD, OperatingSystem.NETBSD -> OperatingSystemFamily.BSD
      OperatingSystem.LINUX, OperatingSystem.MACOS -> OperatingSystemFamily.UNIX
      OperatingSystem.WINDOWS -> OperatingSystemFamily.WINDOWS
      OperatingSystem.UNKNOWN -> OperatingSystemFamily.UNKNOWN
    }
  val architecture =
    when (architectureName) {
      "amd64", "x86_64" -> Architecture.X86_64
      "aarch64", "arm64" -> Architecture.AARCH64
      "arm", "armv7", "armv7h" -> Architecture.ARMV7H
      "riscv64" -> Architecture.RISCV64
      else -> Architecture.UNKNOWN
    }
  return Host(os, family, architecture)
}
