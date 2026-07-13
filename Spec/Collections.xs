// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// collections system:

//
// Array:
// - T[]
// - T[n]
// - Fixed size.
// - Supports indexing.
// - Supports len().
// - Does not support push().
// - Does not support pop().
// - Array element modification is controlled by array/type mutability rules.
// - val array bindings cannot be reassigned, but val does not by itself make
//   the array value deeply immutable.
//
// Array size:
// - T[3] means indexes 0..3.
// - Total element count is 4.
//
// Array initialization:
// - Missing values are filled with the type's default value.
// - Excess values are discarded.
//
// Default values:
// - Numeric types -> 0
// - Bool -> None
// - Str -> None
//
// Vector:
// - std::collections::Vector<T>
// - Dynamic size.
// - Supports push().
// - Supports pop().
// - Supports get().
// - Supports adjust().
// - Supports length().
// - Supports literal initialization.
// - Supports new().
//
// HashMap:
// - std::collections::hash_map<K, V>
// - Supports new().
// - Supports insert().
// - Supports get().
// - Supports delete().
// - Does not support [] access.
// - Does not support literal initialization.
//

imports collections;


// arrays

nums: Int[] = {1, 2, 3};

nums[0] = 5;

length: Int = nums.len();


// fixed-size arrays

nums: Int[3] = {1, 2, 3, 4};

// Result:
// {1, 2, 3, 4}


// missing values

nums: Int[3] = {1, 2};

// Result:
// {1, 2, 0, 0}


// excess values

nums: Int[3] = {1, 2, 3, 4, 5};

// Result:
// {1, 2, 3, 4}


// Str defaults

names: Str[3] = {"Leitewolf"};

// Result:
// {"Leitewolf", None, None, None}


// Bool defaults

flags: Bool[2] = {};

// Result:
// {None, None, None}


// val array binding

val nums: Int[] = {1, 2, 3};

nums[0] = 5;

// Valid when the array value's mutability rules allow element assignment.
// Invalid only if the array/type rules make the element storage immutable.


// vectors

users: std::collections::Vector<Str> = {
    "Leitewolf",
    "Alpha",
};


// vector constructor

users: std::collections::Vector<Str> =
    std::collections::vector::new();


// vector push

std::collections::vector.push(users, "Friedrich");


// vector pop by value

std::collections::vector.pop(users, "Leitewolf");


// vector pop by index

std::collections::vector.pop(users[1]);


// vector get

name: Str =
    std::collections::vector.get(users[0]);


// vector update

std::collections::vector.adjust(
    users[1],
    "Friedrich"
);


// vector length

count: Int =
    std::collections::vector.length(users);


// hashmaps

scores: std::collections::hash_map<Str, Int> =
    std::collections::hash_map::new();


// insert

std::collections::hash_map.insert(
    scores,
    "Alpha",
    90
);

std::collections::hash_map.insert(
    scores,
    "Leitewolf",
    50
);


// get

score: Int =
    std::collections::hash_map.get(
        scores,
        "Alpha"
    );


// delete

std::collections::hash_map.delete(
    scores,
    "Alpha"
);


// output example

imports collections, std, format;

scores: std::collections::hash_map<Str, Int> =
    std::collections::hash_map::new();

std::collections::hash_map.insert(
    scores,
    "Alpha",
    90
);

std::collections::hash_map.insert(
    scores,
    "Leitewolf",
    50
);

println!("{}", std::collections::hash_map.get(scores, "Alpha"));

println!(
    "Leitewolf's score: {}",
    std::collections::hash_map.get(scores, "Leitewolf")
);


// VALID

nums: Int[] = {1, 2, 3};


// VALID

nums: Int[3] = {1, 2};


// VALID

users: std::collections::Vector<Str> =
    std::collections::vector::new();


// VALID

std::collections::vector.push(
    users,
    "Friedrich"
);


// VALID

std::collections::vector.get(
    users[0]
);


// VALID

std::collections::vector.adjust(
    users[1],
    "Friedrich"
);


// VALID

scores: std::collections::hash_map<Str, Int> =
    std::collections::hash_map::new();


// VALID

std::collections::hash_map.insert(
    scores,
    "Alpha",
    90
);


// VALID

std::collections::hash_map.get(
    scores,
    "Alpha"
);


// VALID

std::collections::hash_map.delete(
    scores,
    "Alpha"
);


// INVALID

std::collections::vector.get(
    users,
    0
);


// INVALID

scores["Alpha"];


// INVALID

scores: std::collections::hash_map<Str, Int> = {
    "Alpha": 90,
};


// INVALID

std::collections::vector.push(
    nums,
    5
);


// INVALID

std::collections::vector.pop(
    nums,
    0
);
