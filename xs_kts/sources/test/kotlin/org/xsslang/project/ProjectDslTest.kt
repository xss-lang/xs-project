/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

package org.xsslang.project

import java.nio.charset.StandardCharsets
import java.nio.file.Files
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue

class ProjectDslTest {
  @Test
  fun buildsConditionalProjectPlan() {
    val context = ProjectContext(Host(OperatingSystem.LINUX, OperatingSystemFamily.UNIX, Architecture.X86_64))
    context.project("Demo", "BETA", "0.1.0")
    if (context.host.os == OperatingSystem.LINUX) context.set("PLATFORM", "linux")
    context.authors(arrayOf("Leitwolf", "leitwolf@example.me"))
    context.sources {
      include("sources/main.xs")
      include("sources/library.xs")
      exclude("sources/tests/**")
    }
    context.compiler {
      warnings("all")
      werror(true)
      verbose(true)
    }
    val plan = context.build()
    assertEquals("linux", plan.variables["PLATFORM"])
    assertEquals("LLVM", plan.variables["XS_BACKEND"])
    assertEquals(WarningLevel.ALL, plan.compiler.warningLevel)
    assertTrue(PlanWriter.write(plan).startsWith("{\"format\":\"xs-project-plan\",\"version\":0"))
  }

  @Test
  fun requiresProjectAndSourceInclude() {
    val context = ProjectContext()
    context.project("Demo", "BETA", "0.1.0")
    val error = assertFailsWith<ProjectConfigurationException> { context.build() }
    assertTrue(error.message.orEmpty().contains("sources.include"))
  }

  @Test
  fun requiresOneExplicitMainSource() {
    val missingMain =
      ProjectContext().apply {
        project("Demo", "BETA", "0.1.0")
        sources { include("sources/library.xs") }
      }
    assertFailsWith<ProjectConfigurationException> { missingMain.build() }

    val glob =
      ProjectContext().apply {
        project("Demo", "BETA", "0.1.0")
        sources { include("sources/**/*.xs") }
      }
    assertEquals(listOf("sources/**/*.xs"), glob.build().sourceIncludes)
  }

  @Test
  fun testRuntimeUsesJava25() {
    requireJava25()
    assertEquals(25, Runtime.version().feature())
  }

  @Test
  fun bsdBelongsToBothBsdAndUnixFamilies() {
    val host =
      Host(
        OperatingSystem.FREEBSD,
        OperatingSystemFamily.forOperatingSystem(OperatingSystem.FREEBSD),
        Architecture.X86_64,
      )
    assertTrue(host.family == OperatingSystemFamily.BSD)
    assertTrue(host.family == OperatingSystemFamily.UNIX)
  }

  @Test
  fun linuxAndMacosAreUnixButNotBsd() {
    val linux = OperatingSystemFamily.forOperatingSystem(OperatingSystem.LINUX)
    val macos = OperatingSystemFamily.forOperatingSystem(OperatingSystem.MACOS)
    assertTrue(linux == OperatingSystemFamily.UNIX)
    assertTrue(macos == OperatingSystemFamily.UNIX)
    assertTrue(linux != OperatingSystemFamily.BSD)
    assertTrue(macos != OperatingSystemFamily.BSD)
  }

  @Test
  fun expandsSourceGlobsWithMainFirstAndExcludes() {
    val root = Files.createTempDirectory("xs-project-glob-test-")
    val sources = Files.createDirectories(root.resolve("sources"))
    val tests = Files.createDirectories(sources.resolve("tests"))
    Files.writeString(sources.resolve("main.xs"), "fn main() -> Long { return helper(); }")
    Files.writeString(sources.resolve("helper.xs"), "fn helper() -> Long { return 0; }")
    Files.writeString(tests.resolve("ignored.xs"), "fn ignored() -> Long { return 1; }")
    val output = root.resolve("sources.bin")
    val context =
      ProjectContext().apply {
        project("Glob", "BETA", "0.1.0")
        sources {
          include("sources/**/*.xs")
          exclude("sources/tests/**")
        }
        compiler {
          warnings("all")
          werror(true)
          verbose(true)
        }
      }
    val oldRoot = System.getProperty("xs.project.root")
    val oldOutput = System.getProperty("xs.project.output")
    val oldSources = System.getProperty("xs.project.sources")
    try {
      System.setProperty("xs.project.root", root.toString())
      System.setProperty("xs.project.output", "sources0")
      System.setProperty("xs.project.sources", output.toString())
      ProjectOutput.emit(context.build())
      val paths =
        Files
          .readAllBytes(output)
          .toString(StandardCharsets.UTF_8)
          .split('\u0000')
          .filter(String::isNotEmpty)
      assertEquals(
        listOf("xs-project-sources-v1", "all", "true", "true"),
        paths.take(4),
      )
      assertEquals(listOf(sources.resolve("main.xs").toString(), sources.resolve("helper.xs").toString()), paths.drop(4))
    } finally {
      restoreProperty("xs.project.root", oldRoot)
      restoreProperty("xs.project.output", oldOutput)
      restoreProperty("xs.project.sources", oldSources)
      root.toFile().deleteRecursively()
    }
  }

  private fun restoreProperty(
    name: String,
    value: String?,
  ) {
    if (value == null) System.clearProperty(name) else System.setProperty(name, value)
  }
}
