/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

package org.xsslang.project

import java.io.File
import java.nio.file.Files
import java.nio.file.Path
import kotlin.io.path.writeText
import kotlin.system.exitProcess

private const val MINIMUM_JAVA = 25

private data class ProjectFiles(
  val root: File,
  val project: File?,
  val settings: File?,
  val build: File?,
)

private fun usage() =
  "usage: xs-project evaluate [project-root]\n       xs-project sources0 <project-root> <output-file>"

internal fun isSupportedJavaFeature(actual: Int) = actual >= MINIMUM_JAVA

internal fun requireSupportedJava() {
  val actual = Runtime.version().feature()
  if (!isSupportedJavaFeature(actual)) {
    throw ProjectConfigurationException("JRE 25 or newer is required; found Java $actual")
  }
}

private fun filesAt(root: File): ProjectFiles? {
  val project = root.resolve("xs.project.kts").takeIf(File::isFile)
  val settings = root.resolve("xs.settings.kts").takeIf(File::isFile)
  val build = root.resolve("xs.build.kts").takeIf(File::isFile)
  if (project == null && settings == null && build == null) return null
  if (project != null && (settings != null || build != null)) {
    throw ProjectConfigurationException("xs.project.kts cannot be combined with split project files")
  }
  if (project != null) return ProjectFiles(root, project, null, null)
  if (settings == null || build == null) {
    throw ProjectConfigurationException("split projects require both xs.settings.kts and xs.build.kts")
  }
  return ProjectFiles(root, null, settings, build)
}

private fun discover(input: File): ProjectFiles {
  if (input.isFile) {
    return filesAt(input.absoluteFile.parentFile.canonicalFile)
      ?: throw ProjectConfigurationException("no X# Kotlin project file found beside $input")
  }
  var root: File? = input.canonicalFile
  while (root != null) {
    filesAt(root)?.let { files -> return files }
    root = root.parentFile
  }
  throw ProjectConfigurationException("no X# Kotlin project file found from ${input.canonicalFile}")
}

private fun existingClasspath(): String =
  System
    .getProperty("java.class.path")
    .split(File.pathSeparator)
    .filter { entry -> File(entry).exists() }
    .joinToString(File.pathSeparator)

private fun runKotlin(
  script: File,
  root: File,
  output: String,
  state: Path?,
  sourcesOutput: Path?,
): Int {
  val directory = Files.createTempDirectory("xs-project-kts-")
  return try {
    val wrapped = directory.resolve(script.name)
    val suffix =
      if (output == "state") {
        "\nProjectStateFile.writeConfigured(ProjectRuntime.snapshot())\n"
      } else {
        "\nProjectOutput.emit(ProjectRuntime.build())\n"
      }
    wrapped.writeText("import org.xsslang.project.*\n" + script.readText() + suffix)
    val kotlin = System.getenv("XS_KOTLIN")?.takeIf(String::isNotBlank) ?: "kotlin"
    val arguments =
      mutableListOf(
        kotlin,
        "-Dxs.project.root=${root.absolutePath}",
        "-Dxs.project.output=$output",
      )
    if (state != null) arguments += "-Dxs.project.state=$state"
    if (sourcesOutput != null) arguments += "-Dxs.project.sources=$sourcesOutput"
    arguments +=
      listOf(
        "-classpath",
        existingClasspath(),
        "-howtorun",
        "script",
        wrapped.toString(),
      )
    try {
      ProcessBuilder(arguments)
        .apply { environment()["JAVA_HOME"] = System.getProperty("java.home") }
        .inheritIO()
        .start()
        .waitFor()
    } catch (error: java.io.IOException) {
      throw ProjectConfigurationException("kotlin is required and could not be started: ${error.message}")
    }
  } finally {
    directory.toFile().deleteRecursively()
  }
}

internal fun evaluateWithKotlin(
  input: File,
  output: String = "plan",
  sourcesOutput: Path? = null,
): Int {
  requireSupportedJava()
  val files = discover(input)
  files.project?.let { project -> return runKotlin(project, files.root, output, null, sourcesOutput) }
  val stateDirectory = Files.createTempDirectory("xs-project-state-")
  return try {
    val state = stateDirectory.resolve("state.bin")
    val settingsStatus = runKotlin(files.settings!!, files.root, "state", state, null)
    if (settingsStatus != 0) settingsStatus else runKotlin(files.build!!, files.root, output, state, sourcesOutput)
  } finally {
    stateDirectory.toFile().deleteRecursively()
  }
}

fun main(args: Array<String>) {
  val validEvaluate = args.size in 1..2 && args[0] == "evaluate"
  val validSources = args.size == 3 && args[0] == "sources0"
  if (!validEvaluate && !validSources) {
    System.err.println(usage())
    exitProcess(2)
  }
  val input = File(args.getOrElse(1) { "." })
  try {
    val status =
      if (args[0] ==
        "evaluate"
      ) {
        evaluateWithKotlin(input)
      } else {
        evaluateWithKotlin(input, "sources0", Path.of(args[2]))
      }
    exitProcess(status)
  } catch (error: ProjectConfigurationException) {
    System.err.println("xs-project: ${error.message}")
    exitProcess(1)
  }
}
