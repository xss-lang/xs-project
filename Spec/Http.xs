// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Http standard library module:

//
// The Http module provides synchronous and asynchronous HTTP requests.
//
// Confirmed features:
//
// - Http.client
// - Http.request
// - Http.response<T>
// - URI.create(...)
// - Request builder methods
// - String request bodies
// - String response body handlers
// - Synchronous send(...)
// - Asynchronous sendAsync(...)
// - NetworkException
//
// No additional HTTP API is defined in this file.
//

imports Http, Stdio;


// ============================================================
// Synchronous request
// ============================================================

fn Main() throws NetworkException {
    client: Http.client = new();

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

    response: Http.response<Str> =
        client.send(
            request,
            HttpResponse.BodyHandlers.ofstr()
        );

    print!("{}", response.body());
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
// String request body
// ============================================================

fn StringRequestBody() {
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

fn SendRequest() throws NetworkException {
    client: Http.client = new();

    request: Http.request = new()
        .uri(URI.create("https://example.com"))
        .build();

    response: Http.response<Str> =
        client.send(
            request,
            HttpResponse.BodyHandlers.ofstr()
        );

    body: Str = response.body();
}


// client.send(...):
//
// - Sends the request synchronously.
// - Blocks until a response is available.
// - Returns Http.response<Str> when used with ofstr().
// - May throw NetworkException.


// ============================================================
// Asynchronous send
// ============================================================

async fn SendRequestAsync() => Task<()> {
    client: Http.client = new();

    request: Http.request = new()
        .uri(URI.create("https://example.com"))
        .build();

    response: Http.response<Str> =
        await client.sendAsync(
            request,
            Http.Response.BodyHandlers.ofstr()
        );

    print!("{}", response.body());
}


// client.sendAsync(...):
//
// - Sends the request asynchronously.
// - Returns Task<Http.response<Str>> when used with ofstr().
// - Must be awaited according to Task<T> rules.
// - A network failure is handled through NetworkException.


// ============================================================
// Response
// ============================================================

fn ReadResponseBody(
    response: Http.response<Str>
) => Str {
    return response.body();
}


// Http.response<T>:
//
// - Represents an HTTP response.
// - body() returns the response body as T.


// ============================================================
// Body handlers
// ============================================================

fn StringBodyHandler() {
    handler = HttpResponse.BodyHandlers.ofstr();
}


// Confirmed string body-handler spellings appearing in HTTP code:
//
// - HttpResponse.BodyHandlers.ofstr()
// - Http.Response.BodyHandlers.ofstr()
//
// This file preserves both spellings exactly as provided.
// No equivalence rule is inferred.


// ============================================================
// Exceptions
// ============================================================

fn CatchNetworkException() {
    try {
        client: Http.client = new();

        request: Http.request = new()
            .uri(URI.create("https://example.com"))
            .build();

        response: Http.response<Str> =
            client.send(
                request,
                HttpResponse.BodyHandlers.ofstr()
            );
    }
    catch (error: NetworkException) {
        eprintln!("Network request failed");
    }
}


// NetworkException:
//
// - Represents HTTP or network request failure.
// - May be caught with catch.
// - Synchronous HTTP functions may declare
//   `throws NetworkException`.


// ============================================================
// Confirmed type summary
// ============================================================

//
// Http.client
// Http.request
// Http.response<T>
// Task<Http.response<Str>>
// NetworkException
//