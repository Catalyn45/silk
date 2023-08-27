# sylk
<p align="center">
<img src="./logo.png" alt="drawing" width="30%"/>
</p>

Simple fully turing complete programming language that include almost all of the features of a common programming language.

## Building

Just use `make`. The project have no dependencies.

If you want to run the tests you need to install [google test](https://github.com/google/googletest). After
you install it just run `make test` and then `./sylk_test`.

## Running

`./sylk examples/fibonacci.slk` to run an example.

If you want to use the interpreter as a lib you just need to include the `sylk.h` header from the `src` directory
to use the functions and link the sylk library.

## Documentation

### Variable declaration
to declare a variable you need to use the `var` keyword.
```
var a
var b
```

you can also initialize the variables.
```
var text = 'hello world'
```

### Function declaration
to declare a function you need to use the `def` keyword.
```
def square(n) {
    return n * n
}
```

### Class declaration
to declare a function you need to use the `class` keyword
```
class Test {
    var a
    var b

    def constructor(a, b) {
        self.a = a
        self.b = b
    }

    def add() {
        return self.a + self.b
    }
}
```

Members are declared with the `var` keyword like variable declarations but the can't be initializated.

The constructor is declared as a normal method but it needs to be named "constructor".

You can access the instance in a method with the `self` keyword.

**note** first you need to declare the members and then the methods or it will not compile

## Conditionals
The conditionals works like in any other programming language
```
if a == 2 {
    print(a * a)
}
```

```
while n > 0 {
    print(n)
    n = n - 1
}
```

**note** there is no `for` statement yet.

### Assignment
you can assign a value to a variable with `=`
```
var b;
b = 15;
```

you can also assign a different type to a variable
```
var b = false
b = 'hello'
print(b)
```

**note** assignment is a statement and not an expression so you can't use assignments
where expressions are required.
```
while n = next() {
    print(n)
}
```
the code above will not compile.

## Roadmap

- [x] arithmetic operations
- [x] if/else
- [x] assignments
- [x] while
- [x] function calls
- [x] logical operations
- [x] function defenitions
- [x] string support
- [x] classes
- [x] list data structure
- [x] garbage collector
- [ ] standard library

