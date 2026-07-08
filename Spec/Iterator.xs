// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// iterator system:

//{
Iterators are supported.

Iterators are lazy.

Iterators are zero-cost abstractions.

Iterators use monomorphization.

Type erasure does not exist.

Boxing does not exist.

Virtual dispatch does not exist.

Hidden allocations are forbidden.

Iterator chains must not create intermediate collections.

Iterator usage must compile to code equivalent
to an explicit loop.

Iterator iteration moves yielded values by default.

The source collection remains valid after iteration.

Ownership rules apply normally during iteration.

break and continue are supported inside iterator loops.
}//


imports Collections;


// array iteration

nums: Int[] = {1, 2, 3};

for (num in nums.iter()) {

    std.cout << num << "\n";
}


// vector iteration

users: Collections.vector<Str> = {
    "Hasan",
    "Alfa",
};

for (user in users.iter()) {

    std.cout << user << "\n";
}


// empty collection iteration

users: Collections.vector<Str> =
    Collections.vector.new();

for (user in users.iter()) {

    std.cout << user << "\n";
}

// loop body is never executed


// indexed iteration

users: Collections.vector<Str> = {
    "Hasan",
    "Alfa",
};

for ((index, user) in users.iter()) {

    std.cout
        << index
        << ": "
        << user
        << "\n";
}


// break

for (user in users.iter()) {

    if (user == "Alfa") {
        break;
    }
}


// continue

for (user in users.iter()) {

    if (user == "Hasan") {
        continue;
    }

    std.cout << user << "\n";
}


// iterator filtering

users
    .iter()
    .filter(...);


// iterator mapping

users
    .iter()
    .map(...);


// iterator chaining

users
    .iter()
    .filter(...)
    .map(...);


// mutable iteration

for (user in &mut users.iter()) {

}


// collection remains valid

for (user in users.iter()) {

}

count: Int =
    Collections.vector.length(users);


// VALID

for (item in users.iter()) {

}


// VALID

for ((index, item) in users.iter()) {

}


// VALID

users
    .iter()
    .filter(...)
    .map(...);


// VALID

for (item in &mut users.iter()) {

}


// performance guarantees

//{
Iterators are lazy.

Iterators use monomorphization.

Iterators are zero-cost abstractions.

Iterator chains do not create
intermediate collections.

No hidden allocations.

No boxing.

No type erasure.

No virtual dispatch.
}//
