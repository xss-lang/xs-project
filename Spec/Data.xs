// data system:

//{
Data types are lightweight field-only containers.

Data declarations may appear:

- At top-level
- Inside classes

Data types cannot contain:

- Methods
- Constructors
- Destructors
- Inheritance
- Interfaces

Data types support generics.

Fields are assigned using:

set.field{value}

Fields are accessed using:

value get.field
}//

// simple data

data User {
    name: str
    age: int
}


// initialization

user: User = {
    set.name{"Alfa"};
    set.age{26};
};


// field access

std.cout << user get.name << "\n";

std.cout << user get.age << "\n";


// nested data

class Program {

    data User {
        name: str
        age: int
    }

}


// generic data

data Pair<T, U> {
    first: T
    second: U
}


// generic data initialization

pair: Pair<str, int> = {
    set.first{"Alfa"};
    set.second{26};
};


// generic data access

std.cout << pair get.first << "\n";

std.cout << pair get.second << "\n";


// multiple fields

data Point {
    x: int
    y: int
}


point: Point = {
    set.x{10};
    set.y{20};
};


std.cout << point get.x << "\n";

std.cout << point get.y << "\n";


// VALID

data User {
    name: str
    age: int
}


// VALID

data Pair<T, U> {
    first: T
    second: U
}


// VALID

class Program {

    data User {
        name: str
    }

}


// VALID

user: User = {
    set.name{"Alfa"};
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

    User(name: str) {
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

//{
Data types are field-only containers.

Data supports generics.

Data may be declared at top-level
or inside classes.

Fields are assigned using:

set.field{value}

Fields are accessed using:

value get.field
}//
