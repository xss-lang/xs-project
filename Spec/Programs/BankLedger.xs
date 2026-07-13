// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// A small two-sided ledger with nominal money and audit records.

module programs::bank_ledger;

imports collections, process;

enum data LedgerError {
    UnknownAccount: Str,
    InsufficientFunds: Str,
    InvalidAmount: Int,
}

data Money {
    cents: Int;
    currency: Str;
}

data Account {
    id: Str;
    owner: Str;
    balance: Money;
}

data Transfer {
    source_account: Str;
    target_account: Str;
    amount: Money;
    memo: Optional<Str>;
}

class Ledger {
    accounts: std::collections::HashMap<Str, Account>;
    audit: std::collections::Vector<Transfer>;

    Ledger() {
        self.accounts = std::collections::HashMap<Str, Account>::new();
        self.audit = std::collections::Vector<Transfer>::new();
    }

    fn open(id: Str, owner: Str, balance: Money) {
        self.accounts[id] = Account {
            id: id,
            owner: owner,
            balance: balance,
        };
    }

    fn apply(transfer: Transfer) -> Result<()> {
        if (transfer.amount.cents <= 0) {
            return Error(Error {
                message: "invalid transfer amount",
            });
        }

        from_account: &mut Account = self.account_mut(transfer.source_account)@;
        to_account: &mut Account = self.account_mut(transfer.target_account)@;

        if (from_account.balance.cents < transfer.amount.cents) {
            return Error(Error {
                message: "insufficient funds",
            });
        }

        from_account.balance.cents -= transfer.amount.cents;
        to_account.balance.cents += transfer.amount.cents;
        self.audit.push(transfer);
        return Ok();
    }

    fn account_mut(id: Str) -> Result<&mut Account, Error> {
        if (!self.accounts.contains(id)) {
            return Error(Error {
                message: "unknown account",
            });
        }
        return Ok(&mut self.accounts[id]);
    }

    fn print() -> Result<()> {
        for ((id, account): (Str, Account) in self.accounts) {
            println!("{} {} {}", id, account.owner, account.balance.cents);
        }
        return Ok();
    }
}

fn main() -> Result<Int, Error> {
    ledger := new Ledger();

    ledger.open("checking", "Ada", Money {
        cents: 50'000,
        currency: "USD",
    });
    ledger.open("savings", "Ada", Money {
        cents: 0,
        currency: "USD",
    });

    ledger.apply(Transfer {
        source_account: "checking",
        target_account: "savings",
        amount: Money {
            cents: 12'500,
            currency: "USD",
        },
        memo: std::optional::Some("monthly savings"),
    })@;

    ledger.print()@;
    return Ok(0);
}
