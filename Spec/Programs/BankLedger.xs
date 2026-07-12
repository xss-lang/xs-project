// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// A small two-sided ledger with nominal money and audit records.

module Programs.BankLedger;

imports Collections, Stdio, Process;

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
    sourceAccount: Str;
    targetAccount: Str;
    amount: Money;
    memo: Optional<Str>;
}

class Ledger {
    accounts: STD.Collections.hash_map<Str, Account>;
    audit: STD.Collections.vector<Transfer>;

    Ledger() {
        this.accounts = STD.Collections.hash_map<Str, Account>.new();
        this.audit = STD.Collections.vector<Transfer>.new();
    }

    fn Open(id: Str, owner: Str, balance: Money) {
        this.accounts[id] = Account {
            id: id,
            owner: owner,
            balance: balance,
        };
    }

    fn Apply(transfer: Transfer) throws LedgerError {
        if (transfer.amount.cents <= 0) {
            throw LedgerError.InvalidAmount(transfer.amount.cents);
        }

        fromAccount: &mut Account = this.AccountMut(transfer.sourceAccount);
        toAccount: &mut Account = this.AccountMut(transfer.targetAccount);

        if (fromAccount.balance.cents < transfer.amount.cents) {
            throw LedgerError.InsufficientFunds(fromAccount.id);
        }

        fromAccount.balance.cents -= transfer.amount.cents;
        toAccount.balance.cents += transfer.amount.cents;
        this.audit.push(transfer);
    }

    fn AccountMut(id: Str) => &mut Account throws LedgerError {
        if (!this.accounts.contains(id)) {
            throw LedgerError.UnknownAccount(id);
        }
        return &mut this.accounts[id];
    }

    fn Print() throws IOException {
        for ((id, account): (Str, Account) in this.accounts) {
            println!("{} {} {}", id, account.owner, account.balance.cents);
        }
    }
}

fn Main() => Int throws LedgerError, IOException {
    ledger: Ledger = new();

    ledger.Open("checking", "Ada", Money {
        cents: 50'000,
        currency: "USD",
    });
    ledger.Open("savings", "Ada", Money {
        cents: 0,
        currency: "USD",
    });

    ledger.Apply(Transfer {
        sourceAccount: "checking",
        targetAccount: "savings",
        amount: Money {
            cents: 12'500,
            currency: "USD",
        },
        memo: STD.Optional.Some("monthly savings"),
    });

    ledger.Print();
    return 0;
}
