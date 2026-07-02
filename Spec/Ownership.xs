// ownership system:

//{
All values are move-by-default.

Assignments move ownership.
Function arguments move ownership.
Return values move ownership.

After a move, the source value becomes invalid.

Borrowing follows Rust-style rules.

Immutable borrow:
&T

Mutable borrow:
&mut T

At any given time, a value may have:

- Exactly one mutable borrow (&mut T)

or

- One or more immutable borrows (&T)

Mutable and immutable borrows cannot coexist.

Borrow checking is performed at compile time.

Lifetime analysis is performed at compile time.

nil is a valid value.

Ownership rules apply to all values,
including values containing nil.
}//

// move by assignment

user: User = new();

user2: User = user;

// user moved into user2
// user is now invalid


// move by function call

fn Consume(user: User) {
}

fn Main() {
    user: User = new();

    Consume(user);

    // user is now invalid
}


// move by return

fn CreateUser() => User {
    user: User = new();

    return user;
}

fn Main() {
    user: User = CreateUser();
}


// immutable borrow

user: User = new();

borrow1: &User = &user;
borrow2: &User = &user;


// immutable borrow function

fn Print(user: &User) {
}

fn Main() {
    user: User = new();

    Print(&user);
    Print(&user);
}


// mutable borrow

user: User = new();

borrow: &mut User = &mut user;


// mutable borrow function

fn Rename(user: &mut User) {
}

fn Main() {
    user: User = new();

    Rename(&mut user);
}


// VALID

user: User = new();

a: &User = &user;
b: &User = &user;


// VALID

user: User = new();

a: &User = &user;
b: &User = &user;
c: &User = &user;


// VALID

user: User = new();

a: &mut User = &mut user;


// VALID

user: User = new();

Rename(&mut user);


// VALID

user: User = nil;

if (user == nil) {
}


// INVALID

user: User = new();

user2: User = user;

user.GetName();

// user was moved


// INVALID

fn Consume(user: User) {
}

fn Main() {
    user: User = new();

    Consume(user);

    user.GetName();
}

// user was moved


// INVALID

user: User = new();

a: &User = &user;
b: &mut User = &mut user;

// mutable and immutable borrows cannot coexist


// INVALID

user: User = new();

a: &mut User = &mut user;
b: &User = &user;

// mutable and immutable borrows cannot coexist


// INVALID

user: User = new();

a: &mut User = &mut user;
b: &mut User = &mut user;

// multiple mutable borrows


// INVALID

user: User = new();

a: &User = &user;

user = new();

// value is immutably borrowed


// INVALID

user: User = new();

a: &mut User = &mut user;

user.GetName();

// value is mutably borrowed


// VALID

user: User = new();

a: &mut User = &mut user;

// borrow ends here

user.GetName();


// lifetime example

fn Print(user: &User) {
}

fn Main() {
    user: User = new();

    Print(&user);
}


// lifetime checking is performed at compile time

fn GetUser() => &User {
    user: User = new();

    return &user;
}

// INVALID
// returning a reference to a value that does not live long enough


// nil values

name: str = nil;

enabled: bool = nil;

user: User = nil;


// VALID

if (name == nil) {
}


// VALID

if (enabled == nil) {
}


// INVALID

if (enabled) {
}

// condition cannot be nil
