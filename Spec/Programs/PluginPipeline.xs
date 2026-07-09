// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Runs a configurable text-processing pipeline through nominal plugin types.

module Programs.PluginPipeline;

imports Collections, Stdio, Process;

enum data PipelineError {
    Io: IOException,
    InvalidStage: Str,
}

interface TextPlugin {
    fn Name() => Str;
    fn Run(input: Str) => Str throws PipelineError;
}

class TrimPlugin {
    implements TextPlugin;

    fn Name() => Str {
        return "trim";
    }

    fn Run(input: Str) => Str {
        return input.trim();
    }
}

class UppercasePlugin {
    implements TextPlugin;

    fn Name() => Str {
        return "upper";
    }

    fn Run(input: Str) => Str {
        return input.uppercase();
    }
}

class ReplacePlugin {
    implements TextPlugin;

    fromText: Str;
    toText: Str;

    ReplacePlugin(fromText: Str, toText: Str) {
        this.fromText = fromText;
        this.toText = toText;
    }

    fn Name() => Str {
        return "replace";
    }

    fn Run(input: Str) => Str {
        return input.replace(this.fromText, this.toText);
    }
}

class Pipeline {
    stages: Collections.vector<TextPlugin>;

    Pipeline() {
        this.stages = Collections.vector<TextPlugin>.new();
    }

    fn Add(stage: TextPlugin) {
        this.stages.push(stage);
    }

    fn Run(input: Str) => Str throws PipelineError {
        output: Str = input;

        for (stage: TextPlugin in this.stages) {
            output = stage.Run(output);
        }

        return output;
    }
}

fn Main(args: Collections.vector<Str>) => Int throws PipelineError, IOException {
    input: Str = if (args.length() > 1) {
        args[1];
    }
    else {
        "  hello x#  ";
    };
    pipeline: Pipeline = new();

    pipeline.Add(TrimPlugin.new());
    pipeline.Add(ReplacePlugin.new("x#", "X#"));
    pipeline.Add(UppercasePlugin.new());

    println!("{}", pipeline.Run(input));
    return 0;
}
