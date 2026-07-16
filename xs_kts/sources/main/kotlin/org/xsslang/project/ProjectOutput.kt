/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

package org.xsslang.project

import java.nio.charset.StandardCharsets
import java.nio.file.Files
import java.nio.file.Path
import kotlin.io.path.isRegularFile
import kotlin.streams.toList

object ProjectOutput {
  fun emit(plan: ProjectPlan) {
    val resolved = resolveSources(plan)
    when (System.getProperty("xs.project.output", "plan")) {
      "plan" -> println(PlanWriter.write(plan))
      "sources0" -> writeSources(resolved, plan.compiler, plan.variables)
      else -> throw ProjectConfigurationException("unknown project output mode")
    }
  }

  fun emitModules(state: ProjectState) {
    val root = Path.of(System.getProperty("xs.project.root")).toAbsolutePath().normalize()
    val extension = sourceExtension(state.variables)
    val modules = resolveModules(root, state.moduleIncludes, state.moduleSources, extension)
    writeSources(ResolvedProject(emptyList(), modules, emptyList()), state.compiler, state.variables)
  }

  private data class ResolvedProject(
    val sources: List<Path>,
    val modules: List<Pair<String, Path>>,
    val tests: List<Path>,
  )

  private fun resolveSources(plan: ProjectPlan): ResolvedProject {
    val root = Path.of(System.getProperty("xs.project.root")).toAbsolutePath().normalize()
    val extension = sourceExtension(plan.variables)
    val excludes = plan.sourceExcludes.map(::globRegex)
    val sources =
      plan.sourceIncludes
        .flatMap { include -> expandSourceDirectory(root, include, extension) }
        .filter { path -> excludes.none { matcher -> matcher.matches(relative(root, path)) } }
        .distinct()
        .sortedWith(compareBy<Path> { path -> path.fileName.toString() != "main.$extension" }.thenBy(Path::toString))
    val mainCount = sources.count { path -> path.fileName.toString() == "main.$extension" }
    if (mainCount != 1) {
      throw ProjectConfigurationException(
        "source roots must resolve exactly one case-sensitive main.$extension; found $mainCount",
      )
    }
    val modules = resolveModules(root, plan.moduleIncludes, plan.moduleSources, extension)
    val testExcludes = plan.testExcludes.map(::globRegex)
    val tests =
      plan.testIncludes
        .flatMap { include -> expandSourceDirectory(root, include, extension) }
        .filter { path -> testExcludes.none { matcher -> matcher.matches(relative(root, path)) } }
        .distinct()
        .sortedBy(Path::toString)
    val sourceSet = sources.toSet()
    val overlap = modules.firstOrNull { (_, path) -> path in sourceSet }
    if (overlap != null) {
      throw ProjectConfigurationException("source and module registries overlap: ${relative(root, overlap.second)}")
    }
    return ResolvedProject(sources, modules, tests)
  }

  private fun resolveModules(
    root: Path,
    configuredRoots: List<String>,
    moduleSources: List<ModuleSource>,
    extension: String,
  ): List<Pair<String, Path>> {
    val override = System.getProperty("xs.project.moduleRoot")?.takeIf(String::isNotBlank)
    val roots = if (configuredRoots.isEmpty() && override != null) listOf(override) else configuredRoots
    if (moduleSources.isNotEmpty() && roots.isEmpty()) {
      throw ProjectConfigurationException("module sources require module.include(...) or the --module option")
    }
    val modulePool = roots.flatMap { include -> expandSourceDirectory(root, include, extension) }.distinct()
    val assigned = mutableSetOf<Path>()
    val modules =
      moduleSources.flatMap { source ->
        expandInclude(root, source.path, extension).map { path ->
          if (path !in modulePool) {
            throw ProjectConfigurationException("module member is not selected by module.include(...): ${source.path}")
          }
          if (!assigned.add(path)) {
            throw ProjectConfigurationException("module member is assigned more than once: ${relative(root, path)}")
          }
          source.moduleName to path
        }
      }
    val unassigned = modulePool.filterNot(assigned::contains)
    if (unassigned.isNotEmpty()) {
      throw ProjectConfigurationException("module source has no xs.module.kts membership: ${relative(root, unassigned.first())}")
    }
    return modules.sortedWith(compareBy<Pair<String, Path>> { it.first }.thenBy { it.second.toString() })
  }

  private fun expandInclude(
    root: Path,
    include: String,
    extension: String,
  ): List<Path> {
    val normalized = include.replace('\\', '/')
    val includePath = Path.of(normalized)
    if (includePath.isAbsolute || normalized.split('/').any { component -> component == ".." }) {
      throw ProjectConfigurationException("source include escapes the project root: $include")
    }
    if (normalized.none { character -> character in "*?" }) {
      val path = root.resolve(includePath).normalize()
      if (!path.startsWith(root) || !path.isRegularFile() || !path.fileName.toString().endsWith(".$extension")) {
        throw ProjectConfigurationException("included source file does not exist: $path")
      }
      return listOf(path)
    }
    val matcher = globRegex(normalized)
    val matches =
      Files.walk(root).use { paths ->
        paths
          .filter(Path::isRegularFile)
          .filter { path -> matcher.matches(relative(root, path)) }
          .filter { path -> path.fileName.toString().endsWith(".$extension") }
          .toList()
      }
    if (matches.isEmpty()) throw ProjectConfigurationException("source include pattern matched no files: $include")
    return matches
  }

  private fun expandSourceDirectory(
    root: Path,
    include: String,
    extension: String,
  ): List<Path> {
    val normalized = include.replace('\\', '/')
    val includePath = Path.of(normalized)
    if (includePath.isAbsolute || normalized.split('/').any { component -> component == ".." }) {
      throw ProjectConfigurationException("source root escapes the project root: $include")
    }
    val directory = root.resolve(includePath).normalize()
    if (!directory.startsWith(root) || !Files.isDirectory(directory)) {
      throw ProjectConfigurationException("source root directory does not exist: $directory")
    }
    return Files.walk(directory).use { paths ->
      paths
        .filter(Path::isRegularFile)
        .filter { path -> path.fileName.toString().endsWith(".$extension") }
        .toList()
    }
  }

  private fun sourceExtension(variables: Map<String, List<String>>): String {
    val values = variables["XS_EXTENSION"] ?: listOf("xs")
    if (values.size != 1 || !values.single().matches(Regex("[A-Za-z0-9]+"))) {
      throw ProjectConfigurationException("XS_EXTENSION must be one extension without a leading dot")
    }
    return values.single()
  }

  private fun writeSources(
    project: ResolvedProject,
    compiler: CompilerSettings,
    variables: Map<String, List<String>>,
  ) {
    val configuredOutput = System.getProperty("xs.project.sources")?.takeIf(String::isNotBlank)
    val output = configuredOutput?.let { path -> Files.newOutputStream(Path.of(path)) } ?: System.out
    output.useIfOwned(configuredOutput != null) { stream ->
      writeRecord(stream, "xs-project-sources-v3")
      writeRecord(stream, compiler.warningLevel.name.lowercase())
      writeRecord(stream, compiler.warningsAsErrors.toString())
      writeRecord(stream, compiler.verbose.toString())
      writeRecord(stream, xgcEnabled(variables).toString())
      writeRecord(stream, project.sources.size.toString())
      writeRecord(stream, project.modules.size.toString())
      project.sources.forEach { path ->
        writeRecord(stream, path.toString())
      }
      project.modules.forEach { (name, path) ->
        writeRecord(stream, name)
        writeRecord(stream, path.toString())
      }
      stream.flush()
    }
  }

  private fun xgcEnabled(variables: Map<String, List<String>>): Boolean {
    val values = variables["XGC_ENABLED"] ?: listOf("false")
    if (values.size != 1 || values.single() !in setOf("true", "false")) {
      throw ProjectConfigurationException("XGC_ENABLED must be exactly true or false")
    }
    return values.single().toBooleanStrict()
  }

  private fun writeRecord(
    stream: java.io.OutputStream,
    value: String,
  ) {
    stream.write(value.toByteArray(StandardCharsets.UTF_8))
    stream.write(0)
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
