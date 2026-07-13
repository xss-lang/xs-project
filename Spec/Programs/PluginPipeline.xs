// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Runs a configurable text-processing pipeline through nominal plugin types.

module Programs::PluginPipeline;

imports collections, std, process;

enum data PipelineError {
    Io: Error,
    InvalidStage: Str,
}

interface TextPlugin {
    fn Name() -> Str;
    fn Run(input: Str) -> Result<Str, PipelineError>;
}

class TrimPlugin : TextPlugin {

    fn Name() -> Str {
        return "trim";
    }

    fn Run(input: Str) -> Result<Str, PipelineError> {
        return Ok(input.trim());
    }
}

class UppercasePlugin : TextPlugin {

    fn Name() -> Str {
        return "upper";
    }

    fn Run(input: Str) -> Result<Str, PipelineError> {
        return Ok(input.uppercase());
    }
}

class ReplacePlugin : TextPlugin {

    fromText: Str;
    toText: Str;

    ReplacePlugin(fromText: Str, toText: Str) {
        self.fromText = fromText;
        self.toText = toText;
    }

    fn Name() -> Str {
        return "replace";
    }

    fn Run(input: Str) -> Result<Str, PipelineError> {
        return Ok(input.replace(self.fromText, self.toText));
    }
}

class Pipeline {
    stages: std::collections::Vector<TextPlugin>;

    Pipeline() {
        self.stages = std::collections::Vector<TextPlugin>::new();
    }

    fn Add(stage: TextPlugin) {
        self.stages.push(stage);
    }

    fn Run(input: Str) -> Result<Str, PipelineError> {
        output: Str = input;

        for (stage: TextPlugin in self.stages) {
            output = stage.Run(output)@;
        }

        return Ok(output);
    }
}

fn Main(args: std::collections::Vector<Str>) -> Result<Int, Error> {
    input: Str = if (args.length() > 1) {
        args[1];
    }
    else {
        "  hello x#  ";
    };
    pipeline: Pipeline = new();

    pipeline.Add(TrimPlugin::new());
    pipeline.Add(ReplacePlugin::new("x#", "X#"));
    pipeline.Add(UppercasePlugin::new());

    println!("{}", pipeline.Run(input)@);
    return Ok(0);
}
