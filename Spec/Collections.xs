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
// - STD.Collections.vector<T>
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
// - STD.Collections.hash_map<K, V>
// - Supports new().
// - Supports insert().
// - Supports get().
// - Supports delete().
// - Does not support [] access.
// - Does not support literal initialization.
//

imports Collections;


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

users: STD.Collections.vector<Str> = {
    "Leitewolf",
    "Alpha",
};


// vector constructor

users: STD.Collections.vector<Str> =
    STD.Collections.vector.new();


// vector push

STD.Collections.vector.push(users, "Friedrich");


// vector pop by value

STD.Collections.vector.pop(users, "Leitewolf");


// vector pop by index

STD.Collections.vector.pop(users[1]);


// vector get

name: Str =
    STD.Collections.vector.get(users[0]);


// vector update

STD.Collections.vector.adjust(
    users[1],
    "Friedrich"
);


// vector length

count: Int =
    STD.Collections.vector.length(users);


// hashmaps

scores: STD.Collections.hash_map<Str, Int> =
    STD.Collections.hash_map.new();


// insert

STD.Collections.hash_map.insert(
    scores,
    "Alpha",
    90
);

STD.Collections.hash_map.insert(
    scores,
    "Leitewolf",
    50
);


// get

score: Int =
    STD.Collections.hash_map.get(
        scores,
        "Alpha"
    );


// delete

STD.Collections.hash_map.delete(
    scores,
    "Alpha"
);


// output example

imports Collections, Stdio, Format;

scores: STD.Collections.hash_map<Str, Int> =
    STD.Collections.hash_map.new();

STD.Collections.hash_map.insert(
    scores,
    "Alpha",
    90
);

STD.Collections.hash_map.insert(
    scores,
    "Leitewolf",
    50
);

println!("{}", STD.Collections.hash_map.get(scores, "Alpha"));

println!(
    "Leitewolf's score: {}",
    STD.Collections.hash_map.get(scores, "Leitewolf")
);


// VALID

nums: Int[] = {1, 2, 3};


// VALID

nums: Int[3] = {1, 2};


// VALID

users: STD.Collections.vector<Str> =
    STD.Collections.vector.new();


// VALID

STD.Collections.vector.push(
    users,
    "Friedrich"
);


// VALID

STD.Collections.vector.get(
    users[0]
);


// VALID

STD.Collections.vector.adjust(
    users[1],
    "Friedrich"
);


// VALID

scores: STD.Collections.hash_map<Str, Int> =
    STD.Collections.hash_map.new();


// VALID

STD.Collections.hash_map.insert(
    scores,
    "Alpha",
    90
);


// VALID

STD.Collections.hash_map.get(
    scores,
    "Alpha"
);


// VALID

STD.Collections.hash_map.delete(
    scores,
    "Alpha"
);


// INVALID

STD.Collections.vector.get(
    users,
    0
);


// INVALID

scores["Alpha"];


// INVALID

scores: STD.Collections.hash_map<Str, Int> = {
    "Alpha": 90,
};


// INVALID

STD.Collections.vector.push(
    nums,
    5
);


// INVALID

STD.Collections.vector.pop(
    nums,
    0
);
