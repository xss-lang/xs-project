// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// iterator system:

//
// Iterators are supported.
//
// Iterators are lazy.
//
// Iterators are zero-cost abstractions.
//
// Iterators use monomorphization.
//
// Type erasure does not exist.
//
// Boxing does not exist.
//
// Virtual dispatch does not exist.
//
// Hidden allocations are forbidden.
//
// Iterator chains must not create intermediate collections.
//
// Iterator usage must compile to code equivalent
// to an explicit loop.
//
// Iterator iteration moves yielded values by default.
//
// The source collection remains valid after iteration.
//
// Ownership rules apply normally during iteration.
//
// break and continue are supported inside iterator loops.
//


imports collections;


// array iteration

nums: Int[] = {1, 2, 3};

for (num in nums.iter()) {

    println!("{}", num);
}


// vector iteration

users: std::collections::Vector<Str> = {
    "Leitewolf",
    "Alpha",
};

for (user in users.iter()) {

    println!("{}", user);
}


// empty collection iteration

users: std::collections::Vector<Str> =
    std::collections::Vector::new();

for (user in users.iter()) {

    println!("{}", user);
}

// loop body is never executed


// indexed iteration

users: std::collections::Vector<Str> = {
    "Leitewolf",
    "Alpha",
};

for ((index, user) in users.iter()) {

    println!("{}: {}", index, user);
}


// break

for (user in users.iter()) {

    if (user == "Alpha") {
        break;
    }
}


// continue

for (user in users.iter()) {

    if (user == "Leitewolf") {
        continue;
    }

    println!("{}", user);
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
    std::collections::Vector.length(users);


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

//
// Iterators are lazy.
//
// Iterators use monomorphization.
//
// Iterators are zero-cost abstractions.
//
// Iterator chains do not create
// intermediate collections.
//
// No hidden allocations.
//
// No boxing.
//
// No type erasure.
//
// No virtual dispatch.
//
