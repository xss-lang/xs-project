// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Runs a configurable text-processing pipeline through nominal plugin types.

module programs::plugin_pipeline;

imports collections, stdio, process;

interface TextPlugin {
    fn name() -> Str;
    fn run(input: Str) -> Result<Str, Error>;
}

class TrimPlugin : TextPlugin {

    fn name() -> Str {
        return "trim";
    }

    fn run(input: Str) -> Result<Str, Error> {
        return Ok(input.trim());
    }
}

class UppercasePlugin : TextPlugin {

    fn name() -> Str {
        return "upper";
    }

    fn run(input: Str) -> Result<Str, Error> {
        return Ok(input.uppercase());
    }
}

class ReplacePlugin : TextPlugin {

    from_text: Str;
    to_text: Str;

    ReplacePlugin(from_text: Str, to_text: Str) {
        self.from_text = from_text;
        self.to_text = to_text;
    }

    fn name() -> Str {
        return "replace";
    }

    fn run(input: Str) -> Result<Str, Error> {
        return Ok(input.replace(self.from_text, self.to_text));
    }
}

class Pipeline {
    stages: std::collections::Vector<TextPlugin>;

    Pipeline() {
        self.stages = std::collections::Vector<TextPlugin>::new();
    }

    fn add(stage: TextPlugin) {
        self.stages.push(stage);
    }

    fn run(input: Str) -> Result<Str, Error> {
        output: Str = input;

        for (stage: TextPlugin in self.stages) {
            output = stage.run(output)@;
        }

        return Ok(output);
    }
}

fn main(args: std::collections::Vector<Str>) -> Result<Int, Error> {
    input: Str = if (args.length() > 1) {
        args[1]
    }
    else {
        "  hello x#  "
    };
    pipeline: Pipeline = new Pipeline();

    pipeline.add(new TrimPlugin());
    pipeline.add(new ReplacePlugin("x#", "X#"));
    pipeline.add(new UppercasePlugin());

    println!("{}", pipeline.run(input)@);
    return Ok(0);
}
