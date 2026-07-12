// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// Aggregates asynchronous sensor readings with Optional values and cancellation.

module Programs.SensorStream;

imports Collections, Stdio, Thread, Sync;

enum data SensorError {
    Disconnected: Str,
    Cancelled,
}

data Reading {
    sensorId: Str;
    value: Float;
    unit: Str;
}

data Average {
    sensorId: Str;
    count: Int;
    value: Float;
}

class Sensor {
    id: Str;
    unit: Str;

    Sensor(id: Str, unit: Str) {
        this.id = id;
        this.unit = unit;
    }

    async fn Read() => Task<Optional<Reading>> throws SensorError {
        sample: Optional<Float> = await Hardware.readFloat(this.id);

        return sample?.Map(fn(value: Float) => Reading {
            sensorId: this.id,
            value: value,
            unit: this.unit,
        });
    }
}

class Aggregator {
    totals: STD.Collections.hash_map<Str, Float>;
    counts: STD.Collections.hash_map<Str, Int>;

    Aggregator() {
        this.totals = STD.Collections.hash_map<Str, Float>.new();
        this.counts = STD.Collections.hash_map<Str, Int>.new();
    }

    fn Add(reading: Reading) {
        this.totals[reading.sensorId] = (this.totals[reading.sensorId] ?? 0.0) + reading.value;
        this.counts[reading.sensorId] = (this.counts[reading.sensorId] ?? 0) + 1;
    }

    fn Averages() => STD.Collections.vector<Average> {
        result: STD.Collections.vector<Average> = STD.Collections.vector<Average>.new();

        for ((sensorId, total): (Str, Float) in this.totals) {
            count: Int = this.counts[sensorId]!;
            result.push(Average {
                sensorId: sensorId,
                count: count,
                value: total / Float.From(count),
            });
        }

        return result;
    }
}

async fn Main() => Task<Int> throws SensorError, IOException {
    sensors: STD.Collections.vector<Sensor> = STD.Collections.vector<Sensor>.of(
        Sensor.new("temperature", "C"),
        Sensor.new("humidity", "%"),
        Sensor.new("pressure", "Pa")
    );
    aggregator: Aggregator = new();
    cancellation: CancellationToken = CancellationToken.timeout(5.seconds());

    while (!cancellation.isCancelled()) {
        for (sensor: Sensor in sensors) {
            reading: Optional<Reading> = await sensor.Read();
            if (reading != None) {
                aggregator.Add(reading!);
            }
        }
    }

    for (average: Average in aggregator.Averages()) {
        println!("{} average={}", average.sensorId, average.value);
    }

    return 0;
}
