// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Resolves package dependencies with a topological order and cycle diagnostics.

module Programs.PackageResolver;

imports Collections, Stdio, Process, Result;

enum data ResolveError {
    UnknownPackage: Str,
    Cycle: Str,
}

enum VisitState {
    White,
    Gray,
    Black,
}

data Package {
    name: Str;
    version: Str;
    dependencies: STD.Collections.vector<Str>;
}

class PackageGraph {
    packages: STD.Collections.hash_map<Str, Package>;
    states: STD.Collections.hash_map<Str, VisitState>;
    ordered: STD.Collections.vector<Package>;

    PackageGraph() {
        this.packages = STD.Collections.hash_map<Str, Package>.new();
        this.states = STD.Collections.hash_map<Str, VisitState>.new();
        this.ordered = STD.Collections.vector<Package>.new();
    }

    fn Add(package: Package) {
        this.packages[package.name] = package;
        this.states[package.name] = VisitState.White;
    }

    fn Resolve(root: Str) => Result.Result<STD.Collections.vector<Package>, ResolveError> {
        this.Visit(root)@;
        return Result.Ok(this.ordered);
    }

    fn Visit(name: Str) => Result.Result<Void, ResolveError> {
        if (!this.packages.contains(name)) {
            return Result.Error(ResolveError.UnknownPackage(name));
        }

        state: VisitState = this.states[name];
        match (state) {
            VisitState.Black -> {
                return;
            },
            VisitState.Gray -> {
                return Result.Error(ResolveError.Cycle(name));
            },
            VisitState.White -> {
            },
        }

        this.states[name] = VisitState.Gray;
        package: Package = this.packages[name];

        for (dependency: Str in package.dependencies) {
            this.Visit(dependency)@;
        }

        this.states[name] = VisitState.Black;
        this.ordered.push(package);
        return Result.Ok();
    }
}

fn PackageOf(name: Str, version: Str, dependencies: STD.Collections.vector<Str>) => Package {
    return Package {
        name: name,
        version: version,
        dependencies: dependencies,
    };
}

fn Main() => Result.Result<Int, Result.Error> {
    graph: PackageGraph = new();

    graph.Add(PackageOf("app", "1.0.0", STD.Collections.vector<Str>.of("net", "json")));
    graph.Add(PackageOf("net", "2.1.0", STD.Collections.vector<Str>.of("runtime")));
    graph.Add(PackageOf("json", "3.0.0", STD.Collections.vector<Str>.of("runtime")));
    graph.Add(PackageOf("runtime", "1.4.0", STD.Collections.vector<Str>.new()));

    for (package: Package in graph.Resolve("app")@) {
        println!("{}@{}", package.name, package.version);
    }

    return Result.Ok(0);
}
