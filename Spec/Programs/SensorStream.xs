// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Aggregates asynchronous sensor readings with Optional values and cancellation.

module programs::sensor_stream;

imports collections, hardware, thread, sync;

data Reading {
    sensor_id: Str;
    value: Float;
    unit: Str;
}

data Average {
    sensor_id: Str;
    count: Int;
    value: Float;
}

class Sensor {
    id: Str;
    unit: Str;

    Sensor(id: Str, unit: Str) {
        self.id = id;
        self.unit = unit;
    }

    async fn read() -> Task<Result<Optional<Reading>, Error>> {
        sample: Optional<Float> = await hardware::read_float(self.id);

        return Ok(sample?.map(fn(value) {
            Reading {
                sensor_id: self.id,
                value: value,
                unit: self.unit,
            }
        }));
    }
}

class Aggregator {
    totals: std::collections::HashMap<Str, Float>;
    counts: std::collections::HashMap<Str, Int>;

    Aggregator() {
        self.totals = std::collections::HashMap<Str, Float>::new();
        self.counts = std::collections::HashMap<Str, Int>::new();
    }

    fn add(reading: Reading) {
        self.totals[reading.sensor_id] = (self.totals[reading.sensor_id] ?? 0.0) + reading.value;
        self.counts[reading.sensor_id] = (self.counts[reading.sensor_id] ?? 0) + 1;
    }

    fn averages() -> std::collections::Vector<Average> {
        result: std::collections::Vector<Average> = std::collections::Vector<Average>::new();

        for ((sensor_id, total): (Str, Float) in self.totals) {
            count: Int = self.counts[sensor_id]!;
            result.push(Average {
                sensor_id: sensor_id,
                count: count,
                value: total / Float::from(count),
            });
        }

        return result;
    }
}

async fn main() -> Task<Result<Int, Error>> {
    sensors: std::collections::Vector<Sensor> = std::collections::Vector<Sensor>::of(
        new Sensor("temperature", "C"),
        new Sensor("humidity", "%"),
        new Sensor("pressure", "Pa")
    );
    aggregator: Aggregator = new Aggregator();
    cancellation: std::sync::CancellationToken = std::sync::CancellationToken::timeout(5.seconds());

    while (!cancellation.is_cancelled()) {
        for (sensor: Sensor in sensors) {
            reading: Optional<Reading> = await sensor.read()@;
            if (reading != None) {
                aggregator.add(reading!);
            }
        }
    }

    for (average: Average in aggregator.averages()) {
        println!("{} average={}", average.sensor_id, average.value);
    }

    return Ok(0);
}
