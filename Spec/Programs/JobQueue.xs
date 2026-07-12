// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Demonstrates a small async job queue with Result and Optional values.

module Programs.JobQueue;

imports Collections, Optional, Process, Result, Stdio, Sync;

enum data JobError {
    EmptyQueue,
    Failed: Str,
}

data Job {
    id: Int;
    command: Str;
    retries: Int;
}

class Queue {
    jobs: STD.Collections.vector<Job>;

    Queue() {
        this.jobs = STD.Collections.vector<Job>.new();
    }

    fn Push(job: Job) {
        this.jobs.push(job);
    }

    fn Pop() => Optional<Job> {
        if (this.jobs.length() == 0) {
            return STD.Optional.None;
        }

        return STD.Optional.Some(this.jobs.remove(0));
    }
}

class Worker {
    async fn Run(queue: &mut Queue) => Result.Result<Int, JobError> {
        completed: Int = 0;

        loop {
            maybeJob: Optional<Job> = queue.Pop();
            if (maybeJob == None) {
                return Result.Ok(completed);
            }

            job: Job = maybeJob!;
            result: Result.Result<Void, JobError> = await Worker.Execute(job);
            result@;
            completed += 1;
        }
    }

    static async fn Execute(job: Job) => Result.Result<Void, JobError> {
        status: Int = await STD.Process.run(job.command);
        if (status != 0) {
            return Result.Error(JobError.Failed(job.command));
        }

        return Result.Ok();
    }
}

fn Main() => Int throws IOException {
    queue: Queue = new();
    queue.Push(Job { id: 1, command: "compile", retries: 0 });
    queue.Push(Job { id: 2, command: "test", retries: 0 });

    completed: Int = await Worker.Run(&mut queue).expect("worker failed");
    println!("completed {} jobs", completed);
    return 0;
}
