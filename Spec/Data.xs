// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// data system:

//
// Data types are lightweight field-only containers.
//
// Data declarations may appear:
//
// - At top-level
// - Inside classes
//
// Data types cannot contain:
//
// - Methods
// - Constructors
// - Destructors
// - Inheritance
// - Interfaces
//
// Data types support generics.
//
// Fields are assigned using:
//
// set.field{value}
//
// Fields are accessed using:
//
// value get.field
//

// simple data

data User {
    name: Str
    age: Int
}


// initialization

user: User = {
    set.name{"Alpha"};
    set.age{26};
};


// field access

println!("{}", user get.name);

println!("{}", user get.age);


// nested data

class Program {

    data User {
        name: Str
        age: Int
    }

}


// generic data

data Pair<T, U> {
    first: T
    second: U
}


// generic data initialization

pair: Pair<Str, Int> = {
    set.first{"Alpha"};
    set.second{26};
};


// generic data access

println!("{}", pair get.first);

println!("{}", pair get.second);


// multiple fields

data Point {
    x: Int
    y: Int
}


point: Point = {
    set.x{10};
    set.y{20};
};


println!("{}", point get.x);

println!("{}", point get.y);


// VALID

data User {
    name: Str
    age: Int
}


// VALID

data Pair<T, U> {
    first: T
    second: U
}


// VALID

class Program {

    data User {
        name: Str
    }

}


// VALID

user: User = {
    set.name{"Alpha"};
    set.age{26};
};


// VALID

user get.name

user get.age


// INVALID

data User {

    fn GetName() {
    }

}


// Methods are not allowed.


// INVALID

data User {

    User(name: Str) {
    }

}


// Constructors are not allowed.


// INVALID

data User {

    User.Drop() {
    }

}


// Destructors are not allowed.


// INVALID

data User {

    implements Runnable;

}


// Interfaces are not allowed.


// INVALID

data User {

    extends Person;

}


// Inheritance is not allowed.


// summary

//
// Data types are field-only containers.
//
// Data supports generics.
//
// Data may be declared at top-level
// or inside classes.
//
// Fields are assigned using:
//
// set.field{value}
//
// Fields are accessed using:
//
// value get.field
//
