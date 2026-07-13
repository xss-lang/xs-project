/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

package org.xsslang.project

import java.nio.charset.StandardCharsets
import java.nio.file.Files
import java.nio.file.Path
import kotlin.io.path.isRegularFile

object ProjectOutput {
  fun emit(plan: ProjectPlan) {
    when (System.getProperty("xs.project.output", "plan")) {
      "plan" -> println(PlanWriter.write(plan))
      "sources0" -> writeSources(plan)
      else -> throw ProjectConfigurationException("unknown project output mode")
    }
  }

  private fun writeSources(plan: ProjectPlan) {
    val root = Path.of(System.getProperty("xs.project.root")).toAbsolutePath().normalize()
    val excludes = plan.sourceExcludes.map(::globRegex)
    val sources =
      plan.sourceIncludes
        .map { include -> root.resolve(include).normalize() }
        .onEach { path ->
          if (!path.startsWith(
              root,
            )
          ) {
            throw ProjectConfigurationException("source include escapes the project root: $path")
          }
          if (!path.isRegularFile()) throw ProjectConfigurationException("included source file does not exist: $path")
        }.filter { path -> excludes.none { matcher -> matcher.matches(relative(root, path)) } }
        .distinct()
        .sortedWith(compareBy<Path> { path -> path.fileName.toString() != "main.xs" }.thenBy(Path::toString))
    if (sources.none { path -> path.fileName.toString() == "main.xs" }) {
      throw ProjectConfigurationException("source exclusions removed required main.xs")
    }
    val configuredOutput = System.getProperty("xs.project.sources")?.takeIf(String::isNotBlank)
    val output = configuredOutput?.let { path -> Files.newOutputStream(Path.of(path)) } ?: System.out
    output.useIfOwned(configuredOutput != null) { stream ->
      sources.forEach { path ->
        stream.write(path.toString().toByteArray(StandardCharsets.UTF_8))
        stream.write(0)
      }
      stream.flush()
    }
  }

  private fun relative(
    root: Path,
    path: Path,
  ) = root.relativize(path).toString().replace('\\', '/')

  private fun globRegex(pattern: String): Regex {
    val result = StringBuilder("^")
    var index = 0
    while (index < pattern.length) {
      val character = pattern[index]
      when {
        character == '*' && index + 2 < pattern.length && pattern[index + 1] == '*' && pattern[index + 2] == '/' -> {
          result.append("(?:.*/)?")
          index += 3
        }

        character == '*' && index + 1 < pattern.length && pattern[index + 1] == '*' -> {
          result.append(".*")
          index += 2
        }

        character == '*' -> {
          result.append("[^/]*")
          index++
        }

        character == '?' -> {
          result.append("[^/]")
          index++
        }

        character in ".+()^$|{}[]\\" -> {
          result.append('\\').append(character)
          index++
        }

        else -> {
          result.append(character)
          index++
        }
      }
    }
    return Regex(result.append('$').toString())
  }

  private inline fun java.io.OutputStream.useIfOwned(
    owned: Boolean,
    block: (java.io.OutputStream) -> Unit,
  ) {
    if (owned) use(block) else block(this)
  }
}
