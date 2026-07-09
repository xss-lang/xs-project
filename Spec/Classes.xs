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

A class may contain overloaded constructors.
Constructor overload resolution follows the same arity/type selection model as function overload resolution.

Methods are supported.
Static methods are supported.
Static fields are supported.

Methods with the same name are overloaded.
Method merging is not supported.
Two methods with the same name and the same parameter type list are a duplicate declaration error.

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
    name: Str;
    age: Int;

    enabled: Bool = true;
}

// constructor
class User {
    name: Str;

    User(name: Str) {
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
    fn GetName() => Str {
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
    static Pi: Float = 3.141592653589793;
}

// static method
class Math {
    static fn Add(a: Int, b: Int) => Int {
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
    public name: Str;

    private password: Str;

    protected token: Str;

    internal id: Int;
}

// incomplete class
incomplete class Animal {
    incomplete fn Speak();
}

// method merging
class Math {
    fn Add(a: Int) => Int {
        x: Int = 1;
    }

    fn Add(a: Int, b: Int) => Int {
        y: Int = 2;
    }
}

// equivalent
class Math {
    fn Add(a: Int, b: Int) => Int {
        x: Int = 1;
        y: Int = 2;
    }
}


// VALID
class User {
}


// VALID
class User {
    name: Str;
}


// VALID
class User {
    name: Str = "Alfa";
}


// VALID
class User {
    User(name: Str) {
    }
}


// INVALID
class User {
    User() {
    }

    User(name: Str) {
    }
}
// A class can only have one constructor.


// VALID
class Math {
    static Counter: Int = 0;
}


// VALID
class Program {
    implements Runnable, Closeable;
}


// INVALID
class User() {
}
// Parentheses are not allowed in class declarations.
