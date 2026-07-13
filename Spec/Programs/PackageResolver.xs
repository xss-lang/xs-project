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
        self.packages = std::collections::hash_map<Str, Package>::new();
        self.states = std::collections::hash_map<Str, VisitState>::new();
        self.ordered = std::collections::vector<Package>::new();
    }

    fn Add(package: Package) {
        self.packages[package.name] = package;
        self.states[package.name] = VisitState::White;
    }

    fn Resolve(root: Str) -> Result<std::collections::vector<Package>, ResolveError> {
        self.Visit(root)@;
        return Ok(self.ordered);
    }

    fn Visit(name: Str) -> Result<(), ResolveError> {
        if (!self.packages.contains(name)) {
            return Error(ResolveError::UnknownPackage(name));
        }

        state: VisitState = self.states[name];
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

        self.states[name] = VisitState::Gray;
        package: Package = self.packages[name];

        for (dependency: Str in package.dependencies) {
            self.Visit(dependency)@;
        }

        self.states[name] = VisitState::Black;
        self.ordered.push(package);
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
