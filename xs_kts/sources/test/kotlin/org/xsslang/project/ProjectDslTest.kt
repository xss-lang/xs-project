/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

package org.xsslang.project

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
    assertFailsWith<ProjectConfigurationException> { glob.build() }
  }

  @Test
  fun testRuntimeUsesJava25() {
    requireJava25()
    assertEquals(25, Runtime.version().feature())
  }
}
