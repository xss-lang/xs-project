/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

package org.xsslang.project

object PlanWriter {
  fun write(plan: ProjectPlan): String =
    buildString {
      append("{\"format\":\"xs-project-plan\",\"version\":0")
      field("name", plan.identity.name)
      field("channel", plan.identity.channel)
      field("projectVersion", plan.identity.version)
      mapField("variables", plan.variables)
      append(",\"authors\":[")
      plan.authors.forEachIndexed { index, author ->
        if (index > 0) append(',')
        append("{\"name\":")
          .quoted(author.name)
          .append(",\"email\":")
          .quoted(author.email)
          .append('}')
      }
      append(']')
      arrayField("modules", plan.modules)
      arrayField("targets", plan.targets)
      arrayField("sourceIncludes", plan.sourceIncludes)
      arrayField("sourceExcludes", plan.sourceExcludes)
      arrayField("testIncludes", plan.testIncludes)
      append(",\"testFramework\":")
      plan.testFramework?.let { quoted(it) } ?: append("null")
      append(",\"compiler\":{\"warnings\":")
      quoted(
        plan.compiler.warningLevel.name
          .lowercase(),
      )
      append(",\"werror\":${plan.compiler.warningsAsErrors},\"verbose\":${plan.compiler.verbose}}}")
    }

  private fun StringBuilder.field(
    name: String,
    value: String,
  ) = append(",\"").append(name).append("\":").quoted(value)

  private fun StringBuilder.arrayField(
    name: String,
    values: List<String>,
  ) {
    append(",\"").append(name).append("\":[")
    values.forEachIndexed { index, value ->
      if (index > 0) append(',')
      quoted(value)
    }
    append(']')
  }

  private fun StringBuilder.mapField(
    name: String,
    values: Map<String, String>,
  ) {
    append(",\"").append(name).append("\":{")
    values.entries.forEachIndexed { index, entry ->
      if (index > 0) append(',')
      quoted(entry.key).append(':').quoted(entry.value)
    }
    append('}')
  }

  private fun StringBuilder.quoted(value: String): StringBuilder {
    append('"')
    value.forEach { character ->
      when (character) {
        '"' -> append("\\\"")
        '\\' -> append("\\\\")
        '\n' -> append("\\n")
        '\r' -> append("\\r")
        '\t' -> append("\\t")
        else -> if (character.code < 0x20) append("\\u%04x".format(character.code)) else append(character)
      }
    }
    return append('"')
  }
}
