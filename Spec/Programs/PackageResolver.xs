// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Resolves package dependencies with a topological order and cycle diagnostics.

module Programs.PackageResolver;

imports Collections, Stdio, Process;

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
    dependencies: Collections.vector<Str>;
}

class PackageGraph {
    packages: Collections.hashmap<Str, Package>;
    states: Collections.hashmap<Str, VisitState>;
    ordered: Collections.vector<Package>;

    PackageGraph() {
        this.packages = Collections.hashmap<Str, Package>.new();
        this.states = Collections.hashmap<Str, VisitState>.new();
        this.ordered = Collections.vector<Package>.new();
    }

    fn Add(package: Package) {
        this.packages[package.name] = package;
        this.states[package.name] = VisitState.White;
    }

    fn Resolve(root: Str) => Collections.vector<Package> throws ResolveError {
        this.Visit(root);
        return this.ordered;
    }

    fn Visit(name: Str) throws ResolveError {
        if (!this.packages.contains(name)) {
            throw ResolveError.UnknownPackage(name);
        }

        state: VisitState = this.states[name];
        match (state) {
            VisitState.Black -> {
                return;
            },
            VisitState.Gray -> {
                throw ResolveError.Cycle(name);
            },
            VisitState.White -> {
            },
        }

        this.states[name] = VisitState.Gray;
        package: Package = this.packages[name];

        for (dependency: Str in package.dependencies) {
            this.Visit(dependency);
        }

        this.states[name] = VisitState.Black;
        this.ordered.push(package);
    }
}

fn PackageOf(name: Str, version: Str, dependencies: Collections.vector<Str>) => Package {
    return Package {
        name: name,
        version: version,
        dependencies: dependencies,
    };
}

fn Main() => Int throws ResolveError, IOException {
    graph: PackageGraph = new();

    graph.Add(PackageOf("app", "1.0.0", Collections.vector<Str>.of("net", "json")));
    graph.Add(PackageOf("net", "2.1.0", Collections.vector<Str>.of("runtime")));
    graph.Add(PackageOf("json", "3.0.0", Collections.vector<Str>.of("runtime")));
    graph.Add(PackageOf("runtime", "1.4.0", Collections.vector<Str>.new()));

    for (package: Package in graph.Resolve("app")) {
        std.cout << package.name << "@" << package.version << "\n";
    }

    return 0;
}
