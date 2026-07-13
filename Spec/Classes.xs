// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// class system:

//
// Class declarations use braces.
// Parentheses are not used in class declarations.
//
// Fields are supported.
// Field initializers are optional.
// C#-style properties use getter and setter accessors.
//
// Constructors are supported.
// Constructor name must match the class name.
//
// A class may contain overloaded constructors.
// Constructor overload resolution follows the same arity/type selection model as function overload resolution.
//
// Methods are supported.
// Static methods are supported.
// Static fields are supported.
//
// Methods with the same name are overloaded.
// Method merging is not supported.
// Two methods with the same name and the same parameter type list are a duplicate declaration error.
//
// Single inheritance and interfaces use a C#-style base list.
// The first class type in the list is the base class; interface types are implemented interfaces.
// Multiple interface implementation is supported.
//
// Visibility modifiers:
// - public
// - private
// - protected
// - internal
//
// Visibility follows the C# access model:
// - public: visible from any module that can name the type/member.
// - private: visible only inside the declaring type or source scope.
// - protected: visible inside the declaring type and derived classes.
// - internal: visible inside the current project/package assembly boundary.
//
// Object creation uses new().
// Method calls use the dot operator.
//
// incomplete classes are supported.
//

// basic class
class User {
}

// fields
class User {
    name: Str;
    age: Int;

    enabled: Bool = true;
}

// properties
public class Person {
    public Str Name {
        getter;
        setter;
    }

    public Int Age {
        getter;
        setter;
    }
}

class User {
    private _age: Int;

    name: Str {
        getter;
        setter;
    }

    age: Int {
        getter {
            return self._age;
        }

        setter {
            self._age = value;
        }
    }
}

// constructor
class User {
    name: Str;

    User(name: Str) {
        self.name = name;
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
    fn GetName() -> Str {
        return "Alpha";
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
    static fn Add(a: Int, b: Int) -> Int {
        return a + b;
    }
}

// inheritance
class Animal {
}

class Dog : Animal {
}

// interface
interface Runnable {
    fn Run();
}

// interface implementation
class Program : Runnable {
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

class Program : Runnable, Closeable {
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

// method overloading
class Math {
    fn Add(a: Int) -> Int {
        return a;
    }

    fn Add(a: Int, b: Int) -> Int {
        return a + b;
    }
}

// These overloads stay distinct; X# does not merge method bodies.

// VALID
class User {
}


// VALID
class User {
    name: Str;
}


// VALID
class User {
    name: Str = "Alpha";
}


// VALID
class User {
    User(name: Str) {
    }
}


// VALID
class User {
    User() {
    }

    User(name: Str) {
    }
}
// Constructors may be overloaded by parameter type list.


// VALID
class Math {
    static Counter: Int = 0;
}


// VALID
class Program : Runnable, Closeable {
}


// INVALID
class User() {
}
// Parentheses are not allowed in class declarations.
