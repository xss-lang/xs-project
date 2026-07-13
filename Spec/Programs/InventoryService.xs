// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// A small in-memory order service using ownership, Arc, Mutex, channels and generics.

module Programs::InventoryService;

imports collections, std, thread, sync, result;

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
    lines: std::collections::vector<OrderLine>;
}

data Receipt {
    orderId: Int;
    accepted: Bool;
    message: Str;
}

interface Repository<K, V> {
    fn Get(key: K) -> Result<&V, ServiceError>;
    fn Put(key: K, value: V);
}

class InventoryRepository : Repository<Str, Product> {

    products: std::collections::hash_map<Str, Product>;

    InventoryRepository() {
        this.products = std::collections::hash_map<Str, Product>::new();
    }

    fn Get(key: Str) -> Result<&Product, ServiceError> {
        if (!this.products.contains(key)) {
            return Error(ServiceError::UnknownProduct(key));
        }
        return Ok(&this.products[key]);
    }

    fn Put(key: Str, value: Product) {
        this.products[key] = value;
    }

    fn Reserve(line: OrderLine) -> Result<(), ServiceError> {
        product: &mut Product = &mut this.products[line.sku];

        if (product.stock < line.quantity) {
            return Error(ServiceError::NotEnoughStock(line.sku));
        }

        product.stock -= line.quantity;
        return Ok();
    }
}

class OrderWorker {
    inventory: Arc<Mutex<InventoryRepository>>;

    OrderWorker(inventory: Arc<Mutex<InventoryRepository>>) {
        this.inventory = inventory;
    }

    fn Process(order: Order) -> Receipt {
        guard: Mutex<InventoryRepository> = this.inventory.lock();

        for (line: OrderLine in order.lines) {
            result: Result<(), ServiceError> = (*guard).Reserve(line);
            match (result) {
                Ok(else) -> {},
                Error(error) -> {
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

fn SeedInventory() -> Arc<Mutex<InventoryRepository>> {
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

    return Arc::new(Mutex::new(repository));
}

fn MakeOrder(id: Int, sku: Str, quantity: Int) -> Order {
    lines: std::collections::vector<OrderLine> = std::collections::vector<OrderLine>::new();
    lines.push(OrderLine {
        sku: sku,
        quantity: quantity,
    });

    return Order {
        id: id,
        lines: lines,
    };
}

fn Main() -> Result<(), Error> {
    inventory: Arc<Mutex<InventoryRepository>> = SeedInventory();

    (orders, receipts): Thread.channel<Order> = Thread.channel<Order>();
    (results, resultReader): Thread.channel<Receipt> = Thread.channel<Receipt>();

    workerInventory: Arc<Mutex<InventoryRepository>> = Arc.clone(&inventory);

    Thread.spawn(move fn() {
        worker: OrderWorker = new(workerInventory);

        while (true) {
            maybeOrder: Result<Order, SyncException> = receipts.recv();
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
        maybeReceipt: Result<Receipt, SyncException> = resultReader.recv();
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

    return Ok();
}
