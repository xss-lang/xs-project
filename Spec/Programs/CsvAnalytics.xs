// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Reads CSV sales records, groups them by region, and prints totals.

module Programs.CsvAnalytics;

imports Collections, Stdio, Fs, Process;

enum data CsvError {
    Io: IOException,
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
    static fn ParseLine(line: Str) => Sale throws CsvError {
        fields: Collections.vector<Str> = line.split(",");
        if (fields.length() != 4) {
            throw CsvError.BadRow(line);
        }

        return Sale {
            region: fields[0],
            product: fields[1],
            quantity: Int.Parse(fields[2]),
            revenue: Float.Parse(fields[3]),
        };
    }
}

class Analytics {
    totals: Collections.hashmap<Str, RegionTotal>;

    Analytics() {
        this.totals = Collections.hashmap<Str, RegionTotal>.new();
    }

    fn Add(sale: Sale) {
        if (!this.totals.contains(sale.region)) {
            this.totals[sale.region] = RegionTotal {
                region: sale.region,
                quantity: 0,
                revenue: 0.0,
            };
        }

        this.totals[sale.region].quantity += sale.quantity;
        this.totals[sale.region].revenue += sale.revenue;
    }

    fn Print() throws IOException {
        for ((_, total): (Str, RegionTotal) in this.totals) {
            println!(
                "{}: units={} revenue={}",
                total.region,
                total.quantity,
                total.revenue
            );
        }
    }
}

fn LoadSales(path: Str) => Collections.vector<Sale> throws CsvError, IOException {
    rows: Collections.vector<Sale> = Collections.vector<Sale>.new();
    content: Str = Fs.ReadToStr(path);

    for (line: Str in content.lines().skip(1)) {
        if (line.length() == 0) {
            continue;
        }
        rows.push(CsvParser.ParseLine(line));
    }

    return rows;
}

fn Main(args: Collections.vector<Str>) => Int throws CsvError, IOException {
    path: Str = if (args.length() > 1) {
        args[1];
    }
    else {
        "sales.csv";
    };
    analytics: Analytics = new();

    for (sale: Sale in LoadSales(path)) {
        analytics.Add(sale);
    }

    analytics.Print();
    return 0;
}
