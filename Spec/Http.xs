// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Http standard library module:

//
// The Http module provides synchronous and asynchronous HTTP requests.
//
// Confirmed features:
//
// - Http::client
// - Http.request
// - Http::response<T>
// - URI.create(...)
// - Request builder methods
// - Str request bodies
// - Str response body handlers
// - Synchronous send(...)
// - Asynchronous sendAsync(...)
// - NetworkException error payloads through Result
//
// No additional HTTP API is defined in this file.
//

imports http, std, result;


// ============================================================
// Synchronous request
// ============================================================

fn Main() -> Result::Result<Void, NetworkException> {
    client: Http::client = new();

    request: Http.request = new()
        .uri(URI.create("https://httpbin.org/post"))
        .header("Content-Type", "application/json")
        .POST(
            HttpRequest.BodyPublishers.ofstr(
                """
                {
                    "name": "Alpha"
                }
                """
            )
        )
        .build();

    response: Http::response<Str> =
        client.send(
            request,
            HttpResponse.BodyHandlers.ofstr()
        )@;

    print!("{}", response.body());
    return Result::Ok();
}


// ============================================================
// Request builder
// ============================================================

fn BuildGetRequest() {
    request: Http.request = new()
        .uri(URI.create("https://example.com"))
        .build();
}


// Request construction uses a builder chain.
//
// Confirmed builder operations:
//
// - uri(...)
// - header(...)
// - POST(...)
// - build()


// ============================================================
// Str request body
// ============================================================

fn StrRequestBody() {
    request: Http.request = new()
        .uri(URI.create("https://httpbin.org/post"))
        .header("Content-Type", "application/json")
        .POST(
            HttpRequest.BodyPublishers.ofstr(
                """
                {
                    "name": "Alpha"
                }
                """
            )
        )
        .build();
}


// HttpRequest.BodyPublishers.ofstr(...):
//
// - Creates a request body from Str.
// - May receive a multiline string.


// ============================================================
// Synchronous send
// ============================================================

fn SendRequest() -> Result::Result<Void, NetworkException> {
    client: Http::client = new();

    request: Http.request = new()
        .uri(URI.create("https://example.com"))
        .build();

    response: Http::response<Str> =
        client.send(
            request,
            HttpResponse.BodyHandlers.ofstr()
        )@;

    body: Str = response.body();
    return Result::Ok();
}


// client.send(...):
//
// - Sends the request synchronously.
// - Blocks until a response is available.
// - Returns Http::response<Str> when used with ofstr().
// - Returns Result::Error(NetworkException) on network failure.


// ============================================================
// Asynchronous send
// ============================================================

async fn SendRequestAsync() -> Task<()> {
    client: Http::client = new();

    request: Http.request = new()
        .uri(URI.create("https://example.com"))
        .build();

    response: Http::response<Str> =
        await client.sendAsync(
            request,
            Http.Response.BodyHandlers.ofstr()
        );

    print!("{}", response.body());
}


// client.sendAsync(...):
//
// - Sends the request asynchronously.
// - Returns Task<Http::response<Str>> when used with ofstr().
// - Must be awaited according to Task<T> rules.
// - A network failure is represented as Result::Error(NetworkException).


// ============================================================
// Response
// ============================================================

fn ReadResponseBody(
    response: Http::response<Str>
) -> Str {
    return response.body();
}


// Http::response<T>:
//
// - Represents an HTTP response.
// - body() returns the response body as T.


// ============================================================
// Body handlers
// ============================================================

fn StrBodyHandler() {
    handler = HttpResponse.BodyHandlers.ofstr();
}


// Confirmed Str body-handler spellings appearing in HTTP code:
//
// - HttpResponse.BodyHandlers.ofstr()
// - Http.Response.BodyHandlers.ofstr()
//
// This file preserves both spellings exactly as provided.
// No equivalence rule is inferred.


// ============================================================
// Result error handling
// ============================================================

fn HandleNetworkError() -> Result::Result<Void, NetworkException> {
    client: Http::client = new();

    request: Http.request = new()
        .uri(URI.create("https://example.com"))
        .build();

    result: Result::Result<Http::response<Str>, NetworkException> =
        client.send(
            request,
            HttpResponse.BodyHandlers.ofstr()
        );

    match (result) {
        Result::Ok(response) -> {
            println!("{}", response.body());
        },
        Result::Error(error) -> {
            eprintln!("Network request failed");
            return Result::Error(error);
        },
    }

    return Result::Ok();
}


// NetworkException:
//
// - Represents HTTP or network request failure.
// - Is carried through Result::Error(NetworkException).
// - New code should not use legacy `throws` or `catch`.


// ============================================================
// Confirmed type summary
// ============================================================

//
// Http::client
// Http.request
// Http::response<T>
// Task<Http::response<Str>>
// NetworkException
//
