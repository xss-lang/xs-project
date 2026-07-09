// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// A small in-memory order service using ownership, Arc, Mutex, channels and generics.

module Programs.InventoryService;

imports Collections, Stdio, Thread, Sync;

enum data ServiceError {
    UnknownProduct: Str,
    NotEnoughStock: Str,
    Closed: Str,
}

data Product {
    sku: Str;
    name: Str;
    stock: Int;
}

data OrderLine {
    sku: Str;
    quantity: Int;
}

data Order {
    id: Int;
    lines: Collections.vector<OrderLine>;
}

data Receipt {
    orderId: Int;
    accepted: Bool;
    message: Str;
}

interface Repository<K, V> {
    fn Get(key: K) => &V throws ServiceError;
    fn Put(key: K, value: V);
}

class InventoryRepository {
    implements Repository<Str, Product>;

    products: Collections.hashmap<Str, Product>;

    InventoryRepository() {
        this.products = Collections.hashmap<Str, Product>.new();
    }

    fn Get(key: Str) => &Product throws ServiceError {
        if (!this.products.contains(key)) {
            throw ServiceError.UnknownProduct(key);
        }
        return &this.products[key];
    }

    fn Put(key: Str, value: Product) {
        this.products[key] = value;
    }

    fn Reserve(line: OrderLine) throws ServiceError {
        product: &mut Product = &mut this.products[line.sku];

        if (product.stock < line.quantity) {
            throw ServiceError.NotEnoughStock(line.sku);
        }

        product.stock -= line.quantity;
    }
}

class OrderWorker {
    inventory: Arc<Mutex<InventoryRepository>>;

    OrderWorker(inventory: Arc<Mutex<InventoryRepository>>) {
        this.inventory = inventory;
    }

    fn Process(order: Order) => Receipt {
        guard: Mutex<InventoryRepository> = this.inventory.lock();

        try {
            for (line: OrderLine in order.lines) {
                (*guard).Reserve(line);
            }

            return Receipt {
                orderId: order.id,
                accepted: true,
                message: "accepted",
            };
        }
        catch (error: ServiceError) {
            return Receipt {
                orderId: order.id,
                accepted: false,
                message: error.ToString(),
            };
        }
    }
}

fn SeedInventory() => Arc<Mutex<InventoryRepository>> {
    repository: InventoryRepository = new();

    repository.Put("book", Product {
        sku: "book",
        name: "X# Handbook",
        stock: 10,
    });

    repository.Put("mug", Product {
        sku: "mug",
        name: "Compiler Mug",
        stock: 5,
    });

    return Arc.new(Mutex.new(repository));
}

fn MakeOrder(id: Int, sku: Str, quantity: Int) => Order {
    lines: Collections.vector<OrderLine> = Collections.vector<OrderLine>.new();
    lines.push(OrderLine {
        sku: sku,
        quantity: quantity,
    });

    return Order {
        id: id,
        lines: lines,
    };
}

fn Main() throws IOException {
    inventory: Arc<Mutex<InventoryRepository>> = SeedInventory();

    (orders, receipts): Thread.channel<Order> = Thread.channel<Order>();
    (results, resultReader): Thread.channel<Receipt> = Thread.channel<Receipt>();

    workerInventory: Arc<Mutex<InventoryRepository>> = Arc.clone(&inventory);

    Thread.spawn(move fn() {
        worker: OrderWorker = new(workerInventory);

        while (true) {
            try {
                order: Order = receipts.recv();
                receipt: Receipt = worker.Process(order);
                results.send(receipt);
            }
            catch (error: SyncException) {
                break;
            }
        }
    });

    orders.send(MakeOrder(1, "book", 2));
    orders.send(MakeOrder(2, "mug", 8));
    orders.close();

    while (true) {
        try {
            receipt: Receipt = resultReader.recv();
            println!(
                "order #{} accepted={} message={}",
                receipt.orderId,
                receipt.accepted,
                receipt.message
            );
        }
        catch (error: SyncException) {
            break;
        }
    }
}
