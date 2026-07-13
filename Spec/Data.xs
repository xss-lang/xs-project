// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// data system:

//
// Data types are lightweight nominal containers.
//
// Data declarations may appear:
//
// - At top-level
// - Inside classes
//
// Data types may contain fields, constructors, and methods.
// Constructors and methods may be overloaded by parameter type list.
// Operator overload declarations use the method form:
//
// fn operator +(right: Type) -> Type { }
//
// A duplicate constructor or method parameter type list is an error.
// Data types do not support destructors, inheritance, or interfaces.
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


// overloaded constructors and methods

data Point {
    x: Int
    y: Int

    Point(x: Int, y: Int) {
        this.x = x;
        this.y = y;
    }

    Point(value: Int) {
        this.x = value;
        this.y = value;
    }

    fn Translate(dx: Int, dy: Int) { }
    fn Translate(offset: Point) { }

    fn operator +(right: Point) -> Point {
        return right;
    }
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
// Data types are nominal containers with fields, constructors, and methods.
// Constructors, methods, and operators may be overloaded by parameter type list.
// Data does not support destructors, inheritance, or interfaces.
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
