// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// collections system:

//{
Array:
- T[]
- T[n]
- Fixed size.
- Supports indexing.
- Supports len().
- Does not support push().
- Does not support pop().
- Mutable arrays allow element modification.
- val arrays do not allow element modification.

Array size:
- T[3] means indexes 0..3.
- Total element count is 4.

Array initialization:
- Missing values are filled with the type's default value.
- Excess values are discarded.

Default values:
- Numeric types -> 0
- bool -> nil
- str -> nil

Vector:
- Collections.vector<T>
- Dynamic size.
- Supports push().
- Supports pop().
- Supports get().
- Supports adjust().
- Supports length().
- Supports literal initialization.
- Supports new().

HashMap:
- Collections.hashmap<K, V>
- Supports new().
- Supports insert().
- Supports get().
- Supports delete().
- Does not support [] access.
- Does not support literal initialization.
}//

imports Collections;


// arrays

nums: int[] = {1, 2, 3};

nums[0] = 5;

length: int = nums.len();


// fixed-size arrays

nums: int[3] = {1, 2, 3, 4};

// Result:
// {1, 2, 3, 4}


// missing values

nums: int[3] = {1, 2};

// Result:
// {1, 2, 0, 0}


// excess values

nums: int[3] = {1, 2, 3, 4, 5};

// Result:
// {1, 2, 3, 4}


// string defaults

names: str[3] = {"Hasan"};

// Result:
// {"Hasan", nil, nil, nil}


// bool defaults

flags: bool[2] = {};

// Result:
// {nil, nil, nil}


// immutable arrays

val nums: int[] = {1, 2, 3};

nums[0] = 5;

// ERROR


// vectors

users: Collections.vector<str> = {
    "Hasan",
    "Alfa",
};


// vector constructor

users: Collections.vector<str> =
    Collections.vector.new();


// vector push

Collections.vector.push(users, "Mehmet");


// vector pop by value

Collections.vector.pop(users, "Hasan");


// vector pop by index

Collections.vector.pop(users[1]);


// vector get

name: str =
    Collections.vector.get(users[0]);


// vector update

Collections.vector.adjust(
    users[1],
    "Mehmet"
);


// vector length

count: int =
    Collections.vector.length(users);


// hashmaps

scores: Collections.hashmap<str, int> =
    Collections.hashmap.new();


// insert

Collections.hashmap.insert(
    scores,
    "Alfa",
    90
);

Collections.hashmap.insert(
    scores,
    "Hasan",
    50
);


// get

score: int =
    Collections.hashmap.get(
        scores,
        "Alfa"
    );


// delete

Collections.hashmap.delete(
    scores,
    "Alfa"
);


// output example

imports Collections, Stdio, Format;

scores: Collections.hashmap<str, int> =
    Collections.hashmap.new();

Collections.hashmap.insert(
    scores,
    "Alfa",
    90
);

Collections.hashmap.insert(
    scores,
    "Hasan",
    50
);

std.cout
    << Collections.hashmap.get(scores, "Alfa")
    << "\n";

std.cout
    << std.format(
        "Hasan's score: ${Collections.hashmap.get(scores, \"Hasan\")}\n"
    );


// VALID

nums: int[] = {1, 2, 3};


// VALID

nums: int[3] = {1, 2};


// VALID

users: Collections.vector<str> =
    Collections.vector.new();


// VALID

Collections.vector.push(
    users,
    "Mehmet"
);


// VALID

Collections.vector.get(
    users[0]
);


// VALID

Collections.vector.adjust(
    users[1],
    "Mehmet"
);


// VALID

scores: Collections.hashmap<str, int> =
    Collections.hashmap.new();


// VALID

Collections.hashmap.insert(
    scores,
    "Alfa",
    90
);


// VALID

Collections.hashmap.get(
    scores,
    "Alfa"
);


// VALID

Collections.hashmap.delete(
    scores,
    "Alfa"
);


// INVALID

Collections.vector.get(
    users,
    0
);


// INVALID

scores["Alfa"];


// INVALID

scores: Collections.hashmap<str, int> = {
    "Alfa": 90,
};


// INVALID

Collections.vector.push(
    nums,
    5
);


// INVALID

Collections.vector.pop(
    nums,
    0
);
