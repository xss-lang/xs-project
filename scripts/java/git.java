import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

class XsGit
{
  static final int USAGE_ERROR = 2;
  static final String[] GENERATED_PATHS = {
      "build/",
      ".agents/",
      ".codex",
      "target/",
      "node_modules/",
      "dist/",
      "out/",
      "Cargo.lock",
      ":(glob)**/build/**",
      ":(glob)**/target/**",
      ":(glob)**/node_modules/**",
      ":(glob)**/dist/**",
      ":(glob)**/out/**",
      ":(glob)**/Cargo.lock"
  };

  enum Command
  {
    UPDATE,
    UNCOM,
    HELP
  }

  record Cli(Command command, String message)
  {
  }

  static int run(String... command) throws IOException, InterruptedException
  {
    System.err.println("+ " + String.join(" ", command));

    return new ProcessBuilder(command)
        .inheritIO()
        .start()
        .waitFor();
  }

  static int runQuiet(String... command) throws IOException, InterruptedException
  {
    return new ProcessBuilder(command)
        .redirectOutput(ProcessBuilder.Redirect.DISCARD)
        .redirectError(ProcessBuilder.Redirect.INHERIT)
        .start()
        .waitFor();
  }

  static String capture(String... command) throws IOException, InterruptedException
  {
    Process process = new ProcessBuilder(command)
        .redirectError(ProcessBuilder.Redirect.INHERIT)
        .start();

    ByteArrayOutputStream output = new ByteArrayOutputStream();

    try (var input = process.getInputStream()) {
      input.transferTo(output);
    }

    int code = process.waitFor();

    if (code != 0) {
      throw new RuntimeException(String.join(" ", command) + " failed with code " + code);
    }

    return output.toString(StandardCharsets.UTF_8);
  }

  static void exit(int code, String message)
  {
    System.err.println(message);
    System.exit(code);
  }

  static void help()
  {
    System.out.println("""
        usage:
          git update "Commit message"
          git uncom
          git help

        commands:
          update   Run safe git add, then commit with the given message.
          uncom    Show uncommitted changes.
          help     Show this help.

        update excludes:
          build/, target/, node_modules/, dist/, out/, Cargo.lock

        examples:
          git update "Fix parser"
          git uncom
          git help
        """);
  }

  static Cli parse(String[] args)
  {
    if (args.length == 1 && args[0].equals("help")) {
      return new Cli(Command.HELP, null);
    }

    if (args.length == 1 && args[0].equals("uncom")) {
      return new Cli(Command.UNCOM, null);
    }

    if (args.length == 2 && args[0].equals("update")) {
      return new Cli(Command.UPDATE, args[1]);
    }

    help();
    System.exit(USAGE_ERROR);

    throw new IllegalStateException("unreachable");
  }

  static void requireGitRepo() throws IOException, InterruptedException
  {
    String result = capture("git", "rev-parse", "--is-inside-work-tree").trim();

    if (!result.equals("true")) {
      exit(1, "error: not inside a git work tree");
    }
  }

  static void update(String message) throws IOException, InterruptedException
  {
    int addCode = run("git", "add", "--all");

    if (addCode != 0) {
      exit(addCode, "error: git add --all failed");
    }

    String[] resetCommand = new String[4 + GENERATED_PATHS.length];
    resetCommand[0] = "git";
    resetCommand[1] = "reset";
    resetCommand[2] = "--quiet";
    resetCommand[3] = "--";

    for (int i = 0; i < GENERATED_PATHS.length; ++i) {
      resetCommand[i + 4] = GENERATED_PATHS[i];
    }

    int resetCode = run(resetCommand);

    if (resetCode != 0) {
      exit(resetCode, "error: generated artifact paths could not be removed from the index");
    }

    int diffCode = runQuiet("git", "diff", "--cached", "--quiet");

    if (diffCode == 0) {
      System.err.println("nothing to commit");
      return;
    }

    if (diffCode != 1) {
      exit(diffCode, "error: git diff --cached --quiet failed");
    }

    int commitCode = run("git", "commit", "--message", message);

    if (commitCode != 0) {
      exit(commitCode, "error: git commit failed");
    }
  }

  static void uncom() throws IOException, InterruptedException
  {
    String status = capture("git", "status", "--short");

    if (status.isBlank()) {
      System.out.println("nothing uncommitted");
      return;
    }

    System.out.print(status);
  }

  public static void main(String[] args) throws Exception
  {
    Cli cli = parse(args);

    if (cli.command() == Command.HELP) {
      help();
      return;
    }

    requireGitRepo();

    switch (cli.command()) {
      case UPDATE -> update(cli.message());
      case UNCOM -> uncom();
      case HELP -> help();
    }
  }
}
