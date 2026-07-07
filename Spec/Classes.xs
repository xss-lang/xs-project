// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// class system:

//{
Class declarations use braces.
Parentheses are not used in class declarations.

Fields are supported.
Field initializers are optional.

Constructors are supported.
Constructor name must match the class name.

A class may contain at most one constructor.
Constructor overload is not supported.
Constructor merging is not supported.

Methods are supported.
Static methods are supported.
Static fields are supported.

Methods with the same name are merged.
The last method declaration determines the final parameter list and return type.

Single inheritance is supported through extends.
Interfaces are supported through implements.
Multiple interface implementation is supported.

Visibility modifiers:
- public
- private
- protected
- internal

Object creation uses new().
Method calls use the dot operator.

incomplete classes are supported.
}//

// basic class
class User {
}

// fields
class User {
    name: str;
    age: int;

    enabled: bool = true;
}

// constructor
class User {
    name: str;

    User(name: str) {
        this.name = name;
    }
}

// object creation
class User {
    User() {
    }
}

fn Main() {
    user: User = new();
}

// methods
class User {
    fn GetName() => str {
        return "Alfa";
    }
}

// method call
fn Main() {
    user: User = new();

    user.GetName();
}

// static field
class Math {
    static Pi: float = 3.141592653589793;
}

// static method
class Math {
    static fn Add(a: int, b: int) => int {
        return a + b;
    }
}

// inheritance
class Animal {
}

class Dog {
    extends Animal;
}

// interface
interface Runnable {
    fn Run();
}

// interface implementation
class Program {
    implements Runnable;

    fn Run() {
    }
}

// multiple interface implementation
interface Runnable {
    fn Run();
}

interface Closeable {
    fn Close();
}

class Program {
    implements Runnable, Closeable;

    fn Run() {
    }

    fn Close() {
    }
}

// visibility
class User {
    public name: str;

    private password: str;

    protected token: str;

    internal id: int;
}

// incomplete class
incomplete class Animal {
    incomplete fn Speak();
}

// method merging
class Math {
    fn Add(a: int) => int {
        x: int = 1;
    }

    fn Add(a: int, b: int) => int {
        y: int = 2;
    }
}

// equivalent
class Math {
    fn Add(a: int, b: int) => int {
        x: int = 1;
        y: int = 2;
    }
}


// VALID
class User {
}


// VALID
class User {
    name: str;
}


// VALID
class User {
    name: str = "Alfa";
}


// VALID
class User {
    User(name: str) {
    }
}


// INVALID
class User {
    User() {
    }

    User(name: str) {
    }
}
// A class can only have one constructor.


// VALID
class Math {
    static Counter: int = 0;
}


// VALID
class Program {
    implements Runnable, Closeable;
}


// INVALID
class User() {
}
// Parentheses are not allowed in class declarations.
