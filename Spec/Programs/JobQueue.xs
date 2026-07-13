// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Demonstrates a small async job queue with Result and Optional values.

module programs::job_queue;

imports collections, optional, process, sync;

data Job {
    id: Int;
    command: Str;
    retries: Int;
}

class Queue {
    jobs: std::collections::Vector<Job>;

    Queue() {
        self.jobs = std::collections::Vector<Job>::new();
    }

    fn push(job: Job) {
        self.jobs.push(job);
    }

    fn pop() -> Optional<Job> {
        if (self.jobs.length() == 0) {
            return None;
        }

        return Some(self.jobs.remove(0));
    }
}

class Worker {
    async fn run(queue: &mut Queue) -> Result<Int, Error> {
        completed: Int = 0;

        loop {
            maybe_job: Optional<Job> = queue.pop();
            if (maybe_job == None) {
                return Ok(completed);
            }

            job: Job = maybe_job!;
            result: Result<()> = await Worker::execute(job);
            result@;
            completed += 1;
        }
    }

    static async fn execute(job: Job) -> Result<()> {
        status: Int = await std::process::run(job.command);
        if (status != 0) {
            return Error(new Error("job failed"));
        }

        return Ok();
    }
}

fn main() -> Result<Int, Error> {
    queue: Queue = new Queue();
    queue.push(Job { id: 1, command: "compile", retries: 0 });
    queue.push(Job { id: 2, command: "test", retries: 0 });

    completed: Int = await Worker::run(&mut queue)@;
    println!("completed {} jobs", completed);
    return Ok(0);
}
