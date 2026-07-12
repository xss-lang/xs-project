// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// A small in-memory order service using ownership, Arc, Mutex, channels and generics.

module Programs.InventoryService;

imports Collections, Stdio, Thread, Sync, Result;

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
    lines: STD.Collections.vector<OrderLine>;
}

data Receipt {
    orderId: Int;
    accepted: Bool;
    message: Str;
}

interface Repository<K, V> {
    fn Get(key: K) => Result.Result<&V, ServiceError>;
    fn Put(key: K, value: V);
}

class InventoryRepository {
    implements Repository<Str, Product>;

    products: STD.Collections.hash_map<Str, Product>;

    InventoryRepository() {
        this.products = STD.Collections.hash_map<Str, Product>.new();
    }

    fn Get(key: Str) => Result.Result<&Product, ServiceError> {
        if (!this.products.contains(key)) {
            return Result.Error(ServiceError.UnknownProduct(key));
        }
        return Result.Ok(&this.products[key]);
    }

    fn Put(key: Str, value: Product) {
        this.products[key] = value;
    }

    fn Reserve(line: OrderLine) => Result.Result<Void, ServiceError> {
        product: &mut Product = &mut this.products[line.sku];

        if (product.stock < line.quantity) {
            return Result.Error(ServiceError.NotEnoughStock(line.sku));
        }

        product.stock -= line.quantity;
        return Result.Ok();
    }
}

class OrderWorker {
    inventory: Arc<Mutex<InventoryRepository>>;

    OrderWorker(inventory: Arc<Mutex<InventoryRepository>>) {
        this.inventory = inventory;
    }

    fn Process(order: Order) => Receipt {
        guard: Mutex<InventoryRepository> = this.inventory.lock();

        for (line: OrderLine in order.lines) {
            result: Result.Result<Void, ServiceError> = (*guard).Reserve(line);
            match (result) {
                Result.Ok(else) -> {},
                Result.Error(error) -> {
                    return Receipt {
                        orderId: order.id,
                        accepted: false,
                        message: error.ToString(),
                    };
                },
            }
        }

        return Receipt {
            orderId: order.id,
            accepted: true,
            message: "accepted",
        };
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
    lines: STD.Collections.vector<OrderLine> = STD.Collections.vector<OrderLine>.new();
    lines.push(OrderLine {
        sku: sku,
        quantity: quantity,
    });

    return Order {
        id: id,
        lines: lines,
    };
}

fn Main() => Result.Result<Void, Result.Error> {
    inventory: Arc<Mutex<InventoryRepository>> = SeedInventory();

    (orders, receipts): Thread.channel<Order> = Thread.channel<Order>();
    (results, resultReader): Thread.channel<Receipt> = Thread.channel<Receipt>();

    workerInventory: Arc<Mutex<InventoryRepository>> = Arc.clone(&inventory);

    Thread.spawn(move fn() {
        worker: OrderWorker = new(workerInventory);

        while (true) {
            maybeOrder: Result.Result<Order, SyncException> = receipts.recv();
            if (maybeOrder.isError()) {
                break;
            }
            receipt: Receipt = worker.Process(maybeOrder.unwrap());
            results.send(receipt)@;
        }
    });

    orders.send(MakeOrder(1, "book", 2));
    orders.send(MakeOrder(2, "mug", 8));
    orders.close();

    while (true) {
        maybeReceipt: Result.Result<Receipt, SyncException> = resultReader.recv();
        if (maybeReceipt.isError()) {
            break;
        }
        receipt: Receipt = maybeReceipt.unwrap();
        println!(
            "order #{} accepted={} message={}",
            receipt.orderId,
            receipt.accepted,
            receipt.message
        );
    }

    return Result.Ok();
}
