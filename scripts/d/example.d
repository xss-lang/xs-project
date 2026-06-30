#!/usr/bin/env rdmd --compiler=ldc2

import std.stdio;

void main(string[] args) {
  writeln("Hello D");
  writeln("Hello Codex");

  if (args.length > 1) {
    writeln("Argument found: ", args[1]);
  } else {
    writeln("No argument given");
  }

  if (args.length > 1 && args[1] == "codex") {
    writeln("Codex argument selected");
  }

  writeln("Done");
}
