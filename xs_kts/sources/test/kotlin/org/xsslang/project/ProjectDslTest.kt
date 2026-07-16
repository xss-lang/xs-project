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
  fun compilerPolicyDefaultsAreStable() {
    val settings = CompilerSettings()
    assertEquals(WarningLevel.MEDIUM, settings.warningLevel)
    assertTrue(!settings.warningsAsErrors)
    assertTrue(settings.verbose)
  }

  @Test
  fun splitFilesShareStateWithoutSectionOwnership() {
    val settings =
      ProjectContext().apply {
        source { include("sources") }
        compiler { warnings("low") }
      }
    val build =
      ProjectContext(state = settings.snapshot()).apply {
        project("Split", "BETA", "0.1.0")
        authors(arrayOf("Leitwolf", "leitwolf@example.me"))
      }
    val plan = build.build()
    assertEquals("Split", plan.identity.name)
    assertEquals(listOf("sources"), plan.sourceIncludes)
    assertEquals(WarningLevel.LOW, plan.compiler.warningLevel)
  }

  @Test
  fun buildsConditionalProjectPlan() {
    val context = ProjectContext(Host(OperatingSystem.LINUX, OperatingSystemFamily.UNIX, Architecture.X86_64))
    context.project("Demo", "BETA", "0.1.0")
    if (context.host.os == OperatingSystem.LINUX) context.set("MODE", "native")
    context.set(
      "TARGET",
      "x86_64-unknown-linux-gnu",
      "x86_64-unknown-linux-musl",
      "aarch64-unknown-linux-gnu",
      "armv7h-unknown-linux-gnueabihf",
      "riscv64gc-unknown-linux-gnu",
      "x86_64-apple-darwin",
      "aarch64-apple-darwin",
      "x86_64-pc-windows-msvc",
      "aarch64-pc-windows-msvc",
      "x86_64-unknown-freebsd",
      "aarch64-unknown-freebsd",
    )
    assertEquals("native", context.get("MODE"))
    assertEquals(
      listOf(
        "x86_64-unknown-linux-gnu",
        "x86_64-unknown-linux-musl",
        "aarch64-unknown-linux-gnu",
        "armv7h-unknown-linux-gnueabihf",
        "riscv64gc-unknown-linux-gnu",
        "x86_64-apple-darwin",
        "aarch64-apple-darwin",
        "x86_64-pc-windows-msvc",
        "aarch64-pc-windows-msvc",
        "x86_64-unknown-freebsd",
        "aarch64-unknown-freebsd",
      ),
      context.getAll("TARGET"),
    )
    assertEquals("Demo, BETA, 0.1.0", context.get("PROJECT"))
    assertEquals("LLVM", context.get("XS_BACKEND"))
    assertEquals("xs", context.get("XS_EXTENSION"))
    assertFailsWith<ProjectConfigurationException> { context.get("MISSING") }
    context.authors(arrayOf("Leitwolf", "leitwolf@example.me"))
    context.source {
      include("sources")
      exclude("sources/tests/**")
    }
    context.compiler {
      warnings("all")
      werror(true)
      verbose(true)
    }
    val plan = context.build()
    assertEquals(listOf("native"), plan.variables["MODE"])
    assertEquals(listOf("LLVM"), plan.variables["XS_BACKEND"])
    assertEquals(WarningLevel.ALL, plan.compiler.warningLevel)
    assertTrue(PlanWriter.write(plan).startsWith("{\"format\":\"xs-project-plan\",\"version\":0"))
  }

  @Test
  fun requiresProjectAndSourceInclude() {
    val context = ProjectContext()
    context.project("Demo", "BETA", "0.1.0")
    val error = assertFailsWith<ProjectConfigurationException> { context.build() }
    assertTrue(error.message.orEmpty().contains("source.include"))
  }

  @Test
  fun sourceAndModuleRootsRejectGlobs() {
    val sourceGlob =
      ProjectContext().apply {
        project("Demo", "BETA", "0.1.0")
      }
    assertFailsWith<ProjectConfigurationException> { sourceGlob.source { include("sources/**/*.xs") } }
    assertFailsWith<ProjectConfigurationException> { sourceGlob.module { include("Modules/*") } }
    assertFailsWith<ProjectConfigurationException> { sourceGlob.test { include("Tests/*.xs") } }
    sourceGlob.source { include("Sources") }
    sourceGlob.test {
      include("Tests")
      exclude("Tests/fixtures/**")
    }
    assertEquals(listOf("Tests/fixtures/**"), sourceGlob.build().testExcludes)
  }

  @Test
  fun customSourceExtensionReplacesXsDiscovery() {
    val root = Files.createTempDirectory("xs-project-extension-test-")
    val sources = Files.createDirectories(root.resolve("Sources"))
    val modules = Files.createDirectories(root.resolve("Modules"))
    val tests = Files.createDirectories(root.resolve("Tests"))
    Files.writeString(sources.resolve("main.xsharp"), "fn main() -> Long { return 0; }")
    Files.writeString(sources.resolve("ignored.xs"), "fn ignored() -> Long { return 1; }")
    Files.writeString(modules.resolve("math.xsharp"), "public fn add() -> Long { return 1; }")
    Files.writeString(modules.resolve("ignored.xs"), "public fn ignored() -> Long { return 2; }")
    Files.writeString(tests.resolve("smoke.xsharp"), "fn smoke() {}")
    val output = root.resolve("sources.bin")
    val context =
      ProjectContext().apply {
        project("Extension", "BETA", "0.1.0")
        set("XS_EXTENSION", "xsharp")
        source { include("Sources") }
        module { include("Modules") }
        module {
          name("Math")
          add("Modules/*.xsharp")
        }
        test { include("Tests") }
      }
    val oldRoot = System.getProperty("xs.project.root")
    val oldOutput = System.getProperty("xs.project.output")
    val oldSources = System.getProperty("xs.project.sources")
    try {
      System.setProperty("xs.project.root", root.toString())
      System.setProperty("xs.project.output", "sources0")
      System.setProperty("xs.project.sources", output.toString())
      ProjectOutput.emit(context.build())
      val records =
        Files
          .readAllBytes(output)
          .toString(StandardCharsets.UTF_8)
          .split('\u0000')
          .filter(String::isNotEmpty)
      assertEquals(listOf("xs-project-sources-v2", "medium", "false", "true", "1", "1"), records.take(6))
      assertEquals(
        listOf(sources.resolve("main.xsharp").toString(), "Math", modules.resolve("math.xsharp").toString()),
        records.drop(6),
      )
    } finally {
      restoreProperty("xs.project.root", oldRoot)
      restoreProperty("xs.project.output", oldOutput)
      restoreProperty("xs.project.sources", oldSources)
      root.toFile().deleteRecursively()
    }
  }

  @Test
  fun acceptsJava25AndNewer() {
    assertTrue(!isSupportedJavaFeature(24))
    assertTrue(isSupportedJavaFeature(25))
    assertTrue(isSupportedJavaFeature(26))
    requireSupportedJava()
    assertTrue(Runtime.version().feature() >= 25)
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
  fun reactOsIsNotWindowsButBelongsToTheWindowsFamily() {
    val os = operatingSystemFromName("ReactOS")
    val family = OperatingSystemFamily.forOperatingSystem(os)
    assertTrue(os != OperatingSystem.WINDOWS)
    assertTrue(family == OperatingSystemFamily.WINDOWS)
    assertTrue(family == OperatingSystem.WINDOWS)
    assertTrue(family != OperatingSystemFamily.UNIX)
    assertTrue(family != OperatingSystemFamily.BSD)
  }

  @Test
  fun expandsSourceDirectoriesRecursivelyWithMainFirstAndExcludes() {
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
        source {
          include("sources")
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
        listOf("xs-project-sources-v2", "all", "true", "true", "2", "0"),
        paths.take(6),
      )
      assertEquals(listOf(sources.resolve("main.xs").toString(), sources.resolve("helper.xs").toString()), paths.drop(6))
    } finally {
      restoreProperty("xs.project.root", oldRoot)
      restoreProperty("xs.project.output", oldOutput)
      restoreProperty("xs.project.sources", oldSources)
      root.toFile().deleteRecursively()
    }
  }

  @Test
  fun resolvesExplicitModuleMemberships() {
    val root = Files.createTempDirectory("xs-project-module-test-")
    val sources = Files.createDirectories(root.resolve("Sources"))
    val modules = Files.createDirectories(root.resolve("Modules/Example/Utils"))
    Files.writeString(sources.resolve("main.xs"), "import MyModule;\nfn main() -> Long { return 0; }")
    Files.writeString(modules.resolve("math.xs"), "public fn math(a: Int, b: Int) { a + b }")
    Files.writeString(modules.resolve("topla.xs"), "public fn topla(a: Int, b: Int) { a + b }")
    val output = root.resolve("sources.bin")
    val context =
      ProjectContext().apply {
        project("Modules", "BETA", "0.1.0")
        source { include("Sources") }
        module { include("Modules") }
        module {
          name("MyModule")
          add("Modules/Example/Utils/math*.xs")
          submodule {
            name("util")
            add("Modules/Example/Utils/topla.xs")
          }
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
      val records =
        Files
          .readAllBytes(output)
          .toString(StandardCharsets.UTF_8)
          .split('\u0000')
          .filter(String::isNotEmpty)
      assertEquals(listOf("xs-project-sources-v2", "medium", "false", "true", "1", "2"), records.take(6))
      assertEquals(sources.resolve("main.xs").toString(), records[6])
      assertEquals(
        listOf(
          "MyModule",
          modules.resolve("math.xs").toString(),
          "MyModule::util",
          modules.resolve("topla.xs").toString(),
        ),
        records.drop(7),
      )
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
