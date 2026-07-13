// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Complete-language example program:
// A small two-sided ledger with nominal money and audit records.

module Programs::BankLedger;

imports collections, std, process, result;

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
    accounts: std::collections::hash_map<Str, Account>;
    audit: std::collections::vector<Transfer>;

    Ledger() {
        self.accounts = std::collections::hash_map<Str, Account>::new();
        self.audit = std::collections::vector<Transfer>::new();
    }

    fn Open(id: Str, owner: Str, balance: Money) {
        self.accounts[id] = Account {
            id: id,
            owner: owner,
            balance: balance,
        };
    }

    fn Apply(transfer: Transfer) -> Result<(), LedgerError> {
        if (transfer.amount.cents <= 0) {
            return Error(LedgerError::InvalidAmount(transfer.amount.cents));
        }

        fromAccount: &mut Account = self.AccountMut(transfer.sourceAccount)@;
        toAccount: &mut Account = self.AccountMut(transfer.targetAccount)@;

        if (fromAccount.balance.cents < transfer.amount.cents) {
            return Error(LedgerError::InsufficientFunds(fromAccount.id));
        }

        fromAccount.balance.cents -= transfer.amount.cents;
        toAccount.balance.cents += transfer.amount.cents;
        self.audit.push(transfer);
        return Ok();
    }

    fn AccountMut(id: Str) -> Result<&mut Account, LedgerError> {
        if (!self.accounts.contains(id)) {
            return Error(LedgerError::UnknownAccount(id));
        }
        return Ok(&mut self.accounts[id]);
    }

    fn Print() -> Result<(), IOException> {
        for ((id, account): (Str, Account) in self.accounts) {
            println!("{} {} {}", id, account.owner, account.balance.cents);
        }
        return Ok();
    }
}

fn Main() -> Result<Int, Error> {
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
        memo: std::optional::Some("monthly savings"),
    })@;

    ledger.Print()@;
    return Ok(0);
}
