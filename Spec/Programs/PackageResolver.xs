// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Resolves package dependencies with a topological order and cycle diagnostics.

module Programs::PackageResolver;

imports collections, std, process, result;

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
    dependencies: std::collections::vector<Str>;
}

class PackageGraph {
    packages: std::collections::hash_map<Str, Package>;
    states: std::collections::hash_map<Str, VisitState>;
    ordered: std::collections::vector<Package>;

    PackageGraph() {
        this.packages = std::collections::hash_map<Str, Package>::new();
        this.states = std::collections::hash_map<Str, VisitState>::new();
        this.ordered = std::collections::vector<Package>::new();
    }

    fn Add(package: Package) {
        this.packages[package.name] = package;
        this.states[package.name] = VisitState::White;
    }

    fn Resolve(root: Str) -> Result<std::collections::vector<Package>, ResolveError> {
        this.Visit(root)@;
        return Ok(this.ordered);
    }

    fn Visit(name: Str) -> Result<(), ResolveError> {
        if (!this.packages.contains(name)) {
            return Error(ResolveError::UnknownPackage(name));
        }

        state: VisitState = this.states[name];
        match (state) {
            VisitState::Black -> {
                return;
            },
            VisitState::Gray -> {
                return Error(ResolveError::Cycle(name));
            },
            VisitState::White -> {
            },
        }

        this.states[name] = VisitState::Gray;
        package: Package = this.packages[name];

        for (dependency: Str in package.dependencies) {
            this.Visit(dependency)@;
        }

        this.states[name] = VisitState::Black;
        this.ordered.push(package);
        return Ok();
    }
}

fn PackageOf(name: Str, version: Str, dependencies: std::collections::vector<Str>) -> Package {
    return Package {
        name: name,
        version: version,
        dependencies: dependencies,
    };
}

fn Main() -> Result<Int, Error> {
    graph: PackageGraph = new();

    graph.Add(PackageOf("app", "1.0.0", std::collections::vector<Str>.of("net", "json")));
    graph.Add(PackageOf("net", "2.1.0", std::collections::vector<Str>.of("runtime")));
    graph.Add(PackageOf("json", "3.0.0", std::collections::vector<Str>.of("runtime")));
    graph.Add(PackageOf("runtime", "1.4.0", std::collections::vector<Str>::new()));

    for (package: Package in graph.Resolve("app")@) {
        println!("{}@{}", package.name, package.version);
    }

    return Ok(0);
}
