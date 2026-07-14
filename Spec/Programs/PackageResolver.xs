// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Resolves package dependencies with a topological order and cycle diagnostics.

module programs::package_resolver;

imports collections, stdio, process;

enum VisitState {
    White,
    Gray,
    Black,
}

data Package {
    name: Str;
    version: Str;
    dependencies: std::collections::Vector<Str>;
}

class PackageGraph {
    packages: std::collections::HashMap<Str, Package>;
    states: std::collections::HashMap<Str, VisitState>;
    ordered: std::collections::Vector<Package>;

    PackageGraph() {
        self.packages = std::collections::HashMap<Str, Package>::new();
        self.states = std::collections::HashMap<Str, VisitState>::new();
        self.ordered = std::collections::Vector<Package>::new();
    }

    fn add(package: Package) {
        self.packages[package.name] = package;
        self.states[package.name] = VisitState::White;
    }

    fn resolve(root: Str) -> Result<std::collections::Vector<Package>, Error> {
        self.visit(root)@;
        return Ok(self.ordered);
    }

    fn visit(name: Str) -> Result<()> {
        if (!self.packages.contains(name)) {
            return Error(new Error("unknown package"));
        }

        state: VisitState = self.states[name];
        match (state) {
            VisitState::Black -> {
                return;
            },
            VisitState::Gray -> {
                return Error(new Error("dependency cycle"));
            },
            VisitState::White -> {
            },
        }

        self.states[name] = VisitState::Gray;
        package: Package = self.packages[name];

        for (dependency: Str in package.dependencies) {
            self.visit(dependency)@;
        }

        self.states[name] = VisitState::Black;
        self.ordered.push(package);
        return Ok();
    }
}

fn package_of(name: Str, version: Str, dependencies: std::collections::Vector<Str>) -> Package {
    return Package {
        name: name,
        version: version,
        dependencies: dependencies,
    };
}

fn main() -> Result<Int, Error> {
    graph: PackageGraph = new PackageGraph();

    graph.add(package_of("app", "1.0.0", std::collections::Vector<Str>::of("net", "json")));
    graph.add(package_of("net", "2.1.0", std::collections::Vector<Str>::of("runtime")));
    graph.add(package_of("json", "3.0.0", std::collections::Vector<Str>::of("runtime")));
    graph.add(package_of("runtime", "1.4.0", std::collections::Vector<Str>::new()));

    for (package: Package in graph.resolve("app")@) {
        println!("{}@{}", package.name, package.version);
    }

    return Ok(0);
}
