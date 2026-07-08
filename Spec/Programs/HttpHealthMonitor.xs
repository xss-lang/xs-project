// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Concurrently checks HTTP endpoints and prints a compact health report.

module Programs.HttpHealthMonitor;

imports Http, Stdio, Collections, Thread, Sync;

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
    fn Report(result: HealthResult) throws IOException;
}

class ConsoleReporter implements HealthReporter {
    fn Report(result: HealthResult) throws IOException {
        state: Str = result.ok ? "OK" : "FAIL";
        std.cout
            << state
            << " "
            << result.endpoint.name
            << " status="
            << result.statusCode
            << " preview=\""
            << result.bodyPreview
            << "\"\n";
    }
}

class HealthClient {
    client: Http.client;

    HealthClient() {
        this.client = new();
    }

    async fn Check(endpoint: Endpoint) => Task<HealthResult> throws HealthError {
        request: Http.request = new()
            .uri(URI.create(endpoint.url))
            .header("Accept", "text/plain")
            .build();

        response: Http.response<Str> =
            await this.client.sendAsync(
                request,
                HttpResponse.BodyHandlers.ofstr()
            );

        body: Str = response.body();
        preview: Str = body.length() > 80 ? body.substring(0, 80) : body;

        return HealthResult {
            endpoint: endpoint,
            ok: response.statusCode() >= 200 && response.statusCode() < 300,
            statusCode: response.statusCode(),
            bodyPreview: preview,
        };
    }
}

async fn CheckAll(
    endpoints: Collections.vector<Endpoint>,
    reporter: HealthReporter
) => Task<Int> throws HealthError, IOException {
    client: HealthClient = new();
    tasks: Collections.vector<Task<HealthResult>> = Collections.vector<Task<HealthResult>>.new();

    for (endpoint: Endpoint in endpoints) {
        tasks.push(client.Check(endpoint));
    }

    failures: Int = 0;
    for (task: Task<HealthResult> in tasks) {
        result: HealthResult = await task;
        reporter.Report(result);

        if (!result.ok) {
            failures += 1;
        }
    }

    return failures;
}

fn DefaultEndpoints() => Collections.vector<Endpoint> {
    endpoints: Collections.vector<Endpoint> = Collections.vector<Endpoint>.new();

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

async fn Main() => Task<Int> {
    reporter: ConsoleReporter = new();

    try {
        failures: Int = await CheckAll(DefaultEndpoints(), reporter);
        if (failures == 0) {
            std.cout << "All endpoints are healthy.\n";
        } else {
            std.cerr << failures << " endpoint(s) failed.\n";
        }
        return failures;
    }
    catch (error: HealthError) {
        std.cerr << "Health check failed: " << error.ToString() << "\n";
        return 1;
    }
}
