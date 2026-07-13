// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Concurrently checks HTTP endpoints and prints a compact health report.

module Programs::HttpHealthMonitor;

imports http, std, collections, thread, sync, result;

enum data HealthError {
    Network: NetworkException,
    Timeout: Str,
}

data Endpoint {
    name: Str;
    url: Str;
}

data HealthResult {
    endpoint: Endpoint;
    ok: Bool;
    statusCode: Int;
    bodyPreview: Str;
}

interface HealthReporter {
    fn Report(result: HealthResult) -> Result<(), IOException>;
}

class ConsoleReporter : HealthReporter {

    fn Report(result: HealthResult) -> Result<(), IOException> {
        state: Str = if (result.ok) {
            "OK";
        }
        else {
            "FAIL";
        };
        println!(
            "{} {} status={} preview=\"{}\"",
            state,
            result.endpoint.name,
            result.statusCode,
            result.bodyPreview
        );
        return Ok();
    }
}

class HealthClient {
    client: Http::client;

    HealthClient() {
        this.client = new();
    }

    async fn Check(endpoint: Endpoint) -> Task<Result<HealthResult, HealthError>> {
        request: Http.request = new()
            .uri(URI.create(endpoint.url))
            .header("Accept", "text/plain")
            .build();

        response: Http::response<Str> =
            await this.client.sendAsync(
                request,
                HttpResponse::BodyHandlers.ofstr()
            );

        body: Str = response.body();
        preview: Str = if (body.length() > 80) {
            body.substring(0, 80);
        }
        else {
            body;
        };

        return Ok(HealthResult {
            endpoint: endpoint,
            ok: response.statusCode() >= 200 && response.statusCode() < 300,
            statusCode: response.statusCode(),
            bodyPreview: preview,
        });
    }
}

async fn CheckAll(
    endpoints: std::collections::vector<Endpoint>,
    reporter: HealthReporter
) -> Task<Result<Int, Error>> {
    client: HealthClient = new();
    tasks: std::collections::vector<Task<Result<HealthResult, HealthError>>> = std::collections::vector<Task<Result<HealthResult, HealthError>>>::new();

    for (endpoint: Endpoint in endpoints) {
        tasks.push(client.Check(endpoint));
    }

    failures: Int = 0;
    for (task: Task<HealthResult> in tasks) {
        result: HealthResult = await task@;
        reporter.Report(result)@;

        if (!result.ok) {
            failures += 1;
        }
    }

    return Ok(failures);
}

fn DefaultEndpoints() -> std::collections::vector<Endpoint> {
    endpoints: std::collections::vector<Endpoint> = std::collections::vector<Endpoint>::new();

    endpoints.push(Endpoint {
        name: "example",
        url: "https://example.com",
    });

    endpoints.push(Endpoint {
        name: "httpbin",
        url: "https://httpbin.org/get",
    });

    endpoints.push(Endpoint {
        name: "iana",
        url: "https://www.iana.org/domains/reserved",
    });

    return endpoints;
}

async fn Main() -> Task<Int> {
    reporter: ConsoleReporter = new();

    result: Result<Int, Error> = await CheckAll(DefaultEndpoints(), reporter);
    match (result) {
        Ok(failures) -> {
            if (failures == 0) {
                println!("All endpoints are healthy.");
            } else {
                eprintln!("{} endpoint(s) failed.", failures);
            }
            return failures;
        },
        Error(error) -> {
            eprintln!("Health check failed: {}", error.ToString());
            return 1;
        },
    }
}
