// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Reads CSV sales records, groups them by region, and prints totals.

module Programs::CsvAnalytics;

imports collections, stdio, fs, process, result;

enum data CsvError {
    Io: Error,
    BadRow: Str,
}

data Sale {
    region: Str;
    product: Str;
    quantity: Int;
    revenue: Float;
}

data RegionTotal {
    region: Str;
    quantity: Int;
    revenue: Float;
}

class CsvParser {
    static fn ParseLine(line: Str) -> Result<Sale, Error> {
        fields: std::collections::vector<Str> = line.split(",");
        if (fields.length() != 4) {
            return Error(Error {
                message: "bad CSV row",
            });
        }

        return Ok(Sale {
            region: fields[0],
            product: fields[1],
            quantity: Int::Parse(fields[2]),
            revenue: Float::Parse(fields[3]),
        });
    }
}

class Analytics {
    totals: std::collections::hash_map<Str, RegionTotal>;

    Analytics() {
        self.totals = std::collections::hash_map<Str, RegionTotal>::new();
    }

    fn Add(sale: Sale) {
        if (!self.totals.contains(sale.region)) {
            self.totals[sale.region] = RegionTotal {
                region: sale.region,
                quantity: 0,
                revenue: 0.0,
            };
        }

        self.totals[sale.region].quantity += sale.quantity;
        self.totals[sale.region].revenue += sale.revenue;
    }

    fn Print() -> Result<()> {
        for ((else, total): (Str, RegionTotal) in self.totals) {
            println!(
                "{}: units={} revenue={}",
                total.region,
                total.quantity,
                total.revenue
            );
        }
        return Ok();
    }
}

fn LoadSales(path: Str) -> Result<std::collections::vector<Sale>, Error> {
    rows: std::collections::vector<Sale> = std::collections::vector<Sale>::new();
    content: Str = std::fs::read_to_str(path);

    for (line: Str in content.lines().skip(1)) {
        if (line.length() == 0) {
            continue;
        }
        rows.push(CsvParser::ParseLine(line)@);
    }

    return Ok(rows);
}

fn Main(args: std::collections::vector<Str>) -> Result<Int, Error> {
    path: Str = if (args.length() > 1) {
        args[1];
    }
    else {
        "sales.csv";
    };
    analytics: Analytics = new();

    for (sale: Sale in LoadSales(path)@) {
        analytics.Add(sale);
    }

    analytics.Print()@;
    return Ok(0);
}
